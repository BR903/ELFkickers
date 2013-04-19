/* elftoc.c: The central module.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<stdarg.h>
#include	<errno.h>
#include	<sys/stat.h>
#include	"gen.h"
#include	"elf.h"
#include	"pieces.h"
#include	"addr.h"
#include	"shdrtab.h"
#include	"dynamic.h"
#include	"out.h"
#include	"elftoc.h"

/* The name to use for the initialized variable in the generated C code.
 */
char const	       *varname;

/* The name to use for the structure defined in the generated C code.
 */
char const	       *structname;

/* The output file.
 */
FILE		       *outfile;

/* The name of this program, taken from argv[0]. Used in error
 * messages.
 */
static char const      *thisprog;

/* The name of the input file, taken from argv[1].
 */
static char const      *thefilename = NULL;

/* The input file.
 */
static FILE	       *infile = NULL;

/* The input file's ELF header.
 */
static Elf32_Ehdr      *ehdr = NULL;

/* The input file's program header table, if such is present.
 */
static Elf32_Phdr      *phdrs = NULL;

/* The input file's section header table, if such is present.
 */
static Elf32_Shdr      *shdrs = NULL;

/* The size of the input file.
 */
static int		thefilesize;

/* Displays a formatted error message on stderr. If fmt is NULL, then
 * prints a canned out-of-memory error and exits the program directly.
 * Otherwise, FALSE is returned.
 */
int err(char const *fmt, ...)
{
    va_list	args;

    if (!fmt) {
	fputs("Out of memory!\n", stderr);
	exit(EXIT_FAILURE);
    }
    fprintf(stderr, "%s: ", thefilename ? thefilename : thisprog);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    va_end(args);
    return FALSE;
}

/* A macro to simplify using errno in reporting errors.
 */
#define	errnomsg(msg)	\
    (err(errno ? errno == ENOMEM ? NULL : strerror(errno) : (msg)))

/* Reads and returns an area of the input file.
 */
static void *getarea(int off, int size)
{
    void       *buf = NULL;

    xalloc(buf, size);
    if (fseek(infile, off, SEEK_SET) || fread(buf, size, 1, infile) != 1) {
	free(buf);
	return NULL;
    }
    return buf;
}

/* Reads in the file's ELF header, and runs several sanity checks on
 * the structure's contents. FALSE is returned if the file looks to
 * not be an ELF file at all.
 */
static int readelfhdr(void)
{
    static Elf32_Ehdr	ehdrbuf = { { 0 } };
    unsigned int	n;

    ehdr = &ehdrbuf;
    errno = 0;
    n = fread(ehdr, 1, sizeof *ehdr, infile);
    if (n <= EI_MAG3 || ehdr->e_ident[EI_MAG0] != ELFMAG0
		     || ehdr->e_ident[EI_MAG1] != ELFMAG1
		     || ehdr->e_ident[EI_MAG2] != ELFMAG2
		     || ehdr->e_ident[EI_MAG3] != ELFMAG3)
	return err("not an ELF file.");
    if (n < sizeof *ehdr) {
	err("warning: file does not contain a complete ELF header.");
	recordpiece(0, n, P_SECTION, 0, "ehdr");
    } else
	recordpiece(0, n, P_EHDR, 0, "ehdr");

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32)
	err("warning: not a 32-bit ELF file.");
    n = 1;
    *(char*)&n = 0;
    if (ehdr->e_ident[EI_DATA] != (n ? ELFDATA2MSB : ELFDATA2LSB))
	err("warning: not a %s-endian ELF file.", (n ? "big" : "little"));
    if (ehdr->e_ident[EI_VERSION] != EV_CURRENT)
	err("warning: unknown ELF header version.");
    if (ehdr->e_version != EV_CURRENT)
	err("warning: unknown ELF version.");

    if (ehdr->e_ehsize != sizeof(Elf32_Ehdr))
	err("warning: unrecognized ELF header size.");
    if (ehdr->e_phoff && ehdr->e_phentsize != sizeof(Elf32_Phdr))
	err("warning: unrecognized program header table entry size.");
    if (ehdr->e_shoff && ehdr->e_shentsize != sizeof(Elf32_Shdr))
	err("warning: unrecognized section header table entry size.");

    return TRUE;
}

