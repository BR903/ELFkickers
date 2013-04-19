/* shdrtab.h: Functions for handling the section header table entries.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#ifndef	_shdrtab_h_
#define	_shdrtab_h_

#include	"elf.h"

/* Initializes the section header table (this function must be called
 * before any of the others in this module), and assigns each section
 * a name (for the C enum).
 */
extern int enumsections(Elf32_Ehdr const *ehdr, Elf32_Shdr const *shdrs,
			char const *shstrtab);

/* Returns the section header table entry for a given index.
 */
extern Elf32_Shdr const *getshdr(int shndx);

/* Returns the enum name of a section for a given index.
 */
extern char const *getshdrname(int shndx);

/* Returns the actual name (as stored in the section header string
 * table) of a section for a given index, or NULL if no such name
 * exists.
 */
extern char const *getshdrtruename(int shndx);

/* Outputs the enumeration of the sections. Returns FALSE if no such
 * enumeration can be output.
 */
extern int outshdrnames(void);

#endif
