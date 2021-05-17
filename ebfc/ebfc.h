/* ebfc.h: Shared functions and definitions for the main program.
 *
 * Copyright (C) 1999,2001,2021 by Brian Raiter, under the GNU General
 * Public License, version 2. No warranty. See COPYING for details.
 */

#ifndef	_ebfc_h_
#define	_ebfc_h_

#include <stdbool.h>
#include "elfparts.h"

/* The various targets that the compiler can generate code for. (The
 * compiled code itself is the same in all cases; the types only
 * differ in the prolog and epilog code.)
 */
enum
{
    CODE_NONE,
    CODE_EXEC,		/* standalone executable */
    CODE_FUNC,		/* exported function called from static code */
    CODE_SOFUNC,	/* exported function in relocatable code */
    CODE_FUNCARG	/* exported function with a buffer argument */
};

/* The following is a list of all the types of elfparts that are used
 * by this program. The numbers assigned to each type are used as
 * indices into the blueprint's part list. ebfc always creates its
 * blueprint with all of these parts, disabling the parts that aren't
 * needed for the chosen ELF target.
 */
enum
{
    P_EHDR = 0,
    P_PHDRTAB, P_HASH, P_DYNSYM, P_DYNSTR, P_TEXT, P_REL, P_GOT,
    P_DYNAMIC, P_DATA, P_SHSTRTAB, P_COMMENT, P_SYMTAB, P_STRTAB, P_SHDRTAB,
    P_COUNT
};

/*
 * Functions in ebfc.c.
 */

/* Outputs a formatted error message on stderr. If fmt is NULL, then
 * uses the error message supplied by perror(). Always returns false.
 */
extern bool err(char const *fmt, ...);

/*
 * Functions in brainfuck.c.
 */

/* Reads the contents of the given file as a Brainfuck program and
 * compiles it to the text segment of the blueprint. The codetype
 * argument indicates how the compiled code expects to be invoked. The
 * funcname argument provides the name of the function to export the
 * compiled code under. If compressed is true, the file contents are
 * interpreted as compressed Brainfuck. The return value is false if
 * an input error occurs or if the source code contains a syntax
 * error. This function also creates the appropriate symbols and
 * relocation records for the chosen ELF target, though with
 * incomplete values.
 */
extern bool compilebrainfuck(char const *filename, blueprint *bp, int codetype,
                             char const *funcname, bool compressed);

/* Fills in values for the symbols and relocation records created by
 * the translatebrainfuck() function. This function must be called
 * after setpositions() has finalized the size and location of each
 * part of the blueprint.
 */
extern bool createfixups(blueprint *bp, int codetype, char const *funcname);

#endif
