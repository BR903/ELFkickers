/* pieces.h: Functions for handling the file as a sequence of pieces.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#ifndef	_pieces_h_
#define	_pieces_h_

/* The complete list of types of file pieces. The types are ordered
 * from lowest to highest precedence.
 */
enum
{
    P_UNCLAIMED = 0,				/* not referenced anywhere */
    P_BYTES,					/* generic bunch of bytes */
    P_SECTION,					/* generic section */
    P_HALVES,					/* generic bunch of shorts */
    P_WORDS,					/* generic bunch of words */
    P_NOTE, P_HASH, P_SYMTAB,			/* specific section types */
    P_REL, P_RELA, P_DYNAMIC,			/* specific section types */
    P_SHDRTAB, P_PHDRTAB, P_EHDR,		/* specific ELF headers */
    P_NONEXISTENT,				/* enforced struct padding */
    P_COUNT					/* the number of types */
};

/* Flags to insert warnings in the generated source code.
 */
#define	PW_MISALIGNED	0x0001		/* misaligned for original type */
#define	PW_WRONGSIZE	0x0002		/* incorrect size for original type */

/* The information associated with each file piece.
 */
typedef	struct pieceinfo {
    int		from;				/* offset of start of piece */
    int		to;				/* one past end of piece */
    int		size;				/* (to - from) */
    int		type;				/* the content's type */
    int		shndx;				/* associated shdr entry */
    char	name[64];			/* the piece's field name */
    int		warn;				/* warning flags */
} pieceinfo;

/* The information describing a C type.
 */
typedef	struct typeinfo {
    char const *name;				/* the name of the C type */
    int		size;				/* the type's size */
} typeinfo;

/* The collection of pieces.
 */
extern pieceinfo	       *pieces;
extern int			piecenum;

/* The C types that go with each type of piece.
 */
extern typeinfo const		ctypes[P_COUNT];

/* Records a piece of the file, along with its known type, associated
 * section header table entry (if any), and a suggested name. If the
 * name is prefixed with a tilde, then the name shall be treated as a
 * weak suggestion, to be used if nothing better comes along.
 */
extern void recordpiece(int offset, int size, int type,
			int shndx, char const *name);

/* Sorts the pieces in order of appearance within the file, and deals
 * with any redundancies and/or overlaps.
 */
extern int arrangepieces(void);

/* Checks the alignment and sizing of pieces that contain structure
 * arrays.
 */
void verifypiecetypes(void);

#endif
