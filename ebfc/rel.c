/* rel.c: parts containing a relocation section.
 *
 * Copyright (C) 1999,2001,2021 by Brian Raiter, under the GNU General
 * Public License, version 2. No warranty. See COPYING for details.
 */

#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include "elfparts.h"
#include "elfpartsi.h"

/* Some elf.h headers don't define this macro.
 */
#ifndef	ELF64_R_INFO
#define	ELF64_R_INFO(sym, type) (((sym) << 32) | ((type) & 0xFFFFFFFF))
#endif

/* Set up the elfpart structure.
 */
static bool relnew(elfpart *part)
{
    part->shtype = SHT_REL;
    part->shname = ".rel";
    part->entsize = sizeof(Elf64_Rel);
    return true;
}

/* Set up the elfpart structure for a rela section.
 */
static bool relanew(elfpart *part)
{
    part->shtype = SHT_RELA;
    part->shname = ".rela";
    part->entsize = sizeof(Elf64_Rela);
    return true;
}

/* Name the relocation section based on the section it modifies, and
 * translate its info field from a part index to a section header
 * index.
 */
static bool init(elfpart *part, blueprint const *bp)
{
    int n;

    if (!_validate(bp->parts[part->info].shtype == SHT_PROGBITS,
                   ".rel requires a valid info setting"))
        return false;

    if (bp->parts[part->info].shname) {
        char const *destname = bp->parts[part->info].shname;
        int len1 = strlen(part->shname);
        int len2 = strlen(destname);
        char *str = malloc(len1 + len2 + 1);
        if (!_validate(str, "out of memory"))
            return false;
	memcpy(str, part->shname, len1);
        memcpy(str + len1, destname, len2 + 1);
	part->shname = str;
    }
    n = part->info;
    part->info = 1;
    while (n--)
        if (_partissection(&bp->parts[n]))
	    ++part->info;

    part->done = true;
    return true;
}

/* Update the relocations to have the correct symbol table indices.
 */
static bool relcomplete(elfpart *part, blueprint const *bp)
{
    Elf64_Rel *rel;
    int sym, i;

    if (!part->link->done)
	return true;

    for (i = 0, rel = part->part ; i < part->count ; ++i, ++rel) {
	sym = ELF64_R_SYM(rel->r_info);
	if (sym < 0)
	    sym = part->link->info - sym - 1;
	rel->r_info = ELF64_R_INFO(sym, ELF64_R_TYPE(rel->r_info));
    }

    part->done = true;
    return true;
    (void)bp;
}

/* Update the relocations to have the correct symbol table indices.
 */
static bool relacomplete(elfpart *part, blueprint const *bp)
{
    Elf64_Rela *rela;
    int sym, i;

    if (!part->link->done)
	return true;

    for (i = 0, rela = part->part ; i < part->count ; ++i, ++rela) {
	sym = ELF64_R_SYM(rela->r_info);
	if (sym < 0)
	    sym = part->link->info - sym - 1;
	rela->r_info = ELF64_R_INFO(sym, ELF64_R_TYPE(rela->r_info));
    }

    part->done = true;
    return true;
    (void)bp;
}

/* Adds a new entry to a relocation section. The return value is the
 * index of the entry.
 */
int addentrytorel(elfpart *part, unsigned int offset, int sym, int type)
{
    if (!_validate(part->shtype == SHT_REL || part->shtype == SHT_RELA,
                   "not a relocation section"))
        return -1;

    if (!_resizepart(part, part->size + part->entsize))
        return -1;
    if (part->shtype == SHT_REL) {
        Elf64_Rel *rel = part->part;
        rel += part->count;
        rel->r_offset = offset;
        rel->r_info = ELF64_R_INFO(sym, type);
    } else {
        Elf64_Rela *rela = part->part;
        rela += part->count;
        rela->r_offset = offset;
        rela->r_info = ELF64_R_INFO(sym, type);
        rela->r_addend = 0;
    }
    return part->count++;
}

/* Adds a new entry to a relocation section. The index of the given
 * symbol is retrieved automatically; if the symbol is not already
 * in the table, it is added to the symbol table as well.
 */
int addsymbolrel(elfpart *part, unsigned int offset, int reltype,
                 char const *sym, int symbind, int symtype, int shndx)
{
    int	n;

    if (!_validate(part->shtype == SHT_REL || part->shtype == SHT_RELA,
                   "not a relocation section"))
        return -1;

    if (!sym || (n = getsymindex(part->link, sym)) == 0)
	n = addtosymtab(part->link, sym, symbind, symtype, shndx);
    return addentrytorel(part, offset, n, reltype);
}

/* The relocation elfpart structures.
 */
elfpart part_rel = { relnew, init, NULL, relcomplete };
elfpart part_rela = { relanew, init, NULL, relacomplete };
