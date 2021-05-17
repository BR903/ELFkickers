/* elfpartsi.h: Functions and defintions internal to the elfparts
 * library.
 *
 * Copyright (C) 1999,2001,2021 by Brian Raiter, under the GNU General
 * Public License, version 2. No warranty. See COPYING for details.
 */

#ifndef	_elfpartsi_h_
#define	_elfpartsi_h_

#include <stdbool.h>
#include <elf.h>

/* Value for shtype for a part that has no section header table entry.
 */
#define	SHT_OTHER -1

/* Macro that tests for a part being both present and included in the
 * section header table.
 */
#define _partissection(part) ((part)->shtype > 0)

/* This macro takes advantage of the fact that the ELF header must
 * always be the first item in a blueprint's parts list.
 */
#define _getelfhdr(bp) ((Elf64_Ehdr*)((bp)->parts[0].part))

/* Perform a validation check. If test is true, this function does
 * nothing. Otherwise, message provides a error message relevant to
 * the end user. In the latter case the function will, depending on
 * enforcevalidation(), either return false and store the message, or
 * else it will display the error message on stderr and call exit().
 */
extern int _validate(bool test, char const *message);

/* Change the size of the part's content buffer. This function updates
 * the size field of the blueprint part, as well as reallocating the
 * content buffer as necessary.
 */
extern void *_resizepart(elfpart *part, int size);

#endif
