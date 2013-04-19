/* out.c: Output functions for each type of piece.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<string.h>
#include	"gen.h"
#include	"elf.h"
#include	"elftoc.h"
#include	"addr.h"
#include	"pieces.h"
#include	"shdrtab.h"
#include	"dynamic.h"
#include	"outbasic.h"
#include	"outtools.h"
#include	"out.h"


/* The beginning of every successful output.
 */
static char const *cprolog =
	"#include <stddef.h>\n"
	"#include \"elf.h\"\n"
	"\n";

/* All the macro definitions available for use in the output.
 */
#include	"names.h"

/* The output functions that go with each piece type.
 */
static int (*outfunctions[P_COUNT])(pieceinfo const*, void const*);

/* Outputs a section header index.
 */
static void outshdrname(int shndx)
{
    char const *str;

    if ((str = getshdrname(shndx)))
	out(str);
    else
	out(getname(shndx, section));
}

/* The output function for pieces of type P_UNCLAIMED, P_BYTES, or
 * P_SECTION. The contents are output either as a literal string, an
 * array of character values, or an array of hexadecimal byte values.
 * The last will be used if the contents contain an excess of
 * non-graphic, non-ASCII characters. Otherwise, one of the first two
 * representations will be selected based on whether or not the
 * contents appear to be null-terminated.
 */
static int bytesout(pieceinfo const *piece, void const *ptr)
{
    char const *str = ptr;
    char const *s;
    int		i, n, fullsize, size;

    fullsize = piece->size;
    if (!fullsize)
	return TRUE;
    for (size = fullsize ; size > 0 && !str[size - 1] ; --size) ;
    if (!size) {
	out("{ 0 }");
	return TRUE;
    }
    if (fullsize - size < 16)
	size = fullsize;

    n = stringsize(str, size);

    if (n * 2 > size * 3) {
	beginblock(TRUE);
	for (i = 0, s = str ; i < size ; ++i, ++s)
	    outf("0x%02X", (unsigned char)*s);
	if (size < fullsize)
	    outf("/* 0x00 x %d */", fullsize - size);
	endblock();
    } else if (size < fullsize || str[size - 1]) {
	beginblock(TRUE);
	for (i = 0, s = str ; i < size ; ++i, ++s)
	    out(cqchar(*s));
	if (size < fullsize)
	    outf("/* 0x00 x %d */", fullsize - size);
	endblock();
    } else
	outstring(str, size - 1);

    return TRUE;
}

/* The output functions for pieces of type P_HALVES. The contents will
 * be output as an array of hexadecimal or integer values, depending
 * on the range and average of the values.
 */
static int halvesout(pieceinfo const *piece, void const *ptr)
{
    Elf32_Half const   *half;
    char const	       *fmt = NULL;
    unsigned int	sum;
    int			i, fullsize, size;

    fullsize = piece->size / sizeof *half;
    half = ptr;
    for (size = fullsize ; size > 0 && !half[size - 1] ; --size) ;
    if (!size) {
	out("{ 0 }");
	return TRUE;
    }
    if (fullsize - size < 8)
	size = fullsize;

    sum = 0;
    for (i = 0, half = ptr ; i < size ; ++i, ++half) {
	if (*half > 0xFF) {
	    fmt = "0x%04X";
	    break;
	}
	sum += *half;
    }
    if (!fmt)
	fmt = sum / size < 16 ? "%u" : "0x%02X";

    beginblock(TRUE);
    for (i = 0, half = ptr ; i < size ; ++i, ++half)
	outf(fmt, (unsigned int)*half);
    if (size < fullsize) {
	char buf[32];
	sprintf(buf, "/* %s x %%d */", fmt);
	outf(buf, 0, fullsize - size);
    }
    endblock();

    return TRUE;
}

/* The output functions for pieces of type P_WORDS. The contents will
 * be output as an array of hexadecimal or integer values, depending
 * on the range and average of the values.
 */
