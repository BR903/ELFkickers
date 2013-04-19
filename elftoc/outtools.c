/* outtools.c: The higher-level output functions.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	"gen.h"
#include	"elf.h"
#include	"elftoc.h"
#include	"pieces.h"
#include	"addr.h"
#include	"shdrtab.h"
#include	"outbasic.h"
#include	"outtools.h"

/* Finds a representation of a integer value that uses, if possible, a
 * macro definition. r points to a set of count macro definitions. If
 * none of the ranges associated with the macro symbols includes the
 * given value, the number is returned as a hexadecimal constant.
 */
char const *_getname(unsigned int item, names const *r, int count)
{
    static char	buf[64];
    int		i;

    for (i = 0 ; i < count ; ++i) {
	if (item == r[i].base)
	    return r[i].name;
	else if (item > r[i].base && item <= r[i].max) {
	    sprintf(buf, "%s + %lu", r[i].name, item - r[i].base);
	    return buf;
	}
    }
    sprintf(buf, "0x%X", item);
    return buf;
}

/* Finds a representation of a integer value that uses a bitwise-oring
 * of as many macro symbols as possible. f points to a set of count
 * macro definitions, each assumed to have one bit set. Any bits in
 * the value not included in the given symbols are returned as a
 * hexadecimal constant.
 */
char const *_getflags(unsigned int item, names const *f, int count)
{
    static char		buf[1024];
    unsigned int	val;
    int			i;

    if (!item)
	return "0";
    val = item;
    *buf = '\0';
    for (i = 0 ; i < count ; ++i) {
	if ((item & f[i].base) && !(item & f[i].max)) {
	    val &= ~f[i].base;
	    if (*buf)
		strcat(buf, " | ");
	    strcat(buf, f[i].name);
	}
    }
    if (val)
	sprintf(buf + strlen(buf), "%s0x%X", (*buf ? " | " : ""), val);
    return buf;
}


/* Returns a string representation of a file offset value that, if
 * possible, is expressed as an offset into the C structure. If pindex
 * is not NULL, then its referenced value will be set to the index of
 * the file piece that is found to match the offset, or -1 if no such
 * piece exists.
 */
char const *getoffstr(int offset, int *pindex)
{
    static char	buf[128];
    int		i;

    if (!offset) {
	strcpy(buf, "0");
	i = 0;
    } else if (offset == pieces[piecenum - 1].to) {
	sprintf(buf, "sizeof(%s)", structname);
	i = -1;
    } else if (offset > pieces[piecenum - 1].to) {
	sprintf(buf, "sizeof(%s) + 0x%X",
		     structname, offset - pieces[piecenum - 1].to);
	i = -1;
    } else {
	for (i = 0 ; i < piecenum ; ++i) {
	    if (offset == pieces[i].from) {
		sprintf(buf, "offsetof(%s, %s)", structname, pieces[i].name);
		break;
	    } else if (offset < pieces[i].to) {
		sprintf(buf, "offsetof(%s, %s) + 0x%X",
			      structname, pieces[i].name,
			      offset - pieces[i].from);
		i = -1;
		break;
	    }
	}
    }
    if (pindex)
	*pindex = i;
    return buf;
}

/* Finds string representations of an offset and a memory address, and
 * also returns a piece index or -1, as above. The offset and the
 * address are assumed to be related, ideally with the offset also
 * being the offset of the address into one of the recorded memory
 * segments. Either of the string pointer arguments can be NULL if
 * the associated string representation is not needed.
 */
int getmemstrs(Elf32_Addr addr, Elf32_Off offset,
	       char const **paddrstr, char const **poffstr)
{
    static char		buf[256];
    char const	       *addrstr = NULL;
    char const	       *offstr;
    addrinfo const     *base;
    unsigned int	diff;
    int			index, n;

    offstr = getoffstr(offset, &index);
    if (!addr)
	addrstr = "0";
    else if (index > 0 && addr == offset)
	addrstr = offstr;
    else if ((base = getbaseaddr(addr))) {
	if (addr == base->addr)
	    addrstr = base->name;
	else {
	    addrstr = buf;
	    diff = addr - base->addr;
	    if (diff == offset) {
		if (base->addr)
		    sprintf(buf, "%s + %s", base->name, offstr);
		else
		    addrstr = offstr;
	    } else {
		n = 0;
		if (base->addr)
		    n += sprintf(buf + n, "%s + ", base->name);
		if (index >= 0 && diff > offset) {
		    diff -= pieces[index].from;
		    n += sprintf(buf + n, "%s + ", offstr);
		}
		sprintf(buf + n, "0x%X", diff);
	    }
	}
    } else if (index > 0 && (int)addr > pieces[index].from
			 && (int)addr < pieces[index].to) {
	addrstr = buf;
	sprintf(buf, "%s + 0x%X", offstr,
				  (unsigned)(addr - pieces[index].from));
    }

    if (paddrstr) {
	if (!addrstr) {
	    sprintf(buf, "0x%X", (unsigned)addr);
	    addrstr = buf;
	}
	*paddrstr = addrstr;
    }
    if (poffstr)
	*poffstr = offstr;
    return index;
}

