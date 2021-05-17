/* shdrtab.c: part containing the section header table.
 *
 * Copyright (C) 1999,2001,2021 by Brian Raiter, under the GNU General
 * Public License, version 2. No warranty. See COPYING for details.
 */

#include <stdlib.h>
#include <elf.h>
#include "elfparts.h"
#include "elfpartsi.h"

/* Set up the elfpart structure.
 */
static bool new(elfpart *part)
{
    part->shtype = SHT_OTHER;
    part->entsize = sizeof(Elf64_Shdr);
    return true;
}

/* Create a blank section header table with enough entries for all the
 * appropriate parts in the blueprint.
 */
static bool init(elfpart *part, blueprint const *bp)
{
    static Elf64_Shdr const blankshdr = { .sh_type = SHT_NULL };
    Elf64_Shdr *shdr;
    int i;

    foreachpart (p0 in bp)
        if (_partissection(p0) && !p0->done)
            return true;

    _getelfhdr(bp)->e_shoff = (Elf64_Off)(intptr_t)part;
    part->count = 1;
    foreachpart (p0 in bp)
        if (_partissection(p0))
            ++part->count;

    shdr = _resizepart(part, part->count * part->entsize);
    if (!shdr)
        return false;
    for (i = 0 ; i < part->count ; ++i)
	shdr[i] = blankshdr;

    part->done = true;
    return true;
}

/* Add the section names to the section header string table, if one is
 * present.
 */
static bool fill(elfpart *part, blueprint const *bp)
{
    Elf64_Shdr *shdr = part->part;
    int n;

    if (part->link) {
        n = 1;
        foreachpart (p0 in bp) {
            if (p0->shtype <= 0)
                continue;
	    if (p0->shname)
		shdr[n].sh_name = addtostrtab(part->link, p0->shname);
	    if (p0 == part->link)
                _getelfhdr(bp)->e_shstrndx = n;
	    ++n;
	}
	part->link = NULL;
        if (part->count != n) {
            part->count = n;
            _resizepart(part, part->count * part->entsize);
        }
    }

    part->done = true;
    return true;
}

/* Fill in the section header table, using the information from the
 * parts list in the blueprint.
 */
static bool complete(elfpart *part, blueprint const *bp)
{
    Elf64_Shdr *shdr;
    int n;

    foreachpart (p0 in bp)
        if (_partissection(p0) && !p0->done)
            return true;

    shdr = part->part;
    foreachpart(p0 in bp) {
	if (p0->shtype <= 0)
	    continue;
	++shdr;
	shdr->sh_type = p0->shtype;
	shdr->sh_offset = p0->offset;
	shdr->sh_size = p0->size;
	shdr->sh_addr = p0->addr;
	shdr->sh_entsize = p0->entsize;
	if (p0->flags) {
	    shdr->sh_addralign = sizeof(Elf64_Addr);
	    shdr->sh_flags = SHF_ALLOC;
	    if (p0->flags & PF_W)
		shdr->sh_flags |= SHF_WRITE;
	    if (p0->flags & PF_X) {
		shdr->sh_flags |= SHF_EXECINSTR;
		shdr->sh_addralign = 16;
	    }
	} else if (shdr->sh_entsize) {
	    shdr->sh_addralign = sizeof(Elf64_Addr);
        }
	if (p0->info)
	    shdr->sh_info = p0->info;
	if (p0->link) {
	    shdr->sh_link = 1;
	    for (n = 0 ; n < bp->partcount ; ++n) {
		if (bp->parts + n == p0->link)
		    break;
                if (_partissection(&bp->parts[n]))
		    ++shdr->sh_link;
	    }
	}
    }

    part->done = true;
    return true;
}

/* The section header table elfpart structure.
 */
elfpart part_shdrtab = { new, init, fill, complete };
