/* ehdr.c: part containing the ELF header.
 *
 * Copyright (C) 1999,2001,2021 by Brian Raiter, under the GNU General
 * Public License, version 2. No warranty. See COPYING for details.
 */

#include <stdlib.h>
#include <elf.h>
#include "elfparts.h"
#include "elfpartsi.h"

/* Set up the elfpart structure, and create an empty ELF header.
 */
static bool new(elfpart *part)
{
    static Elf64_Ehdr const blankehdr = {
	{ 0x7F, 'E', 'L', 'F',
          ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_NONE },
	0, 0, EV_CURRENT, 0, 0, 0, 0, sizeof(Elf64_Ehdr), 0, 0, 0, 0, 0
    };

    part->shtype = SHT_OTHER;
    part->flags = PF_R;
    if (!_resizepart(part, sizeof(Elf64_Ehdr)))
        return false;
    *(Elf64_Ehdr*)part->part = blankehdr;
    return true;
}

/* Fill in the ELF file type.
 */
static bool init(elfpart *part, blueprint const *bp)
{
    ((Elf64_Ehdr*)part->part)->e_type = (Elf64_Half)bp->filetype;
    part->done = true;
    return true;
}

/* Fill in the information for the program and section header tables.
 */
static bool complete(elfpart *part, blueprint const *bp)
{
    Elf64_Ehdr *ehdr = part->part;
    elfpart *p0;

    if (ehdr->e_phoff) {
	p0 = (elfpart*)(intptr_t)ehdr->e_phoff;
	ehdr->e_phoff = p0->offset;
	ehdr->e_phnum = p0->count;
	ehdr->e_phentsize = p0->entsize;
    }
    if (ehdr->e_shoff) {
	p0 = (elfpart*)(intptr_t)ehdr->e_shoff;
	ehdr->e_shoff = p0->offset;
	ehdr->e_shnum = p0->count;
	ehdr->e_shentsize = p0->entsize;
    }

    if (!ehdr->e_machine)
	ehdr->e_machine = EM_X86_64;

    part->done = true;
    return true;
    (void)bp;
}

elfpart part_ehdr = { new, init, NULL, complete };