/* Reads in the file's program header table and/or section header
 * table, and does some basic sanity checking. If a section header
 * table is present, the function also attempts to read the section
 * header string table.
 */
static int readhdrtables(void)
{
    int	n;

    if (ehdr->e_phoff) {
	n = ehdr->e_phnum * sizeof *phdrs;
	if ((phdrs = getarea(ehdr->e_phoff, n)))
	    recordpiece(ehdr->e_phoff, n, P_PHDRTAB, 0, "phdrs");
	else
	    err("warning: invalid program header table offset.\n");
    }

    if (ehdr->e_shoff) {
	n = ehdr->e_shnum * sizeof *shdrs;
	if ((shdrs = getarea(ehdr->e_shoff, n)))
	    recordpiece(ehdr->e_shoff, n, P_SHDRTAB, 0, "shdrs");
	else
	    err("warning: invalid section header table offset.\n");
    }

    if (!phdrs && !shdrs)
	err("warning: ELF file has no valid header tables.");

    if (shdrs && ehdr->e_shstrndx) {
	if (ehdr->e_shstrndx >= ehdr->e_shnum)
	    err("warning: invalid section header string table");
	else
	    enumsections(ehdr, shdrs,
			 getarea(shdrs[ehdr->e_shstrndx].sh_offset,
				 shdrs[ehdr->e_shstrndx].sh_size));
    }

    return TRUE;
}

/* Uses the program header table to find the file offset corresponding
 * to a memory address. -1 is returned if no such offset exists.
 */
static int addrtooffset(unsigned addr)
{
    Elf32_Phdr *phdr;
    unsigned	i;

    for (i = 0, phdr = phdrs ; i < ehdr->e_phnum ; ++i, ++phdr)
	if (phdr->p_vaddr <= addr && phdr->p_vaddr + phdr->p_memsz > addr)
	    return addr - phdr->p_vaddr + phdr->p_offset;
    return -1;
}

/* Records the contents referenced by various entries in the dynamic
 * object as separate pieces of the file. This function helps make up
 * for the absence of such information when a file contains no section
 * header table.
 */