static int wordsout(pieceinfo const *piece, void const *ptr)
{
    Elf32_Word const   *word;
    char const	       *fmt = NULL;
    unsigned int	sum;
    int			i, fullsize, size;

    fullsize = piece->size / sizeof *word;
    word = ptr;
    for (size = fullsize ; size > 0 && !word[size - 1] ; --size) ;
    if (!size) {
	out("{ 0 }");
	return TRUE;
    }
    if (fullsize - size < 8)
	size = fullsize;

    sum = 0;
    for (i = 0, word = ptr ; i < size ; ++i, ++word) {
	if (*word > 0xFFFF) {
	    fmt = "0x%08lX";
	    break;
	}
	sum += *word;
    }
    if (!fmt)
	fmt = sum / size < 256 ? "%lu" : "0x%04lX";

    beginblock(TRUE);
    for (i = 0, word = ptr ; i < size ; ++i, ++word)
	outf(fmt, (unsigned long)*word);
    if (size < fullsize) {
	char buf[32];
	sprintf(buf, "/* %s x %%d */", fmt);
	outf(buf, 0, fullsize - size);
    }
    endblock();

    return TRUE;
}

/* The output function for pieces of type P_NOTE. Most of the note
 * section is mainly free-form in nature, the only constraint being
 * alignment. It is displayed as an array of values, with line breaks
 * to indicate the beginning of a note header.
 */
static int noteout(pieceinfo const *piece, void const *ptr)
{
    Elf32_Word const   *word = ptr;
    int			i, n;

    beginblock(TRUE);
    i = 0;
    while (i < piece->size / 4) {
	n = (word[i] + word[i + 1] + 3) / 4;
	linebreak();
	outint(word[i++]);
	outint(word[i++]);
	outword(word[i++]);
	while (n-- && i < piece->size / 4)
	    outf("0x%08lX", (unsigned long)word[i++]);
    }
    endblock();
    return TRUE;
}

/* The output function for pieces of type P_HASH. The hash section is
 * simply displayed as an array of integers, with line breaks
 * indicating the starts of the bucket and chain arrays.
 */
static int hashout(pieceinfo const *piece, void const *ptr)
{
    Elf32_Word const   *word = ptr;
    Elf32_Shdr const   *shdr;
    int			i, n;

    beginblock(TRUE);
    n = piece->size / sizeof *word;
    if (n < 2 || (int)(word[0] + word[1] + 2) != n) {
	for (i = 0 ; i < n ; ++i, ++word)
	    outint(*word);
    } else {
	outint(word[0]);
	i = piecenum;
	if (word[1] && piece->shndx && (shdr = getshdr(piece->shndx))
				    && shdr->sh_link) {
	    for (i = 0 ; i < piecenum ; ++i)
		if (pieces[i].shndx == (int)(shdr->sh_link))
		    break;
	}
	if (i < piecenum
		&& word[1] * sizeof(Elf32_Sym) == (size_t)pieces[i].size)
	    outf("sizeof %s.%s / sizeof(Elf32_Sym)", varname, pieces[i].name);
	else
	    outint(word[1]);
	linebreak();
	for (i = 0 ; i < (int)word[0] ; ++i)
	    outint(word[i + 2]);
	linebreak();
	for (i = 0 ; i < (int)word[1] ; ++i)
	    outint(word[i + word[0] + 2]);
    }
    endblock();
    return TRUE;
}

/* The output functions for pieces of type P_SYMTAB. The contents are
 * interpreted as an array of Elf32_Sym structures.
 */
static int symsout(pieceinfo const *piece, void const *ptr)
{
    Elf32_Sym const    *sym;
    unsigned int	i;

    beginblock(TRUE);
    for (i = 0, sym = ptr ; i < piece->size / sizeof *sym ; ++i, ++sym) {
	beginblock(FALSE);
	outint(sym->st_name);
	out(getaddrstr(sym->st_value, NULL));
	outint(sym->st_size);
	outf("ELF32_ST_INFO(%s, %s)",
		getname(ELF32_ST_BIND(sym->st_info), symbind),
		getname(ELF32_ST_TYPE(sym->st_info), symtype));
	outint(sym->st_other);
	outshdrname(sym->st_shndx);
	endblock();
    }
    endblock();
    return TRUE;
}

/* The output function for pieces of type P_DYNAMIC. The contents are
 * displayed as an array of Elf32_Dyn structures. A first pass is used
 * to make connections between entries that have related information.
 */