/* Returns a string representation of a memory address. This function
 * is similar to the previous one, but is used when no related file
 * offset value is present. If pindex is not NULL, then its referenced
 * value will be set to a piece index or -1, as above.
 */
char const *getaddrstr(Elf32_Addr addr, int *pindex)
{
    addrinfo const     *base;
    char const	       *addrstr;
    int			index;

    if (!addr)
	return "0";
    base = getbaseaddr(addr);
    if (base) {
	if (addr == base->addr)
	    return base->name;
	index = getmemstrs(addr, addr - base->addr, &addrstr, NULL);
    } else {
	addrstr = getoffstr(addr, &index);
	if (index < 0)
	    index = getmemstrs(addr, 0, &addrstr, NULL);
    }
    if (pindex)
	*pindex = index;
    return addrstr;
}

/* Returns a string representation of a size value. If index is greater
 * than -1, it supplies a piece index that is believed to be associated
 * with the given size (usually which has been previously supplied by
 * the getoffstr function).
 */
char const *getsizestr(int size, int index)
{
    static char	buf[256];
    pieceinfo  *piece = pieces + index;
    int		i;

    if (index < 0 || !size) {
	sprintf(buf, "%d", size);
	return buf;
    }

    piece = pieces + index;

    if (size == piece->size) {
	sprintf(buf, "sizeof %s.%s", varname, piece->name);
	return buf;
    } else if (size > piece->size) {
	if (size == pieces[piecenum - 1].to) {
	    sprintf(buf, "sizeof(%s)", structname);
	    return buf;
	} else if (size > pieces[piecenum - 1].to) {
	    sprintf(buf, "sizeof(%s) + %d",
		    structname, size - pieces[piecenum - 1].to);
	    return buf;
	} else if (size == pieces[piecenum - 1].to - piece->from) {
	    sprintf(buf, "sizeof(%s) - offsetof(%s, %s)",
		    structname, structname, piece->name);
	    return buf;
	} else if (size > pieces[piecenum - 1].to - piece->from) {
	    sprintf(buf, "%d", size);
	    return buf;
	}
    } else {
	if (ctypes[piece->type].size > 1) {
	    if (size == ctypes[piece->type].size) {
		sprintf(buf, "sizeof %s.%s[0]", varname, piece->name);
		return buf;
	    } else if (size % ctypes[piece->type].size == 0) {
		sprintf(buf, "sizeof %s.%s[0] * %u", varname, piece->name,
					size / ctypes[piece->type].size);
		return buf;
	    }
	}
	if (piece->size % size == 0) {
	    sprintf(buf, "sizeof %s.%s / %u", varname, piece->name,
					      piece->size / size);
	    return buf;
	}
	i = piece->size - size;
	if (i * 8 < piece->size) {
	    sprintf(buf, "sizeof %s.%s - %d", varname, piece->name, i);
	    return buf;
	}
	sprintf(buf, "%d", size);
	return buf;
    }


    for (i = index + 1 ; i < piecenum ; ++i) {
	if (size == pieces[i].from - piece->from) {
	    if (piece->from)
		sprintf(buf, "offsetof(%s, %s) - offsetof(%s, %s)",
			     structname, pieces[i].name,
			     structname, piece->name);
	    else
		sprintf(buf, "offsetof(%s, %s)", structname, pieces[i].name);
	    return buf;
	} else if (size < pieces[i].from - piece->from)
	    break;
    }

    sprintf(buf, "sizeof %s.%s + 0x%02X", varname, piece->name,
					  size - piece->size);
    return buf;
}