static void getdynamicentries(Elf32_Dyn const *dynamic)
{
    Elf32_Dyn const    *dyn;
    Elf32_Sword		val[N_DT_COUNT];
    int		       *p;
    int			off, n, i;

    memset(val, 0, sizeof val);
    for (i = 0, dyn = dynamic ; dyn->d_tag != DT_NULL ; ++i, ++dyn)
	if ((n = getdyntagid(dyn->d_tag)) > 0)
	    val[n] = dyn->d_un.d_val;

#define	trysegment(addr, sz, off, type, name)				\
    (((off) = addrtooffset(addr)) >= 0					\
	&& (recordpiece((off), ((off) + (sz) > thefilesize ?		\
				     thefilesize - (off) : (sz)),	\
			(type), 0, (name)), 0))

    trysegment(val[N_DT_STRTAB], val[N_DT_STRSZ], off, P_SECTION, "~dynstr");
    trysegment(val[N_DT_SYMINFO], val[N_DT_SYMINSZ],
	       off, P_HALVES, "~syminfo");
    trysegment(val[N_DT_INIT_ARRAY], val[N_DT_INIT_ARRAYSZ],
	       off, P_WORDS, "~init_array");
    trysegment(val[N_DT_FINI_ARRAY], val[N_DT_FINI_ARRAYSZ],
	       off, P_WORDS, "~fini_array");
    trysegment(val[N_DT_PREINIT_ARRAY], val[N_DT_PREINIT_ARRAYSZ],
	       off, P_WORDS, "~preinit_array");
    if (val[N_DT_RELENT] == sizeof(Elf32_Rel))
	trysegment(val[N_DT_REL], val[N_DT_RELSZ], off, P_REL, "~rel_got");
    if (val[N_DT_RELAENT] == sizeof(Elf32_Rela))
	trysegment(val[N_DT_RELA], val[N_DT_RELASZ], off, P_RELA, "~rela_got");
    n = val[N_DT_PLTREL] == sizeof(Elf32_Rel) ? P_REL :
	val[N_DT_PLTREL] == sizeof(Elf32_Rela) ? P_RELA : 0;
    if (n)
	trysegment(val[N_DT_JMPREL], val[N_DT_PLTRELSZ], off, n,
		   n == P_REL ? "~plt_rel" : "plt_rela");

#undef trysegment

    if ((off = addrtooffset(val[N_DT_HASH])) >= 0) {
	if ((p = getarea(off, 8))) {
	    n = (p[0] + p[1] + 2) * sizeof(Elf32_Word);
	    if (off + n > thefilesize)
		n = thefilesize - off;
	    recordpiece(off, n, P_HASH, 0, "~hash");
	    off = addrtooffset(val[N_DT_SYMTAB]);
	    if (off >= 0 && val[N_DT_SYMENT] == sizeof(Elf32_Sym)) {
		n = p[1] * sizeof(Elf32_Sym);
		if (off + n > thefilesize)
		    n = thefilesize - off;
		recordpiece(off, n, P_SYMTAB, 0, "~dynsym");
	    }
	    free(p);
	}
    }
}

/* Records the contents referenced by the various entries in the
 * program header table and the section header table, each as a
 * separate piece of the file. Attempts to assign each piece an
 * appropriate name. Addresses, if present, are also recorded.
 */
static int recordsections(void)
{
    char	buf[64];
    char const *str;
    int		type;
    int		i, j;

    if (phdrs) {
	for (i = 0 ; i < ehdr->e_phnum ; ++i) {
	    switch (phdrs[i].p_type) {
	      case PT_NULL:	type = 0;				break;
	      case PT_PHDR:	type = P_PHDRTAB;  str = "~phdrs";	break;
	      case PT_DYNAMIC:	type = P_DYNAMIC;  str = "~dynamic";	break;
	      case PT_INTERP:	type = P_SECTION;  str = "~interp";	break;
	      case PT_NOTE:	type = P_NOTE;     str = "~note";	break;
	      default:		type = P_BYTES;	   str = NULL;		break;
	    }
	    if (!type)
		continue;
	    if (!str) {
		if (phdrs[i].p_flags & PF_X)
		    str = "~text";
		else if (phdrs[i].p_flags & PF_W)
		    str = "~data";
		else
		    str = "~segment";
	    }
	    j = phdrs[i].p_offset + phdrs[i].p_filesz;
	    if (j > thefilesize)
		j = thefilesize;
	    j -= phdrs[i].p_offset;
	    recordpiece(phdrs[i].p_offset, j, type, 0, str);
	    if ((phdrs[i].p_flags & PF_R) && phdrs[i].p_vaddr)
		recordaddr(phdrs[i].p_vaddr, phdrs[i].p_offset,
			   phdrs[i].p_memsz, str);
	    if (type == P_DYNAMIC)
		getdynamicentries(getarea(phdrs[i].p_offset, j));
	}
    }

    if (shdrs) {
	for (i = 0 ; i < ehdr->e_shnum ; ++i) {
	    switch (shdrs[i].sh_type) {
	      case SHT_NULL:		type = 0;			break;
	      case SHT_NOBITS:		type = 0;			break;
	      case SHT_SYMTAB:		type = P_SYMTAB;		break;
	      case SHT_DYNSYM:		type = P_SYMTAB;		break;
	      case SHT_HASH:		type = P_HASH;			break;
	      case SHT_DYNAMIC:		type = P_DYNAMIC;		break;
	      case SHT_REL:		type = P_REL;			break;
	      case SHT_RELA:		type = P_RELA;			break;
	      case SHT_NOTE:		type = P_NOTE;			break;
	      case SHT_GNU_verdef:	type = P_WORDS;			break;
	      case SHT_GNU_verneed:	type = P_WORDS;			break;
	      case SHT_GNU_versym:	type = P_HALVES;		break;
	      default:
		  if (shdrs[i].sh_entsize == 4)
		      type = P_WORDS;
		  else if (shdrs[i].sh_entsize == 2)
		      type = P_HALVES;
		  else
		      type = P_SECTION;
		  break;
	    }
	    if (!type)
		continue;
	    if ((str = getshdrtruename(i))) {
		if (type == P_SECTION && !strcmp(str, ".stab"))
		    type = P_WORDS;
		strncpy(buf, str, sizeof buf);
		buf[sizeof buf - 1] = '\0';
		for (j = 0 ; buf[j] && !isalnum(buf[j]) ; ++j) ;
		str = buf + j;
		for ( ; buf[j] ; ++j)
		    if (!isalnum(buf[j]))
			buf[j] = '_';
	    } else
		str = "~section";
	    j = shdrs[i].sh_offset + shdrs[i].sh_size;
	    if (j > thefilesize)
		j = thefilesize;
	    j -= shdrs[i].sh_offset;
	    recordpiece(shdrs[i].sh_offset, j, type, i, str);
	    if ((shdrs[i].sh_flags & SHF_ALLOC) && shdrs[i].sh_addr)
		recordaddr(shdrs[i].sh_addr, shdrs[i].sh_offset,
			   shdrs[i].sh_size, str);
	}
    }

    return TRUE;
}

