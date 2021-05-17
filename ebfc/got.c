/* got.c: part containing a global offset table.
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
    part->shtype = SHT_PROGBITS;
    part->shname = ".got";
    part->flags = PF_R | PF_W;
    part->entsize = sizeof(Elf64_Addr);
    return true;
}

/* Add the three required entries to the GOT, and add the GOT to
 * the dynamic symbol table (if present).
 */
static bool init(elfpart *part, blueprint const *bp)
{
    Elf64_Addr *got;

    part->count = 3;
    got = _resizepart(part, part->count * part->entsize);
    if (!got)
        return false;
    got[0] = 0;
    got[1] = 0;
    got[2] = 0;
    foreachpart (p0 in bp) {
	if (p0->shtype == SHT_DYNSYM) {
	    addtosymtab(p0, NAME_GOT, STB_GLOBAL, STT_OBJECT, SHN_ABS);
	    break;
	}
    }
    part->done = true;
    return true;
}

/* Set the first entry in the GOT to point to the .dynamic section,
 * and set the address of the GOT in the dynamic symbol table.
 */
static bool complete(elfpart *part, blueprint const *bp)
{
    foreachpart (p0 in bp) {
        if (p0->shtype == SHT_DYNAMIC)
	    ((Elf64_Addr*)part->part)[0] = p0->addr;
	else if (p0->shtype == SHT_DYNSYM)
	    setsymvalue(p0, NAME_GOT, part->addr);
    }
    part->done = true;
    return true;
}

/* The GOT elfpart structure.
 */
elfpart part_got = { new, init, NULL, complete };
