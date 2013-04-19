/* elftoc.h: Data defined in the central module.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#ifndef	_elftoc_h_
#define	_elftoc_h_

#include	<stdio.h>

/* The name to use for the initialized variable in the generated C code.
 */
extern char const      *varname;

/* The name to use for the structure defined in the generated C code.
 */
extern char const      *structname;

/* The output file.
 */
extern FILE	       *outfile;

#endif
