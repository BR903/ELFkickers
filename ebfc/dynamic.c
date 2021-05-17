/* dynamic.c: part containing a .dynamic section.
 *
 * Copyright (C) 1999,2001,2021 by Brian Raiter, under the GNU General
 * Public License, version 2. No warranty. See COPYING for details.
 */

#include <stdlib.h>
#include <elf.h>
#include "elfparts.h"
#include "elfpartsi.h"

/* Set the value of an entry in a dynamic section. The entry is added
 * to the section if it is not already present. The return value is
 * the index of the entry.
 */
static int setdynvalue(elfpart *part, int tag, intptr_t value)
{
    Elf64_Dyn *dyn;
    int n;

    dyn = part->part;
    for (n = 0 ; n < part->count - 1 ; ++n) {
	if (dyn[n].d_tag == tag) {
            dyn[n].d_un.d_val = value;
            return n;
        }
    }
    return -1;
}

/* Set up the elfpart structure for a dynamic section.
 */
static bool new(elfpart *part)
{
    part->shtype = SHT_DYNAMIC;
    part->shname = ".dynamic";
    part->flags = PF_R | PF_W;
    part->entsize = sizeof(Elf64_Dyn);
    return true;
}

/* Initialize the dynamic section with the minimally required entries.
 */
static bool init(elfpart *part, blueprint const *bp)
{
    Elf64_Dyn *dyn;
    int i;

    part->count = 6;
    dyn = _resizepart(part, part->count * part->entsize);
    if (!dyn)
        return false;
    dyn[0].d_tag = DT_HASH;
    dyn[1].d_tag = DT_SYMTAB;
    dyn[2].d_tag = DT_SYMENT;
    dyn[3].d_tag = DT_STRTAB;
    dyn[4].d_tag = DT_STRSZ;
    dyn[5].d_tag = DT_NULL;
    for (i = 0 ; i < 6 ; ++i)
        dyn[i].d_un.d_val = 0;

    part->done = true;
    return true;
    (void)bp;
}

/* Fill in the values for the required entries.
 */
static bool complete(elfpart *part, blueprint const *bp)
{
    elfpart *other;

    other = NULL;
    foreachpart (p0 in bp) {
	if (p0->shtype == SHT_HASH) {
            other = p0;
	    break;
        }
    }
    if (other) {
        setdynvalue(part, DT_HASH, other->addr);
        other = other->link;
        if (other) {
            setdynvalue(part, DT_SYMTAB, other->addr);
            setdynvalue(part, DT_SYMENT, other->entsize);
            other = other->link;
            if (other) {
                setdynvalue(part, DT_STRTAB, other->addr);
                setdynvalue(part, DT_STRSZ, other->size);
            }
        }
    }

    part->done = true;
    return true;
}

/* The dynamic elfpart structure.
 */
elfpart part_dynamic = { new, init, NULL, complete };
