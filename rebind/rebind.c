/* rebind: Copyright (C) 2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include	<unistd.h>
#include	<linux/elf.h>

#ifndef TRUE
#define	TRUE		1
#define	FALSE		0
#endif

#ifndef ELF32_ST_INFO
#define ELF32_ST_INFO(b, t) (((b) << 4) + ((t) & 0xF))
#endif

/* The memory-allocation macro.
 */
#define	alloc(p, n)	(((p) = realloc(p, n))				\
			    || (fputs("Out of memory.\n", stderr),	\
				exit(EXIT_FAILURE), 0))

/* The online help text.
 */
static char const *yowzitch =
	"Usage: rebind [-sw] OBJFILE [SYMBOL...]\n\n"
	"   -h  Display this help\n"
	"   -v  Display version information\n"
	"   -s  Change weak symbols into global symbols\n"
	"   -w  Change global symbols into weak symbols (the default)\n"
	"If no symbol names are given on the command line, the program\n"
	"reads the symbols from standard input.\n";

/* The version text.
 */
static char const *vourzhon =
	"rebind, version 0.9: Copyright (C) 2001 Brian Raiter\n";

static unsigned char	fromstrength;	/* the binding to change from */
static unsigned char	tostrength;	/* the binding to change to */
static char const      *tostrengthtext;	/* what to tell the user is going on */

static char	      **namelist;	/* the list of symbol names to seek */
static int		namecount;	/* the number of symbols in namelist */

static char const      *thefilename;	/* the current file name */
static FILE	       *thefile;	/* the current file handle */

static Elf32_Ehdr	ehdr;		/* the file's ELF header */


/* Standard qsort/bsearch string comparison function.
 */
static int qstrcmp(void const *a, void const *b)
{
    return strcmp(*(char const**)a, *(char const**)b);
}

/* An error-handling function. The given error message is used only
 * when errno is not set.
 */
static int err(char const *errmsg)
{
    if (errno)
	perror(thefilename);
    else
	fprintf(stderr, "%s: %s\n", thefilename, errmsg);
    return FALSE;
}

/* Reads and returns an area of the input file.
 */
static void *getarea(int off, int size)
{
    void       *buf = NULL;

    alloc(buf, size);
    if (fseek(thefile, off, SEEK_SET) || fread(buf, size, 1, thefile) != 1) {
	free(buf);
	return NULL;
    }
    return buf;
}

/* namelistfromfile() builds an array of all the non-whitespace strings
 * in the given file.
 */
static int namelistfromfile(FILE *fp)
{
    char	word[256];
    int		allocated = 0;
    int		n;

    while (fscanf(fp, "%255s", word) > 0) {
	if (namecount >= allocated) {
	    allocated = allocated ? allocated * 2 : 16;
	    alloc(namelist, allocated * sizeof *namelist);
	}
	namelist[namecount] = NULL;
	n = strlen(word) + 1;
	alloc(namelist[namecount], n);
	memcpy(namelist[namecount], word, n);
	++namecount;
    }
    return TRUE;
}


/* readheader() checks to make sure that this is in fact a proper ELF
 * object file that we're proposing to munge.
 */
static int readheader(void)
{
    int	endianness;

    /*
     * First, load the ELF header and do some sanity-checking.
     */

    errno = 0;
    if (fread(&ehdr, sizeof ehdr, 1, thefile) != 1)
	return err("not an ELF file.");
    if (ehdr.e_ident[EI_MAG0] != ELFMAG0 || ehdr.e_ident[EI_MAG1] != ELFMAG1
					 || ehdr.e_ident[EI_MAG2] != ELFMAG2
					 || ehdr.e_ident[EI_MAG3] != ELFMAG3)
	return err("not an ELF file.");

    endianness = 1;
    *(unsigned char*)&endianness = 0;
    if (ehdr.e_ident[EI_DATA] != (endianness ? ELFDATA2MSB : ELFDATA2LSB)) {
	fprintf(stderr, "%s: not %s-endian.\n",
			thefilename, endianness ? "big" : "little");
	return FALSE;
    }
    if (ehdr.e_ehsize != sizeof(Elf32_Ehdr))
	return err("unrecognized ELF header size");
    if (!ehdr.e_shoff)
	return err("no section header table.");
    if (ehdr.e_shentsize != sizeof(Elf32_Shdr))
	return err("unrecognized section header size");

    return TRUE;
}

/* changesymbols() finds all symbols in a given symbol table that
 * appear in the namelist with the specified binding and alters their
 * binding strength.
 */
