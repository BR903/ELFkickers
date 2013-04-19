/* outbasic.c: The low-level output functions.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include	<stdarg.h>
#include	"gen.h"
#include	"elftoc.h"
#include	"outbasic.h"

/* The right-hand margin of the output. The output should never extend
 * more than a few characters past this column.
 */
static int const	rmargin = 76;

/* How many spaces to indent within a block.
 */
static int const	indent = 2;

/* The stack of booleans for each level of indentation. A true value
 * on the top of the stack means that the braces around the current
 * block should appear alone on a line.
 */
static char		blockstack[80];

/* The current level of indentation.
 */
static int		level = 0;

/* The cursor's current x-position.
 */
static int		xpos = 0;

/* True if a comma should separate the next item that is output.
 * Separating commas are not output until a sibling item arrives for
 * output, so that the final item in a block will not have a trailing
 * comma.
 */
static int		pendingseparator = 0;


/* Outputs a string and updates xpos.
 */
static void textout(char const *str)
{
    int	n;

    n = strlen(str);
    fputs(str, outfile);
    xpos += n;
    str = strrchr(str, '\n');
    if (str)
	xpos = strlen(str + 1);
}

/* Returns the C representation for a character value as a literal
 * character constant.
 */
char const *cqchar(int ch)
{
    static char	buf[8];

    switch (ch) {
      case '\a': return "'\\a'";	case '\t': return "'\\t'";
      case '\b': return "'\\b'";	case '\v': return "'\\t'";
      case '\f': return "'\\f'";	case '\0': return "'\\0'";
      case '\n': return "'\\n'";	case '\'': return "'\\''";
      case '\r': return "'\\r'";	case '\\': return "'\\\\'";
    }

    if (isprint(ch) || ch == ' ') {
	buf[0] = '\'';
	buf[1] = ch;
	buf[2] = '\'';
	buf[3] = '\0';
    } else
	sprintf(buf, "0x%02X", (unsigned char)ch);
    return buf;
}

/* Returns the C representation for a character value, as it should
 * appear within a literal string constant.
 */
char const *sqchar(char const *ptr)
{
    static char	buf[8];

    switch (*ptr) {
      case '\a': return "\\a";		case '\r': return "\\r";
      case '\b': return "\\b";		case '\t': return "\\t";
      case '\f': return "\\f";		case '\v': return "\\v";
      case '\n': return "\\n";		case '\\': return "\\\\";
      case '"':  return "\\\"";
    }

    if (!*ptr && !(ptr[1] >= '0' && ptr[1] <= '7'))
	return "\\0";
    else if (isprint(*ptr) || *ptr == ' ') {
	buf[0] = *ptr;
	buf[1] = '\0';
    } else {
	buf[0] = '\\';
	buf[1] = '0' + ((unsigned char)*ptr / 64);
	buf[2] = '0' + (((unsigned char)*ptr / 8) % 8);
	buf[3] = '0' + (*ptr % 8);
	buf[4] = '\0';
    }
    return buf;
}

/* Given a series of length bytes at str, returns the number of
 * characters that would be required to output those bytes within a
 * literal C string.
 */
int stringsize(char const *str, int length)
{
    int	size = 0;

    for ( ; length ; ++str, --length) {
	if (isprint(*str))
	    ++size;
	else
	    switch (*str) {
	      case ' ':
		++size;
		break;
	      case '\0':
		size += length > 1 && str[1] >= '0' && str[1] <= '7' ? 4 : 2;
		break;
	      case '\a': case '\r':
	      case '\b': case '\t':
	      case '\f': case '\v':
	      case '\n': case '\\':
	      case '"':
		size += 2;
		break;
	      default:
		size += 4;
		break;
	    }
    }
    return size;
}

/* Moves the output to the beginning of the next line, if the current
 * cursor position is not already at the beginning of a line.
 */
void linebreak(void)
{
    static char	spaces[80];
    int		n;

    if (pendingseparator) {
	textout(",");
	pendingseparator = 0;
    }
    n = level * indent;
    if (xpos > n)
	textout("\n");
    if (xpos < n) {
	n -= xpos;
	memset(spaces, ' ', n);
	spaces[n] = '\0';
	textout(spaces);
    }
}

/* Starts a new block: outputs a left brace and increases the current
 * indentation level by one. If brk is true, then the block's braces
 * will be followed by line breaks; otherwise, the block's contents
 * will appear on the same lines as the braces.
 */
void beginblock(int brk)
{
    linebreak();
    blockstack[level] = brk;
    ++level;
    pendingseparator = 0;
    textout("{");
    if (brk) {
	textout("\n");
	linebreak();
    } else
	textout(" ");
}

/* Closes the current block: outputs a right brace and decrements the
 * level of indentation.
 */
void endblock(void)
{
    pendingseparator = 0;
    --level;
    if (blockstack[level])
	linebreak();
    else
	textout(" ");
    textout("}");
    if (level)
	pendingseparator = -1;
    else
	textout(";\n");
}

/* Outputs a string, and if the indentation level is not zero (i.e.,
 * if the current output is inside a block, a separator is set as
 * pending.
 */
void out(char const *str)
{
    int	n;

    n = strlen(str);
    if (xpos + n >= rmargin)
	linebreak();
    if (pendingseparator) {
	if (pendingseparator > 0 && xpos + 2 < rmargin)
	    textout(", ");
	else
	    linebreak();
    }
    textout(str);
    pendingseparator = level > 0;
}

/* Calls out after building a formatted string.
 */
void outf(char const *fmt, ...)
{
    static char	       *buf = NULL;
    static int		bufsize = 0;
    va_list		args;
    int			n;

    va_start(args, fmt);
#if 0
    while (!bufsize || (n = vsnprintf(buf, bufsize, fmt, args)) < 0) {
	bufsize += rmargin;
	xalloc(buf, bufsize);
    }
#else
    for (;;) {
	bufsize += rmargin;
	xalloc(buf, bufsize);
	n = vsnprintf(buf, bufsize, fmt, args);
	if (n >= 0 && n < bufsize)
	    break;
    }
#endif
    va_end(args);
    out(buf);
}

/* Takes an integer and outputs it in hexadecimal notation.
 */
void outword(unsigned int word)
{
    if (word > 0xFFFFFF)
	outf("0x%08X", word);
    else if (word > 0xFFFF)
	outf("0x%06X", word);
    else if (word > 15)
	outf("0x%X", word);
    else
	outf("%u", word);
}

/* Takes a string (not necessarily null-terminated) and outputs it as
 * a literal C string, as a single item.
 */
void outstring(char const *str, int length)
{
    char const *s;
    int		ps, i;

    if (xpos >= rmargin || (length > 4 && xpos + 8 >= rmargin))
	linebreak();
    out("\"");
    ps = pendingseparator;
    pendingseparator = 0;
    ++level;
    for (i = 0, s = str ; i < length ; ++i, ++s) {
	textout(sqchar(s));
	if (xpos >= rmargin && i + 2 < length) {
	    textout("\"");
	    linebreak();
	    textout("\"");
	}
    }
    --level;
    textout("\"");
    pendingseparator = ps;
}

/* Outputs a given string as a C comment, with no separator pending
 * afterwards.
 */
void outcomment(char const *str)
{
    int	n;

    n = strlen(str);
    if (pendingseparator) {
	if (pendingseparator < 0 || xpos + n + 9 >= rmargin) {
	    textout(",");
	    linebreak();
	} else
	    textout(", ");
	pendingseparator = 0;
    } else {
	if (xpos + n + 7 >= rmargin)
	    linebreak();
    }
    textout("/* ");
    textout(str);
    textout(" */ ");
}
