/* addr.c: Functions for identifying and handling memory addresses.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	"gen.h"
#include	"addr.h"

/* The collection of memory segments.
 */
addrinfo       *addrs = NULL;
int		addrnum = 0;

/* Adds an address to the collection. addr is the address, offset is
 * the (presumed) offset within the memory segment, size is the size
 * of the chunk of memory starting at that address, and name is the
 * name associated with that chunk. The largest chunk in that memory
 * address will determine which name is used to name the memory
 * segment.
 */
void recordaddr(unsigned int addr, int offset, int size, char const *name)
{
    static int		alloced = 0;
    unsigned int	base;
    int			i;

    base = addr - offset;
    for (i = 0 ; i < addrnum ; ++i)
	if (addrs[i].addr == base)
	    break;
    if (i == addrnum) {
	if (addrnum == alloced) {
	    alloced += 8;
	    xalloc(addrs, alloced * sizeof *addrs);
	    memset(addrs + alloced - 8, 0, 8 * sizeof *addrs);
	}
	++addrnum;
    }
    addrs[i].addr = base;
    ++addrs[i].count;
    if (!*addrs[i].name || addrs[i].size < size) {
	strcpy(addrs[i].name, name);
	addrs[i].size = size;
    }
    if (!addrs[i].from || addrs[i].from > addr)
	addrs[i].from = addr;
    if (addrs[i].to < addr + size)
	addrs[i].to = addr + size;
}

/* Weeds through the collection of memory segments and pick out the
 * ones that are likely to be real (i.e., the ones that are used in
 * more than one place and/or begin on a page boundary). These
 * segments are then assigned unique names.
 */
void setaddrnames(void)
{
    char       *str;
    int		i, j, n;

    i = 0;
    while (i < addrnum) {
	if (addrs[i].count == 1 && (addrs[i].addr & 0x0FFF)) {
	    --addrnum;
	    addrs[i] = addrs[addrnum];
	} else
	    ++i;
    }

    for (i = 0 ; i < addrnum ; ++i) {
	str = addrs[i].name;
	while (!isalnum(*str))
	    ++str;
	memmove(addrs[i].name + 5, str, strlen(str));
	memcpy(addrs[i].name, "ADDR_", 5);
	for (str = addrs[i].name + 5 ; *str ; ++str)
	    *str = isalnum(*str) ? toupper(*str) : '_';
    }

    for (i = 0 ; i < addrnum - 1 ; ++i) {
	n = 1;
	for (j = i + 1 ; j < addrnum ; ++j)
	    if (!strcmp(addrs[j].name, addrs[i].name))
		sprintf(addrs[j].name, "%s%d", addrs[i].name, ++n);
	if (n > 1)
	    strcat(addrs[i].name, "1");
    }
}

/* Given an address, determines which memory segment it belongs to and
 * return the information for that segment. If no match is found, NULL
 * is returned.
 */
addrinfo const *getbaseaddr(unsigned int addr)
{
    addrinfo const     *bestaddr;
    int			i;

    bestaddr = NULL;
    for (i = 0 ; i < addrnum ; ++i) {
	if ((!bestaddr || addrs[i].addr > bestaddr->addr)
		&& (addr >= addrs[i].from && addr < addrs[i].to))
	    bestaddr = addrs + i;
    }
    if (!bestaddr) {
	for (i = 0 ; i < addrnum ; ++i) {
	    if (addr == addrs[i].to) {
		bestaddr = addrs + i;
		break;
	    }
	}
    }
    return bestaddr;
}
