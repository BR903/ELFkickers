/* gen.h: Generic functions and definitions used by all modules.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#ifndef	_gen_h_
#define	_gen_h_

#include	<stdio.h>
#include	<stdlib.h>

#ifndef TRUE
#define	TRUE		1
#define	FALSE		0
#endif

/* The memory allocation macro. If memory is unavailable, the program
 * is aborted.
 */
#define	xalloc(p, n)	(((p) = realloc((p), (n))) ? (p) : (err(NULL), NULL))

/* Prints a message on stderr and returns FALSE. If fmt is NULL, prints
 * an out-of-memory error and aborts the program.
 */
extern int err(char const *fmt, ...);

#endif