/* Selects a unique name for each piece, to be used as the name of the
 * field in the C structure.
 */
static int setnames(void)
{
    int	i, j, n;

    n = 0;
    for (i = 0 ; i < piecenum ; ++i) {
	if (*pieces[i].name != '~')
	    continue;
	if (pieces[i].type == P_UNCLAIMED
		|| (pieces[i].type == P_BYTES && pieces[i].size < 16
					      && !(pieces[i].to & 3)))
	    sprintf(pieces[i].name, "pad%d", ++n);
	else
	    memmove(pieces[i].name, pieces[i].name + 1,
				    sizeof pieces->name - 1);
    }

    for (i = 0 ; i < piecenum - 1 ; ++i) {
	n = 1;
	for (j = i + 1 ; j < piecenum ; ++j)
	    if (!strcmp(pieces[i].name, pieces[j].name))
		sprintf(pieces[j].name, "%s%d", pieces[i].name, ++n);
	if (n > 1)
	    strcat(pieces[i].name, "1");
    }

    setaddrnames();

    return TRUE;
}

/* main().
 */
int main(int argc, char *argv[])
{
    void       *buf;
    struct stat	st;
    int		i;

    thisprog = argv[0];
    structname = "elf";
    varname = "foo";
    outfile = stdout;

    if (argc != 2) {
	err("Usage: elftoc ELFFILENAME");
	return argc < 2;
    }
    thefilename = argv[1];

    if (stat(thefilename, &st)) {
	errnomsg("can't stat file");
	return 1;
    }
    thefilesize = st.st_size;

    if (!(infile = fopen(thefilename, "r"))
		|| !readelfhdr()
		|| !readhdrtables())
	return 1;

    recordpiece(0, thefilesize, P_UNCLAIMED, 0, "~unused");
    recordsections();
    arrangepieces();
    verifypiecetypes();

    setnames();
    beginoutpieces();
    for (i = 0 ; i < piecenum ; ++i) {
	buf = getarea(pieces[i].from, pieces[i].size);
	outpiece(pieces + i, buf);
	free(buf);
    }
    endoutpieces();

    return 0;
}