static int dynout(pieceinfo const *piece, void const *ptr)
{
    char		addrs[N_DT_COUNT][128];
    int			shndx[N_DT_COUNT];
    Elf32_Dyn const    *dyn;
    unsigned int	i;
    int			n, done;

    memset(addrs, 0, sizeof addrs);
    memset(shndx, 0, sizeof shndx);

    for (i = 0, dyn = ptr ; i < piece->size / sizeof *dyn ; ++i, ++dyn) {
	switch (dyn->d_tag) {
	  case DT_STRTAB:
	    strcpy(addrs[N_DT_STRTAB],
		   getaddrstr(dyn->d_un.d_ptr, &shndx[N_DT_STRSZ]));
	    break;
	  case DT_REL:
	    strcpy(addrs[N_DT_REL],
		   getaddrstr(dyn->d_un.d_ptr, &shndx[N_DT_RELSZ]));
	    break;
	  case DT_RELA:
	    strcpy(addrs[N_DT_RELA],
		   getaddrstr(dyn->d_un.d_ptr, &shndx[N_DT_RELASZ]));
	    break;
	  case DT_JMPREL:
	    strcpy(addrs[N_DT_JMPREL],
		   getaddrstr(dyn->d_un.d_ptr, &shndx[N_DT_PLTRELSZ]));
	    break;
	  case DT_SYMINFO:
	    strcpy(addrs[N_DT_SYMINFO],
		   getaddrstr(dyn->d_un.d_ptr, &shndx[N_DT_SYMINSZ]));
	    break;
	  case DT_INIT_ARRAY:
	    strcpy(addrs[N_DT_INIT_ARRAY],
		   getaddrstr(dyn->d_un.d_ptr, &shndx[N_DT_INIT_ARRAYSZ]));
	    break;
	  case DT_FINI_ARRAY:
	    strcpy(addrs[N_DT_FINI_ARRAY],
		   getaddrstr(dyn->d_un.d_ptr, &shndx[N_DT_FINI_ARRAYSZ]));
	    break;
	  case DT_PREINIT_ARRAY:
	    strcpy(addrs[N_DT_PREINIT_ARRAY],
		   getaddrstr(dyn->d_un.d_ptr, &shndx[N_DT_PREINIT_ARRAYSZ]));
	    break;
	}
    }

    beginblock(TRUE);
    for (i = 0, dyn = ptr ; i < piece->size / sizeof *dyn ; ++i, ++dyn) {
	done = FALSE;
	beginblock(FALSE);
	out(getname(dyn->d_tag, dyntag));
	n = getdyntagid(dyn->d_tag);
	if (n <= 0) {
	    outf("{ %ld }", dyn->d_un.d_val);
	    done = TRUE;
	} else if (*addrs[n]) {
	    outf("{ %s }", addrs[n]);
	    done = TRUE;
	} else if (shndx[n]) {
	    outf("{ %s }", getsizestr(dyn->d_un.d_val, shndx[n]));
	    done = TRUE;
	} else {
	    switch (n) {
	      case N_DT_FLAGS:
		outf("{ %s }", getflags(dyn->d_un.d_val, dynflag));
		done = TRUE;
		break;
	      case N_DT_FLAGS_1:
		outf("{ %s }", getflags(dyn->d_un.d_val, dynflag1));
		done = TRUE;
		break;
	      case N_DT_FEATURE_1:
		outf("{ %s }", getflags(dyn->d_un.d_val, dynftrf1));
		done = TRUE;
		break;
	      case N_DT_POSFLAG_1:
		outf("{ %s }", getflags(dyn->d_un.d_val, dynposf1));
		done = TRUE;
		break;
	      case N_DT_RELAENT:
		if (dyn->d_un.d_val == sizeof(Elf32_Rela)) {
		    out("{ sizeof(Elf32_Rela) }");
		    done = TRUE;
		}
		break;
	      case N_DT_RELENT:
		if (dyn->d_un.d_val == sizeof(Elf32_Rel)) {
		    out("{ sizeof(Elf32_Rel) }");
		    done = TRUE;
		}
		break;
	      case N_DT_SYMENT:
		if (dyn->d_un.d_val == sizeof(Elf32_Sym)) {
		    out("{ sizeof(Elf32_Sym) }");
		    done = TRUE;
		}
		break;
	      case N_DT_SYMINENT:
		if (dyn->d_un.d_val == sizeof(Elf32_Syminfo)) {
		    out("{ sizeof(Elf32_Syminfo) }");
		    done = TRUE;
		}
		break;
	      case N_DT_PLTREL:
		if (dyn->d_un.d_val == DT_REL) {
		    out("{ DT_REL }");
		    done = TRUE;
		} else if (dyn->d_un.d_val == DT_RELA) {
		    out("{ DT_RELA }");
		    done = TRUE;
		}
		break;
	      case N_DT_PLTGOT:
	      case N_DT_INIT:
	      case N_DT_HASH:
	      case N_DT_FINI:
	      case N_DT_SYMTAB:
	      case N_DT_VERSYM:
	      case N_DT_VERDEF:
	      case N_DT_VERNEED:
		outf("{ %s }", getaddrstr(dyn->d_un.d_ptr, NULL));
		done = TRUE;
		break;
	      default:
		break;
	    }
	}
	if (!done)
	    outf("{ %ld }", (long)dyn->d_un.d_val);
	endblock();
    }

    endblock();
    return TRUE;
}

