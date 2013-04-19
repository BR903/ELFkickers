/* shdrtab.c: Functions for handling the section header table entries.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#include	<stdlib.h>
#include	<ctype.h>
#include	"gen.h"
#include	"elf.h"
#include	"outbasic.h"
#include	"shdrtab.h"

/* The list of section header names that are already defined.
 */
static char const	       *shdrtaken[] = { "UNDEF", "ABS", "COMMON",
						"LORESERVE", "HIRESERVE",
						"LOPROC", "HIPROC" };

/* The file's section header table.
 */
static Elf32_Shdr const	       *shdrs = NULL;
static int			shdrnum = 0;

/* The file's section header string table.
 */
static char const	       *shstrtab = NULL;

/* The names of the sections used in the C enum.
 */
static char		      **shdrnames = NULL;

/* Returns the section header table entry for a given index.
 */
Elf32_Shdr const *getshdr(int shndx)
{
    if (shdrs && shndx > 0 && shndx < shdrnum)
	return shdrs + shndx;
    return NULL;
}

/* Returns the section's name from the section header string table.
 */
char const *getshdrtruename(int shndx)
{
    if (shstrtab && shndx > 0 && shndx < shdrnum)
	return shstrtab + shdrs[shndx].sh_name;
    return NULL;
}

/* Returns the enum name of a section for a given index.
 */
char const *getshdrname(int shndx)
{
    if (shdrnames && shndx > 0 && shndx < shdrnum)
	return shdrnames[shndx];
    return NULL;
}

/* Records the section header table and the section header string
 * table, and assigns each section a unique name. This function must
 * be called before any of the others in this module.
 */
int enumsections(Elf32_Ehdr const *ehdr, Elf32_Shdr const *shdrsin,
		 char const *shstrtabin)
{
    char const *str;
    int		conflict;
    int		i, n;

    shdrnum = ehdr->e_shnum;
    shdrs = shdrsin;
    shstrtab = shstrtabin;
    if (!shdrs)
	shdrnum = 0;
    if (!shdrnum)
	return TRUE;

    i = shdrnum * sizeof(char*);
    n = shdrs[ehdr->e_shstrndx].sh_size + shdrnum * 12;
    xalloc(shdrnames, i + n);
    *shdrnames = (char*)shdrnames + i;
    **shdrnames = '\0';
    for (i = 1 ; i < shdrnum ; ++i) {
	shdrnames[i] = shdrnames[i - 1] + strlen(shdrnames[i - 1]) + 1;
	str = shstrtab ? shstrtab + shdrs[i].sh_name : NULL;
	if (str && *str) {
	    if (*str == '.')
		++str;
	    conflict = FALSE;
	    for (n = 0 ; n < (int)(sizeof shdrtaken / sizeof *shdrtaken) ; ++n)
		if (!strcmp(str, shdrtaken[n])) {
		    conflict = TRUE;
		    break;
		}
	    strcpy(shdrnames[i], "SHN_");
	    for (n = 4 ; *str ; ++n, ++str)
		shdrnames[i][n] = isalnum(*str) ? toupper(*str) : '_';
	    if (!conflict) {
		for (n = 0 ; n < i ; ++n)
		    if (!strcmp(shdrnames[i], shdrnames[n])) {
			conflict = TRUE;
			break;
		}
	    }
	    if (conflict)
		sprintf(shdrnames[i] + strlen(shdrnames[i]), "_%d", i);
	} else
	    sprintf(shdrnames[i], "SHN_%d", i);
    }

    return TRUE;
}

/* Output the C enum containing the section header names. Returns
 * FALSE if names are not available.
 */
int outshdrnames(void)
{
    int	i;

    if (!shdrnum || !shdrnames)
	return FALSE;
    out("enum sections");
    beginblock(TRUE);
    outf("%s = 1", shdrnames[1]);
    for (i = 2 ; i < shdrnum ; ++i)
	out(shdrnames[i]);
    out("SHN_COUNT");
    endblock();
    out("\n");
    return TRUE;
}
