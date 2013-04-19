/* out.h: The top-level output functions.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#ifndef	_out_h_
#define	_out_h_

#include	"pieces.h"

/* Begins the output of the C code. Outputs the initial definitions
 * and the beginning of the initialization statement.
 */
extern void beginoutpieces(void);

/* Outputs the initialization code for a piece of the file, given the
 * actual contents of that piece.
 */
extern void outpiece(pieceinfo const *piece, void const *contents);

/* Concludes the output of the C code. Closes the initialization 
 * statement.
 */
extern void endoutpieces(void);

#endif
