/* progbits.c: parts containing segments of the program.
 *
 * Copyright (C) 1999,2001,2021 by Brian Raiter, under the GNU General
 * Public License, version 2. No warranty. See COPYING for details.
 */

#include <stdlib.h>
#include <elf.h>
#include "elfparts.h"
#include "elfpartsi.h"

/* Generic setup of the elfpart structure.
 */
static bool prognew(elfpart *part)
{
    part->shtype = SHT_PROGBITS;
    return true;
}

/* Set up the elfpart structure for a text segment.
 */
static bool textnew(elfpart *part)
{
    part->shtype = SHT_PROGBITS;
    part->flags = PF_R | PF_X;
    part->shname = ".text";
    return true;
}

/* Set up the elfpart structure for a data segment.
 */
static bool datanew(elfpart *part)
{
    part->shtype = SHT_PROGBITS;
    part->flags = PF_R | PF_W;
    part->shname = ".data";
    return true;
}

/* Set up the elfpart structure for a read-only data segment.
 */
static bool rodatanew(elfpart *part)
{
    part->shtype = SHT_PROGBITS;
    part->flags = PF_R;
    part->shname = ".rodata";
    return true;
}

/* The progbits elfpart structures.
 */
elfpart part_progbits = { prognew, NULL, NULL, NULL };
elfpart part_text = { textnew, NULL, NULL, NULL };
elfpart part_data = { datanew, NULL, NULL, NULL };
elfpart part_rodata = { rodatanew, NULL, NULL, NULL };