static int changesymbols(Elf32_Sym *symtab, char const *strtab, int count)
{
    Elf32_Sym  *sym;
    char const *name;
    int		touched;
    int		i;

    touched = FALSE;
    for (i = 0, sym = symtab ; i < count ; ++i, ++sym) {
	if (ELF32_ST_BIND(sym->st_info) != fromstrength)
	    continue;
	name = strtab + sym->st_name;
	if (!bsearch(&name, namelist, namecount, sizeof *namelist, qstrcmp))
	    continue;
	sym->st_info = ELF32_ST_INFO(tostrength, ELF32_ST_TYPE(sym->st_info));
	printf("%s \"%s\".\n", tostrengthtext, name);
	touched = TRUE;
    }
    return touched;
}

/* rebind() does most of the grunt work. After checking over the ELF
 * headers, the function iterates through the sections, looking for
 * symbol tables containing non-local symbol. When it finds one, it
 * loads the non-local part of the table and the associated string
 * table into memory, and calls changesymbols(). If changesymbols()
 * actually changes anything, the altered symbol table is written back
 * out to the object file.
 */
static int rebind(void)
{
    Elf32_Shdr *shdrs;
    Elf32_Sym  *symtab;
    char       *strtab;
    unsigned	offset, count;
    int		i;

    if (!readheader())
	return FALSE;
    if (!(shdrs = getarea(ehdr.e_shoff, ehdr.e_shnum * sizeof(Elf32_Shdr))))
	return err("invalid section header table.");

    for (i = 0 ; i < ehdr.e_shnum ; ++i) {
	if (shdrs[i].sh_type != SHT_SYMTAB && shdrs[i].sh_type != SHT_DYNSYM)
	    continue;
	if (shdrs[i].sh_entsize != sizeof *symtab) {
	    err("symbol table of unrecognized structure ignored.");
	    continue;
	}
	offset = shdrs[i].sh_offset + shdrs[i].sh_info * sizeof *symtab;
	count = shdrs[i].sh_size / sizeof *symtab - shdrs[i].sh_info;
	if (!count)
	    continue;

	if (!(symtab = getarea(offset, count * sizeof *symtab)))
	    return err("invalid symbol table");
	if (!(strtab = getarea(shdrs[shdrs[i].sh_link].sh_offset,
			       shdrs[shdrs[i].sh_link].sh_size)))
	    return err("invalid associated string table");
	if (changesymbols(symtab, strtab, count)) {
	    if (fseek(thefile, offset, SEEK_SET)
		    || fwrite(symtab, sizeof *symtab, count, thefile) != count)
		return err("unable to write to the object file");
	}
	free(symtab);
	free(strtab);
    }

    free(shdrs);
    return TRUE;
}

/* main() interprets the command-line arguments, builds the array of
 * symbol names, opens the object file, and calls rebind().
 */
int main(int argc, char *argv[])
{
    int	n;

    fromstrength = STB_GLOBAL;
    tostrength = STB_WEAK;
    tostrengthtext = "Weakening";

    while ((n = getopt(argc, argv, "hsvw")) != EOF) {
	switch (n) {
	  case 's':
	    fromstrength = STB_WEAK;
	    tostrength = STB_GLOBAL;
	    tostrengthtext = "Strengthening";
	    break;
	  case 'w':
	    fromstrength = STB_GLOBAL;
	    tostrength = STB_WEAK;
	    tostrengthtext = "Weakening";
	    break;
	  case 'h':
	    fputs(yowzitch, stdout);
	    return EXIT_SUCCESS;
	  case 'v':
	    fputs(vourzhon, stdout);
	    return EXIT_SUCCESS;
	  default:
	    fputs(yowzitch, stderr);
	    return EXIT_FAILURE;
	}
    }
    if (optind == argc) {
	fputs(yowzitch, stderr);
	return EXIT_FAILURE;
    }

    thefilename = argv[optind];
    if (!(thefile = fopen(thefilename, "rb+"))) {
	err("unable to open.");
	return EXIT_FAILURE;
    }
    ++optind;

    if (optind == argc) {
	if (!namelistfromfile(stdin))
	    return EXIT_FAILURE;
    } else {
	namelist = argv + optind;
	namecount = argc - optind;
    }
    qsort(namelist, namecount, sizeof *namelist, qstrcmp);

    n = rebind() ? EXIT_SUCCESS : EXIT_FAILURE;

    fclose(thefile);
    return n;
}
