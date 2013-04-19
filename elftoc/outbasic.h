/* outbasic.h: The low-level output functions.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#ifndef	_outbasic_h_
#define	_outbasic_h_

/* Returns the C representation for a character value as a literal
 * character constant.
 */
extern char const *cqchar(int ch);

/* Returns the C representation for a character value, for use within
 * a literal string constant.
 */
extern char const *sqchar(char const *str);

/* Returns the length of a C representation of a literal C string.
 */
extern int stringsize(char const *str, int length);

/* Begins a C block. brk is true if the opening brace should be on a
 * separate line from the block's contents.
 */
extern void beginblock(int brk);

/* Closes a C block.
 */
extern void endblock(void);

/* Adds a line break to the output.
 */
extern void linebreak(void);

/* Outputs a string. If the output is within a block, then the string
 * is assumed to be a single item, and will be separated from its
 * siblings by commas.
 */
extern void out(char const *str);

/* Outputs a formatted sequence, as above.
 */
extern void outf(char const *fmt, ...);

/* Outputs a number, as above.
 */
#define	outint(n)	(outf("%ld", (long)(n)))

/* Outputs a number, preferring hexadecimal notation, as above.
 */
extern void outword(unsigned int word);

/* Outputs a literal C string, as above.
 */
extern void outstring(char const *str, int size);

/* Outputs a string inside a comment. If output is currently inside a
 * block, then the comment will be separated from its left "sibling"
 * by a comma.
 */
extern void outcomment(char const *str);

#endif
