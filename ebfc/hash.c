/* hash.c: part containing a hash table.
 *
 * Copyright (C) 1999,2001,2021 by Brian Raiter, under the GNU General
 * Public License, version 2. No warranty. See COPYING for details.
 */

#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include "elfparts.h"
#include "elfpartsi.h"

/* Set up the elfpart structure.
 */
static bool new(elfpart *part)
{
    part->shtype = SHT_HASH;
    part->shname = ".hash";
    part->flags = PF_R;
    part->entsize = sizeof(Elf64_Word);
    return true;
}

/* Select an efficient size for the hash table and allocate the
 * necessary memory.
 */
static bool fill(elfpart *part, blueprint const *bp)
{
    static Elf64_Word const buckets[] = {
	1, 1, 3, 17, 37, 67, 97, 131, 197, 263, 521, 1031, 2053,
	4099, 8209, 16411, 32771
    };

    Elf64_Word *hash;
    Elf64_Word symnum;
    int i;

    if (!part->link->done)
	return true;
    if (!_validate(part->link, "hash table has no symbol table set"))
        return false;

    symnum = part->link->count;
    for (i = 1 ; i < (int)(sizeof buckets / sizeof *buckets) ; ++i)
	if (buckets[i] > symnum)
	    break;
    --i;
    part->count = symnum + buckets[i] + 2;
    hash = _resizepart(part, part->count * part->entsize);
    if (!hash)
        return false;
    hash[0] = buckets[i];
    hash[1] = symnum;
    memset(hash + 2, 0, (buckets[i] + symnum) * sizeof *hash);

    part->done = true;
    return true;
    (void)bp;
}

/* Fill the table with pointers to the symbol table entries.
 */
static bool complete(elfpart *part, blueprint const *bp)
{
    Elf64_Word *chain;
    Elf64_Sym *sym;
    unsigned char const *strtab;
    unsigned char const *name;
    Elf64_Word bucketnum;
    Elf64_Word hash;
    Elf64_Sword n;
    int i;

    if (!part->link->done)
	return true;

    sym = part->link->part;
    strtab = part->link->link->part;
    chain = part->part;
    bucketnum = chain[0];
    chain += 2 + bucketnum;
    for (i = 1, ++sym ; i < part->link->count ; ++i, ++sym) {
	name = strtab + sym->st_name;
	for (hash = 0 ; *name ; ++name) {
	    hash = (hash << 4) + *name;
	    hash = (hash ^ ((hash & 0xF0000000) >> 24)) & 0x0FFFFFFF;
	}
	hash = hash % bucketnum;
	n = hash - bucketnum;
	while (chain[n])
	    n = chain[n];
	chain[n] = i;
    }

    part->done = true;
    return true;
    (void)bp;
}

/* The hash elfpart structure.
 */
elfpart part_hash = { new, NULL, fill, complete };
