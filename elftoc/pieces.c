/* pieces.c: Functions for handling the file as a sequence of pieces.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	"gen.h"
#include	"elf.h"
#include	"pieces.h"

/* The collection of file pieces.
 */
pieceinfo      *pieces = NULL;
int		piecenum = 0;

#define	_(str)		#str, sizeof(str)

/* The C types that go with each type of piece.
 */
typeinfo const ctypes[P_COUNT] = {
    { _(unsigned char) },		/* P_UNCLAIMED */
    { _(unsigned char) },		/* P_BYTES */
    { _(unsigned char) },		/* P_SECTION */
    { _(Elf32_Half) },			/* P_HALVES */
    { _(Elf32_Word) },			/* P_WORDS */
    { _(Elf32_Word) },			/* P_NOTE */
    { _(Elf32_Word) },			/* P_HASH */
    { _(Elf32_Sym) },			/* P_SYMTAB */
    { _(Elf32_Rel) },			/* P_REL */
    { _(Elf32_Rela) },			/* P_RELA */
    { _(Elf32_Dyn) },			/* P_DYNAMIC */
    { _(Elf32_Shdr) },			/* P_SHDRTAB */
    { _(Elf32_Phdr) },			/* P_PHDRTAB */
    { _(Elf32_Ehdr) },			/* P_EHDR */
    { _(char) }				/* P_NONEXISTENT */
};

#undef _

/* Records an area of the file as being a unique piece. offset and
 * size identify the area of the file, type specifies the type of the
 * contents at that piece, shndx specifies a section header table
 * entry associated with that piece, if any, and name supplies a
 * suggested name for that piece, to be used as the field name for the
 * structure definition in the generated C code. A tilde prefixed to
 * the name indicates that the suggestion is a weak one, and other
 * suggestions should take precedence.
 */
void recordpiece(int offset, int size, int type, int shndx, char const *name)
{
    static int	piecealloc = 0;
    int const	piecechunk = 16;

    if (!size)
	return;
    if (piecenum == piecealloc) {
	piecealloc += piecechunk;
	xalloc(pieces, piecealloc * sizeof *pieces);
    }
    pieces[piecenum].from = offset;
    pieces[piecenum].to = offset + size;
    pieces[piecenum].shndx = shndx;
    if (name) {
	strncpy(pieces[piecenum].name, name, sizeof pieces->name);
	pieces[piecenum].name[sizeof pieces->name - 1] = '\0';
    } else
	strcpy(pieces[piecenum].name, "~segment");
    pieces[piecenum].type = type;
    pieces[piecenum].warn = FALSE;
    ++piecenum;
}

/* An ordering function for qsort, arranging the pieces by their
 * starting position.
 */
static int piececmp(const void *p1, const void *p2)
{
    int n;

    n = ((pieceinfo*)p1)->from - ((pieceinfo*)p2)->from;
    if (!n)
	n = ((pieceinfo*)p2)->to - ((pieceinfo*)p1)->to;
    return n;
}

/* Arranges the collection of pieces in order of appearance within the
 * file, and removes any redundancies or overlapping areas. (If two
 * pieces overlap, then the one with the type of higher precedence
 * will be used for overlapping area. The end results is that the
 * collection of pieces will be mutually exclusive and (presuming that
 * every byte in the file has been assigned to at least one piece)
 * collectively exhaustive. Finally, the function adds, if necessary,
 * a trailing field to account for padding in the C structure. (If
 * this enforced padding were not explicitly accounted for, the rest
 * of the program would have an incorrect size for the structure.)
 */
int arrangepieces(void)
{
    pieceinfo  *p1, *p2;
    int		i, j;

    qsort(pieces, piecenum, sizeof *pieces, piececmp);

    for (;;) {
	for (i = 0, p1 = pieces ; i < piecenum - 1 ; ++i, ++p1) {
	    for (j = i + 1, p2 = p1 + 1 ; j < piecenum ; ++j, ++p2)
		if (p1->to > p2->from)
		    break;
	    if (j < piecenum)
		break;
	}
	if (i >= piecenum - 1)
	    break;

	if (p1->from == p2->from) {
	    if (!p1->shndx)
		p1->shndx = p2->shndx;
	    else if (!p2->shndx)
		p2->shndx = p1->shndx;
	}
	if (p1->type == p2->type) {
	    if (p1->to < p2->to)
		p1->to = p2->to;
	    if (*p1->name == '~' && *p2->name != '~')
		strcpy(p1->name, p2->name);
	    --piecenum;
	    memmove(p2, p2 + 1, (piecenum - j) * sizeof *pieces);
	} else if (p1->type > p2->type) {
	    if (p1->to >= p2->to) {
		if (*p1->name == '~' && *p2->name != '~')
		    strcpy(p1->name, p2->name);
		--piecenum;
		memmove(p2, p2 + 1, (piecenum - j) * sizeof *pieces);
	    } else {
		p2->from = p1->to;
		p2->shndx = 0;
	    }
	} else {
	    if (p1->from == p2->from) {
		if (p1->to == p2->to) {
		    if (*p1->name == '~')
			strcpy(p1->name, p2->name);
		    p1->type = p2->type;
		    --piecenum;
		    memmove(p2, p2 + 1, (piecenum - j) * sizeof *pieces);
		} else {
		    pieceinfo temp;
		    temp = *p1;
		    *p1 = *p2;
		    *p2 = temp;
		    p2->from = p1->to;
		    p2->shndx = 0;
		}
	    } else if (p1->to <= p2->to) {
		p1->to = p2->from;
	    } else {
		j = p1->to - p2->to;
		p1->to = p2->from;
		recordpiece(p2->to, j, p1->type, 0, p1->name);
		qsort(pieces, piecenum, sizeof *pieces, piececmp);
	    }
	}
    }

    j = 0;
    for (i = 0, p1 = pieces ; i < piecenum ; ++i, ++p1)
	if (p1->to > j)
	    j = p1->to;
    if (j & 3)
	recordpiece(j, 4 - (j & 3), P_NONEXISTENT, 0, "_end");

    for (i = 0, p1 = pieces ; i < piecenum ; ++i, ++p1)
	p1->size = p1->to - p1->from;

    return TRUE;
}

/* Checks the offset and sizing of pieces that contain arrays of
 * anything other than simple characters. If either of these values is
 * inconsistent with the alignment requirements of the field's type,
 * then force the field to be a simple byte array and issue a warning.
 */
void verifypiecetypes(void)
{
    int	i;

    for (i = 0 ; i < piecenum ; ++i) {
	if (ctypes[pieces[i].type].size < 2)
	    continue;
	if (pieces[i].from % (ctypes[pieces[i].type].size == 2 ? 2 : 4))
	    pieces[i].warn |= PW_MISALIGNED;
	if (pieces[i].size % ctypes[pieces[i].type].size)
	    pieces[i].warn |= PW_WRONGSIZE;
    }
}
