/* outtools.h: The higher-level output functions.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#ifndef	_outtools_h_
#define	_outtools_h_

/* A structure that identifies a macro symbol for a numerical value.
 */
typedef	struct names {
    char const	       *name;		/* the macro symbol */
    unsigned long	base;		/* the symbol's value */
    unsigned long	max;		/* the upper value to use */
} names;

/* Given a value and a set of macro symbols, returns a string
 * representation of that value based on the closest matching value.
 */
extern char const *_getname(unsigned int item, names const *r, int count);
#define	getname(item, rn)	\
    _getname(item, rn##_names, sizeof rn##_names / sizeof *(rn##_names))

/* Given a value and a set of macro symbols, returns a string
 * representation of that value as a bitwise-or of values.
 */
extern char const *_getflags(unsigned int item, names const *f, int count);
#define	getflags(item, fn)	\
    _getflags(item, fn##_names, sizeof fn##_names / sizeof *(fn##_names))

/* Returns a string representation of a file offset value, and
 * optionally supplies the index of the piece the offset is within, or
 * -1 if no piece could be reliably associated with the offset.
 */
extern char const *getoffstr(int offset, int *pindex);

/* Given a memory address and a related file offset, supplies strings
 * representing the values, and returns a piece index or -1, as above.
 */
extern int getmemstrs(Elf32_Addr addr, Elf32_Off offset,
		      char const **paddrstr, char const **poffstr);

/* Returns a string representation of a memory address, and optionally
 * supplies a piece index or -1, as above.
 */
extern char const *getaddrstr(Elf32_Addr addr, int *pindex);

/* Returns a string representation of a size, optionally given the
 * index of a piece most likely associated with the size, or -1 if
 * no piece is known to be associated with the size.
 */
extern char const *getsizestr(int size, int index);

#endif
