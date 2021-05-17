/* phdrtab.c: part containing the program header table.
 *
 * Copyright (C) 1999,2001,2021 by Brian Raiter, under the GNU General
 * Public License, version 2. No warranty. See COPYING for details.
 */

#include <stdlib.h>
#include <elf.h>
#include "elfparts.h"
#include "elfpartsi.h"

/* The different possible entries in the program header table.
 */
enum { PH_TEXT = 0, PH_DATA, PH_DYNAMIC, PH_COUNT };

/* Set up the elfpart structure.
 */
static bool new(elfpart *part)
{
    part->shtype = SHT_OTHER;
    part->flags = PF_R;
    part->entsize = sizeof(Elf64_Phdr);
    return true;
}

/* Record the existence of the program header table in the ELF header.
 */
static bool init(elfpart *part, blueprint const *bp)
{
    _getelfhdr(bp)->e_phoff = (Elf64_Off)(intptr_t)part;
    part->done = true;
    return true;
}

/* Add entries in the program header table for the text segment, the
 * data segment, and (if present) the dynamic section.
 */
static bool fill(elfpart *part, blueprint const *bp)
{
    static Elf64_Phdr const blankphdr = { .p_type = PT_NULL };
    Elf64_Phdr *phdr;
    int dynid;

    part->count = PH_COUNT;
    for (dynid = bp->partcount - 1 ; dynid > 0 ; --dynid)
	if (bp->parts[dynid].shtype == SHT_DYNAMIC)
	    break;
    if (!dynid)
	--part->count;
    phdr = _resizepart(part, part->count * part->entsize);
    if (!phdr)
        return false;

    phdr[PH_TEXT] = blankphdr;
    phdr[PH_TEXT].p_type = PT_LOAD;
    phdr[PH_TEXT].p_flags = PF_R | PF_X;
    phdr[PH_TEXT].p_align = 0x1000;

    phdr[PH_DATA] = blankphdr;
    phdr[PH_DATA].p_type = PT_LOAD;
    phdr[PH_DATA].p_flags = PF_R | PF_W;
    phdr[PH_DATA].p_align = 0x1000;

    if (dynid) {
	phdr[PH_DYNAMIC] = blankphdr;
	phdr[PH_DYNAMIC].p_type = PT_DYNAMIC;
	phdr[PH_DYNAMIC].p_flags = PF_R | PF_W;
	phdr[PH_DYNAMIC].p_align = sizeof(Elf64_Addr);
	phdr[PH_DYNAMIC].p_offset = (Elf64_Off)(intptr_t)(bp->parts + dynid);
    }

    part->done = true;
    return true;
}

/* Fill in all the necessary information in the program header table.
 */
static bool complete(elfpart *part, blueprint const *bp)
{
    Elf64_Phdr *phdr = part->part;
    elfpart *p0;
    int i, n;

    phdr[PH_TEXT].p_offset = phdr[PH_DATA].p_offset = ~0;
    phdr[PH_TEXT].p_memsz = phdr[PH_DATA].p_memsz = 0;
    for (i = 0, p0 = bp->parts ; i < bp->partcount ; ++i, ++p0) {
	if (!(p0->flags & PF_R))
	    continue;
	n = p0->flags & PF_W ? PH_DATA : PH_TEXT;
	if (phdr[n].p_offset > p0->offset) {
	    phdr[n].p_offset = p0->offset;
	    phdr[n].p_vaddr = p0->addr;
	}
	if (phdr[n].p_memsz < p0->offset + p0->size)
	    phdr[n].p_memsz = p0->offset + p0->size;
    }
    phdr[PH_TEXT].p_memsz -= phdr[PH_TEXT].p_offset;
    phdr[PH_DATA].p_memsz -= phdr[PH_DATA].p_offset;

    if (part->count > PH_DYNAMIC) {
	p0 = (elfpart*)(intptr_t)phdr[PH_DYNAMIC].p_offset;
	phdr[PH_DYNAMIC].p_offset = p0->offset;
	phdr[PH_DYNAMIC].p_vaddr = p0->addr;
	phdr[PH_DYNAMIC].p_memsz = p0->size;
    }

    for (i = 0 ; i < part->count ; ++i) {
	phdr[i].p_paddr = phdr[i].p_vaddr;
	phdr[i].p_filesz = phdr[i].p_memsz;
    }

    part->done = true;
    return true;
}

/* The program header table elfpart structure.
 */
elfpart part_phdrtab = { new, init, fill, complete };