/* The output function for pieces of type P_REL. The contents are
 * displayed as an array of Elf32_Rel structures.
 */
static int relout(pieceinfo const *piece, void const *ptr)
{
    Elf32_Rel const    *rel;
    unsigned int	i;

    beginblock(TRUE);
    for (i = 0, rel = ptr ; i < piece->size / sizeof *rel ; ++i, ++rel) {
	beginblock(FALSE);
	out(getaddrstr(rel->r_offset, NULL));
	outf("ELF32_R_INFO(%u, %s)",
		(unsigned int)ELF32_R_SYM(rel->r_info),
		getname(ELF32_R_TYPE(rel->r_info), reltype));
	endblock();
    }
    endblock();
    return TRUE;
}

/* The output function for pieces of type P_RELA. The contents are
 * displayed as an array of Elf32_Rela structures.
 */
static int relaout(pieceinfo const *piece, void const *ptr)
{
    Elf32_Rela const   *rela;
    unsigned int	i;

    beginblock(TRUE);
    for (i = 0, rela = ptr ; i < piece->size / sizeof *rela ; ++i, ++rela) {
	beginblock(FALSE);
	outword(rela->r_offset);
	outf("ELF32_R_INFO(%u, %s)",
		(unsigned int)ELF32_R_SYM(rela->r_info),
		getname(ELF32_R_TYPE(rela->r_info), reltype));
	outf("%ld", (long)rela->r_addend);
	endblock();
    }
    endblock();
    return TRUE;
}

/* The output function for pieces of type P_EHDR. The contents are
 * displayed as an Elf32_Ehdr structure. (This is the only piece type
 * that is not interpreted as an array.)
 */
static int ehdrout(pieceinfo const *piece, void const *ptr)
{
    Elf32_Ehdr const   *ehdr = ptr;
    int			i, j;

    beginblock(TRUE);
    beginblock(FALSE);
    out(cqchar(ehdr->e_ident[EI_MAG0]));
    out(cqchar(ehdr->e_ident[EI_MAG1]));
    out(cqchar(ehdr->e_ident[EI_MAG2]));
    out(cqchar(ehdr->e_ident[EI_MAG3]));
    out(getname(ehdr->e_ident[EI_CLASS], eclass));
    out(getname(ehdr->e_ident[EI_DATA], edata));
    out(getname(ehdr->e_ident[EI_VERSION], eversion));
    for (i = EI_OSABI ; i < EI_NIDENT ; ++i)
	if (ehdr->e_ident[i])
	    break;
    if (i != EI_NIDENT) {
	out(getname(ehdr->e_ident[EI_OSABI], eosabi));
	outint(ehdr->e_ident[EI_ABIVERSION]);
	for (i = EI_PAD ; i < EI_NIDENT ; ++i) {
	    if (ehdr->e_ident[i])
		outf("0x%02X", (unsigned int)ehdr->e_ident[i]);
	    else
		out("0");
	}
    }
    endblock();
    out(getname(ehdr->e_type, elftype));
    out(getname(ehdr->e_machine, emachine));
    out(getname(ehdr->e_version, eversion));
    out(getaddrstr(ehdr->e_entry, NULL));
    if (!ehdr->e_phoff) {
	out("0");
	i = -1;
    } else
	out(getoffstr(ehdr->e_phoff, &i));
    if (!ehdr->e_shoff) {
	out("0");
	j = -1;
    } else
	out(getoffstr(ehdr->e_shoff, &j));
    outword(ehdr->e_flags);
    if (ehdr->e_ehsize == sizeof(Elf32_Ehdr))
	out("sizeof(Elf32_Ehdr)");
    else
	outint(ehdr->e_ehsize);
    if (ehdr->e_phentsize == sizeof(Elf32_Phdr))
	out("sizeof(Elf32_Phdr)");
    else
	outint(ehdr->e_phentsize);
    if (i >= 0 && ehdr->e_phnum
			== pieces[i].size / ctypes[pieces[i].type].size)
	outf("sizeof %s.%s / sizeof *%s.%s", varname, pieces[i].name,
					     varname, pieces[i].name);
    else
	outint(ehdr->e_phnum);
    if (ehdr->e_shentsize == sizeof(Elf32_Shdr))
	out("sizeof(Elf32_Shdr)");
    else
	outint(ehdr->e_shentsize);
    if (j >= 0 && ehdr->e_shnum
			== pieces[j].size / ctypes[pieces[j].type].size)
	outf("sizeof %s.%s / sizeof *%s.%s", varname, pieces[j].name,
					     varname, pieces[j].name);
    else
	outint(ehdr->e_shnum);
    outshdrname(ehdr->e_shstrndx);
    endblock();
    return TRUE;
    (void)piece;
}

