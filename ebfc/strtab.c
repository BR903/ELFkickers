/* strtab.c: parts containing a string table.
 *
 * Copyright (C) 1999,2001,2021 by Brian Raiter, under the GNU General
 * Public License, version 2. No warranty. See COPYING for details.
 */

#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include "elfparts.h"
#include "elfpartsi.h"

/* Set up the elfpart structure and allocate an empty string table.
 */
static bool strnew(elfpart *part)
{
    part->shtype = SHT_STRTAB;
    part->shname = ".strtab";
    _resizepart(part, 1);
    *(char*)part->part = '\0';
    return true;
}

/* Set up the elfpart structure for a section header string table.
 */
static bool shnew(elfpart *part)
{
    strnew(part);
    part->shname = ".shstrtab";
    return true;
}

/* Set up the elfpart structure for a dynamic string table.
 */
static bool dynnew(elfpart *part)
{
    strnew(part);
    part->shname = ".dynstr";
    part->flags = PF_R;
    return true;
}

/* Adds a string to a string table. The return value is the index
 * of the string in the table.
 */
int addtostrtab(elfpart *part, char const *str)
{
    char *strtab;
    size_t pos, len;

    if (!_validate(part->shtype == SHT_STRTAB, "not a string table"))
        return -1;

    if (!str || !*str)
	return 0;
    len = strlen(str) + 1;
    pos = part->size;
    strtab = _resizepart(part, part->size + len);
    if (!strtab)
        return -1;
    memcpy(strtab + pos, str, len);
    return pos;
}

/* The string table elfpart structures.
 */
elfpart part_strtab   = { strnew, NULL, NULL, NULL };
elfpart part_dynstr   = { dynnew, NULL, NULL, NULL };
elfpart part_shstrtab = { shnew, NULL, NULL, NULL };
