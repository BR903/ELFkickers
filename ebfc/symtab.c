/* symtab.c: parts containing a symbol table.
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
#ifndef	ELF64_ST_INFO
#define	ELF64_ST_INFO(bind, type) (((bind) << 4) | ((type) & 0x0F))
#endif

/* Dynamic sections automatically get an entry in the symbol table.
 */
#define	NAME_DYNAMIC "_DYNAMIC"

/* Set up the elfpart structure and allocate an empty symbol table.
 */
static bool symnew(elfpart *part)
{
    static Elf64_Sym const blanksym = { .st_shndx = SHN_UNDEF };

    part->shtype = SHT_SYMTAB;
    part->shname = ".symtab";
    part->info = 1;
    part->entsize = sizeof(Elf64_Sym);
    part->count = 1;
    if (!_resizepart(part, part->entsize))
        return false;
    *((Elf64_Sym*)part->part) = blanksym;
    return true;
}

/* Translate the symbols' shndx fields from a part index to a section
 * header index.
 */
static bool symcomplete(elfpart *part, blueprint const *bp)
{
    Elf64_Sym *sym;
    int i, n;

    for (i = 0, sym = part->part ; i < part->count ; ++i, ++sym) {
	if (sym->st_shndx > 0 && sym->st_shndx < bp->partcount) {
	    n = sym->st_shndx;
	    sym->st_shndx = 1;
	    while (n--)
		if (_partissection(&bp->parts[n]))
		    ++sym->st_shndx;
	}
    }

    part->done = true;
    return true;
    (void)bp;
}

/* Set up the elfpart structure for a dynamic symbol table.
 */
static bool dynnew(elfpart *part)
{
    if (!symnew(part))
        return false;
    part->shtype = SHT_DYNSYM;
    part->shname = ".dynsym";
    part->flags = PF_R;
    return true;
}

/* If the parts list has a .dynamic section, then add a _DYNAMIC
 * symbol to the symbol table.
 */
static bool dyninit(elfpart *part, blueprint const *bp)
{
    foreachpart (p0 in bp) {
        if (p0->shtype == SHT_DYNAMIC) {
	    addtosymtab(part, NAME_DYNAMIC, STB_GLOBAL, STT_OBJECT, SHN_ABS);
	    break;
	}
    }
    part->done = true;
    return true;
}

/* Set the value of the _DYNAMIC symbol to point to the address of the
 * dynamic section.
 */
static bool dyncomplete(elfpart *part, blueprint const *bp)
{
    foreachpart (p0 in bp) {
        if (p0->shtype == SHT_DYNAMIC) {
            setsymvalue(part, NAME_DYNAMIC, p0->addr);
            break;
        }
    }
    return symcomplete(part, bp);
}

/* Adds a symbol to a symbol table. The symbol's value is initialized
 * to zero; the other data associated with the symbol are supplied by
 * the function's arguments. The symbol's name is automatically added
 * to the appropriate string table. The return value is an index of
 * the symbol in the table (see below for comments on the index
 * value).
 */
int addtosymtab(elfpart *part, char const *str, int bind, int type, int shndx)
{
    Elf64_Sym *sym;
    int n;

    if (!_validate(part->shtype == SHT_SYMTAB || part->shtype == SHT_DYNSYM,
                   "not a symbol table"))
        return 0;
    if (!_validate(part->link && part->link->shtype == SHT_STRTAB,
                   "symbol table has no string table"))
        return 0;

    sym = _resizepart(part, part->size + part->entsize);
    if (!sym)
        return 0;
    if (bind == STB_LOCAL) {
	sym += part->info;
	if (part->info < part->count)
	    memmove(sym + 1, sym, (part->count - part->info) * sizeof *sym);
	++part->count;
	n = part->info++;
    } else {
	sym += part->count++;
	n = part->info - part->count;
    }
    sym->st_name = addtostrtab(part->link, str);
    sym->st_value = 0;
    sym->st_size = 0;
    sym->st_info = ELF64_ST_INFO(bind, type);
    sym->st_other = 0;
    sym->st_shndx = shndx;
    return n;
}

/* Look up a symbol already in a symbol table by name. The return
 * value is the index of the symbol, or zero if the symbol is not in
 * the table. The index will be negative for local symbols, since the
 * true index cannot be determined until all the global symbols have
 * been added. After fill() has been called, a negative index can be
 * converted to a real index by subtracting it from the value in the
 * symbol table's info field, less one.
 */
int getsymindex(elfpart const *part, char const *name)
{
    Elf64_Sym *sym;
    char const *strtab;
    int i;

    if (!_validate(part->shtype == SHT_SYMTAB || part->shtype == SHT_DYNSYM,
                   "not a symbol table"))
        return 0;
    if (!part->link || !part->link->part)
        return 0;

    strtab = part->link->part;
    sym = part->part;
    for (i = 1, ++sym ; i < part->count ; ++i, ++sym) {
	if (!strcmp(name, strtab + sym->st_name)) {
	    if (i >= part->info)
		return part->info - i - 1;
	    else
		return i;
	}
    }
    return 0;
}

/* Sets the value of a symbol in a symbol table. The return value is
 * false if the given symbol could not be found in the table.
 */
bool setsymvalue(elfpart *part, char const *name, intptr_t value)
{
    Elf64_Sym *sym;
    char const *strtab;
    int i;

    if (!_validate(part->shtype == SHT_SYMTAB || part->shtype == SHT_DYNSYM,
                   "not a symbol table"))
        return false;
    if (!part->link || !part->link->part)
        return false;

    strtab = part->link->part;
    sym = part->part;
    for (i = 1, ++sym ; i < part->count ; ++i, ++sym) {
	if (!strcmp(name, strtab + sym->st_name)) {
	    sym->st_value = value;
	    return true;
	}
    }
    return false;
}

/* The symbol table elfpart structures.
 */
elfpart part_symtab = { symnew, NULL, NULL, symcomplete };
elfpart part_dynsym = { dynnew, dyninit, NULL, dyncomplete };