/* The output function for pieces of type P_PHDRTAB. The contents are
 * displayed as an array of Elf32_Phdr structures.
 */
static int phdrsout(pieceinfo const *piece, void const *ptr)
{
    Elf32_Phdr const   *phdr;
    char const	       *offstr;
    char const	       *str;
    int			phnum, i, n;

    phnum = piece->size / sizeof *phdr;
    beginblock(TRUE);
    for (i = 0, phdr = ptr ; i < phnum ; ++i, ++phdr) {
	beginblock(FALSE);
	out(getname(phdr->p_type, ptype));
	n = getmemstrs(phdr->p_vaddr, phdr->p_offset, &str, &offstr);
	out(offstr);
	out(str);
	if (phdr->p_paddr == phdr->p_vaddr)
	    out(str);
	else if (phdr->p_paddr) {
	    getmemstrs(phdr->p_paddr, phdr->p_offset, &str, NULL);
	    out(str);
	} else
	    out("0");
	str = getsizestr(phdr->p_filesz, n);
	out(str);
	if (phdr->p_filesz == phdr->p_memsz)
	    out(str);
	else if (!phdr->p_filesz)
	    outword(phdr->p_memsz);
	else if (phdr->p_memsz > phdr->p_filesz)
	    outf("%s + 0x%02lX", str, (long)phdr->p_memsz - phdr->p_filesz);
	else
	    outword(phdr->p_memsz);
	out(getflags(phdr->p_flags, pflags));
	outword(phdr->p_align);
	endblock();
    }
    endblock();
    return TRUE;
}

/* The output function for pieces of type P_SHDRTAB. The contents are
 * displayed as an array of Elf32_Shdr structures.
 */
static int shdrsout(pieceinfo const *piece, void const *ptr)
{
    Elf32_Shdr const   *shdr;
    char const	       *addrstr;
    char const	       *offstr;
    char		buf[64];
    int			shnum, i, n, s;

    shnum = piece->size / sizeof *shdr;
    beginblock(TRUE);
    for (i = 0, shdr = ptr ; i < shnum ; ++i, ++shdr) {
	beginblock(FALSE);
	outint(shdr->sh_name);
	out(getname(shdr->sh_type, shtype));
	out(getflags(shdr->sh_flags, shflags));
	switch (shdr->sh_type) {
	  case SHT_SYMTAB:	s = P_SYMTAB;	break;
	  case SHT_DYNSYM:	s = P_SYMTAB;	break;
	  case SHT_DYNAMIC:	s = P_DYNAMIC;	break;
	  case SHT_REL:		s = P_REL;	break;
	  case SHT_RELA:	s = P_RELA;	break;
	  case SHT_HASH:	s = P_HASH;	break;
	  default:		s = 0;		break;
	}
	n = getmemstrs(shdr->sh_addr, shdr->sh_offset, &addrstr, &offstr);
	if (shdr->sh_type == SHT_NOTE && (int)shdr->sh_addr == pieces[n].size)
	    outf("sizeof %s.%s", varname, pieces[n].name);
	else
	    out(addrstr);
	out(offstr);
	if (shdr->sh_type == SHT_NOBITS)
	    outword(shdr->sh_size);
	else if (n >= 0 && (int)shdr->sh_size == pieces[n].size)
	    outf("sizeof %s.%s", varname, pieces[n].name);
	else if (shdr->sh_entsize || s) {
	    if (s && (!shdr->sh_entsize
				|| (int)(shdr->sh_entsize) == ctypes[s].size))
		sprintf(buf, "sizeof(%s)", ctypes[s].name);
	    else
		sprintf(buf, "%lu", (unsigned long)shdr->sh_entsize);
	    if (shdr->sh_size % shdr->sh_entsize == 0)
		outf("%lu * %s",
		     (unsigned long)shdr->sh_size / shdr->sh_entsize, buf);
	    else
		outf("%lu * %s + %lu",
		     (unsigned long)shdr->sh_size / shdr->sh_entsize, buf,
		     (unsigned long)shdr->sh_size % shdr->sh_entsize);
	} else if (n >= 0)
	    out(getsizestr(shdr->sh_size, n));
	else
	    outword(shdr->sh_size);
	outshdrname(shdr->sh_link);
	if ((shdr->sh_type == SHT_REL || shdr->sh_type == SHT_RELA)
			&& shdr->sh_info)
	    outshdrname(shdr->sh_info);
	else
	    outint(shdr->sh_info);
	outint(shdr->sh_addralign);
	if (s && (int)shdr->sh_entsize == ctypes[s].size)
	    outf("sizeof(%s)", ctypes[s].name);
	else
	    outint(shdr->sh_entsize);
	endblock();
    }
    endblock();
    return TRUE;
}

/* The output function for pieces of type P_NONEXISTENT. This type
 * does not actually represent anything in the file, but simply makes
 * padding bytes explicit.
 */
static int nothingout(pieceinfo const *piece, void const *ptr)
{
    outf("{ 0 }");
    return TRUE;
    (void)piece;
    (void)ptr;
}

/* Begins the output of the C code. Outputs the C prolog, the macro
 * definitions for the memory addresses, the section header enum, the
 * structure definition, and the beginning of the initialization.
 */
void beginoutpieces(void)
{
    pieceinfo  *p;
    int		i, f;

    out(cprolog);

    if (addrnum && addrs) {
	for (i = 0 ; i < addrnum ; ++i)
	    if (addrs[i].addr)
		outf("#define %-24s0x%08X\n", addrs[i].name, addrs[i].addr);
	out("\n");
    }
    f = outshdrnames();

    outf("typedef struct %s\n{\n", structname);
    for (i = 0, p = pieces ; i < piecenum ; ++i, ++p) {
	if (p->warn)
	    outf("    %-20s%s[%d];", ctypes[P_SECTION].name, p->name, p->size);
	else if (p->type == P_EHDR)
	    outf("    %-20s%s;", ctypes[p->type].name, p->name);
	else if (p->type == P_SHDRTAB && f)
	    outf("    %-20s%s[SHN_COUNT];", ctypes[p->type].name, p->name);
	else
	    outf("    %-20s%s[%d];", ctypes[p->type].name, p->name,
				     p->size / ctypes[p->type].size);
	out("\n");
    }
    outf("} %s;", structname);
    out("\n\n");

    outf("%s %s = ", structname, varname);
    beginblock(TRUE);
}

/* Outputs the initialization code for a field in the structure, which
 * corresponds to one file piece. contents points to the actual
 * contents of the piece.
 */
void outpiece(pieceinfo const *piece, void const *contents)
{
    char	buf[256];
    int		type;

    linebreak();
    outcomment(piece->name);
    linebreak();

    type = piece->type;
    if (piece->warn) {
	sprintf(buf, "This section is of type %s but has the wrong %s",
		     ctypes[type].name,
		     (piece->warn == PW_MISALIGNED ? "alignment" :
		      piece->warn == PW_WRONGSIZE ? "size"
						  : "alignment and size"));
	outcomment(buf);
	linebreak();
	type = P_SECTION;
    }

    if (!contents && type != P_NONEXISTENT) {
	beginblock(FALSE);
	outcomment("??? invalid file contents ???");
	endblock();
    } else
	(*outfunctions[type])(piece, contents);
}

/* Outputs the end of the initialization.
 */
void endoutpieces(void)
{
    endblock();
}

/* The actual outfunctions array.
 */
static int (*outfunctions[P_COUNT])(pieceinfo const*, void const*) = {
    bytesout,
    bytesout,
    bytesout,
    halvesout,
    wordsout,
    noteout, hashout, symsout,
    relout, relaout, dynout,
    shdrsout, phdrsout, ehdrout,
    nothingout
};
