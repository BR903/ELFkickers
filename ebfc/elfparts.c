/* elfparts.c: Global functions supplied by the elfparts library.
 *
 * Copyright (C) 1999,2001,2021 by Brian Raiter, under the GNU General
 * Public License, version 2. No warranty. See COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <elf.h>
#include "elfparts.h"
#include "elfpartsi.h"

/* Align a position to the next boundary appropriate to the
 * destination.
 */
#define filealignment (sizeof(Elf64_Addr) - 1)
#define memalignment (0x1000 - 1)
#define	filealign(off) (((off) + filealignment) & ~filealignment)
#define	memalign(off) (((off) + memalignment) & ~memalignment)

/* Default load address for ELF executables.
 */
static intptr_t const loadaddr = 0x00400000;

/* Most recent error message.
 */
static char const *errormessage = NULL;

/* True if the library should call exit() when a validation error is
 * detected.
 */
static bool dieoninvalid = true;

/* A wrapper function for fwrite() that automatically returns success
 * on a write of zero bytes.
 */
static bool writebytes(FILE *fp, void const *p, int size)
{
    if (size <= 0)
        return true;
    return fwrite(p, size, 1, fp) == 1;
}

/* A function that encapsulates the allocation of memory for the
 * contents of an elfpart. This macro assumes that the part field has
 * already been safely initialized.
 */
void *_resizepart(elfpart *part, int size)
{
    part->size = size;
    part->part = realloc(part->part, size);
    _validate(part->part, "out of memory");
    return part->part;
}

/*
 * Error handling functions.
 */

/* Set how the library should handle invalid states. If flag is true,
 * encountering an invalid state will cause an error to be displayed
 * on stderr and the program will exit. If flag is false, error
 * messages will instead be stored, and can be retrieved with
 * getlasterror().
 */
void enforcevalidation(bool flag)
{
    dieoninvalid = flag;
}

/* Return a text message describing the most recent error, or NULL if
 * no error has occurred.
 */
char const *getlasterror(void)
{
    return errormessage;
}

/* Return a text message describing the most recent error, or NULL if
 * no error has occurred.
 */
int _validate(bool test, char const *message)
{
    if (test)
        return true;
    if (dieoninvalid) {
        fprintf(stderr, "error: %s\n", message);
        exit(EXIT_FAILURE);
    }
    errormessage = message;
    return false;
}

/*
 * The ELF stage building functions. These functions must be called in
 * sequence. These functions all use _validate() to detect error
 * conditions, so unless dieoninvalid is false, they will only return
 * on a successful state. Three of these functions (initparts(),
 * fillparts(), and completeparts()) have the same basic structure of
 * calling a specific function defined for each part of the blueprint,
 * using the done flag to allow some parts to depend on data provided
 * by other parts. The newparts() function is similar, except that
 * parts cannot have interdependencies at this point, so the done flag
 * is not used. Finally, the measureparts() function works the same
 * for each part regardless of its type, so it is self-contained.
 */

/* Call new() on each blueprint part to put it into its initial state.
 * The blueprint parts cannot introduce dependencies on each other via
 * this function.
 */
bool newparts(blueprint *bp)
{
    if (!_validate(bp->partcount > 0 && bp->parts, "empty blueprint"))
        return false;
    foreachpart (part in bp) {
        if (part->new)
            if (!(*part->new)(part))
                return false;
    }
    return true;
}

/* Call init() on each blueprint part to put it in a valid, empty
 * state. When this function returns, each part should be ready to be
 * modified by the caller. Parts that are depeendent on the presence
 * or absence of other parts should make such determinations in this
 * function.
 */
bool initparts(blueprint *bp)
{
    int notdone, notdoneprev;

    foreachpart (part in bp)
        part->done = !partispresent(part) || !part->init;
    notdone = bp->partcount;
    while (notdone) {
        notdoneprev = notdone;
        notdone = 0;
        foreachpart (part in bp) {
            if (!part->done) {
                if (!(*part->init)(part, bp))
                    return false;
                if (!part->done)
                    ++notdone;
            }
        }
        if (!_validate(notdone < notdoneprev,
                       "mutually dependent ELF parts in initparts()"))
            return false;
    }
    return true;
}

/* Call fill() on each blueprint part to add any and all data that the
 * part provides automatically. Before calling this function, the
 * caller should have finished adding their own data to each part.
 * When this function returns, each part's size cannot be changed
 * further.
 */
bool fillparts(blueprint *bp)
{
    int notdone, notdoneprev;

    foreachpart (part in bp)
        part->done = !partispresent(part) || !part->fill;
    notdone = bp->partcount;
    while (notdone) {
        notdoneprev = notdone;
        notdone = 0;
        foreachpart (part in bp) {
            if (!part->done) {
                if (!(*part->fill)(part, bp))
                    return false;
                if (!part->done)
                    ++notdone;
            }
        }
        if (!_validate(notdone < notdoneprev,
                       "mutually dependent ELF parts in fillparts()"))
            return false;
    }
    return true;
}

/* Determine the position of each part, in the ELF file and (for parts
 * that are loaded) in memory.
 */
void measureparts(blueprint *bp)
{
    intptr_t off;

    off = 0;
    foreachpart (part in bp) {
	if (partispresent(part)) {
	    if (part->entsize)
		part->size = part->count * part->entsize;
	    part->offset = off;
	    off = filealign(off + part->size);
        }
    }

    if (bp->filetype != ET_REL) {
	off = 0;
        foreachpart (part in bp) {
	    if (part->flags) {
		part->addr = part->offset;
		if (!(part->flags & PF_W)) {
		    if (off < part->addr + part->size)
			off = part->addr + part->size;
		}
            }
	}
	off = memalign(off);
        foreachpart (part in bp)
	    if (part->flags & PF_W)
		part->addr += off;

	if (bp->filetype == ET_EXEC) {
            foreachpart (part in bp)
		if (part->flags)
		    part->addr += loadaddr;
	}
    }
}

/* Call complete() on each blueprint part to update any values that
 * could not be determined at an earlier stage. After this function
 * the blueprint is complete and the ELF file can be output.
 */
bool completeparts(blueprint *bp)
{
    int notdone, notdoneprev;

    foreachpart (part in bp)
        part->done = !partispresent(part) || !part->complete;
    notdone = bp->partcount;
    while (notdone) {
        notdoneprev = notdone;
        notdone = 0;
        foreachpart (part in bp) {
            if (!part->done) {
                if (!(*part->complete)(part, bp))
                    return false;
                if (!part->done)
                    ++notdone;
            }
        }
        if (!_validate(notdone < notdoneprev,
                       "mutually dependent ELF parts in completeparts()"))
            return false;
    }
    return true;
}

/*
 * Functions that connect parts to each others.
 */

/* Define which string table is the section header string table.
 */
bool setshdrstrs(blueprint *bp, int shdrsindex, int strsindex)
{
    if (!_validate(shdrsindex < bp->partcount && strsindex < bp->partcount,
                   "invalid index in setshdrstrs()"))
        return false;
    bp->parts[shdrsindex].link = &bp->parts[strsindex];
    return true;
}

/* Define which symbol table a hash table refers to.
 */
bool sethashsyms(blueprint *bp, int hashindex, int symsindex)
{
    if (!_validate(hashindex < bp->partcount && symsindex < bp->partcount,
                   "invalid index in sethashsyms()"))
        return false;
    bp->parts[hashindex].link = &bp->parts[symsindex];
    return true;
}

/* Define which string table that a symbol table uses.
 */
bool setsymstrings(blueprint *bp, int symsindex, int strsindex)
{
    if (!_validate(symsindex < bp->partcount && strsindex < bp->partcount,
                   "invalid index in setsymstrings()"))
        return false;
    bp->parts[symsindex].link = &bp->parts[strsindex];
    return true;
}

/* Define which symbol table a relocation table uses.
 */
bool setrelsyms(blueprint *bp, int relindex, int symsindex)
{
    if (!_validate(relindex < bp->partcount && symsindex < bp->partcount,
                   "invalid index in setrelsyms()"))
        return false;
    bp->parts[relindex].link = &bp->parts[symsindex];
    return true;
}

/* Define which section a relocation table modifies.
 */
bool setrelsection(blueprint *bp, int relindex, int sectindex)
{
    if (!_validate(relindex < bp->partcount && sectindex < bp->partcount,
                   "invalid index in setrelsection()"))
        return false;
    bp->parts[relindex].info = sectindex;
    return true;
}

/* Set the e_entry field of the ELF header.
 */
void setexecentry(blueprint *bp, intptr_t addr)
{
    _getelfhdr(bp)->e_entry = addr;
}

/*
 * The output function.
 */

/* Write the completed blueprint to the given filename. The return
 * value is false if an error occurs during output.
 */
bool outputelf(blueprint const *bp, char const *filename)
{
    char padding[64] = { 0 };
    FILE *fp;
    Elf64_Off off;
    int success;

    if (!(fp = fopen(filename, "wb")))
	return false;
    off = 0;
    foreachpart (part in bp) {
	if (!partispresent(part))
	    continue;
        success = writebytes(fp, padding, part->offset - off)
               && writebytes(fp, part->part, part->size);
        if (!success) {
	    int e = errno;
	    fclose(fp);
	    errno = e;
	    return false;
	}
	off = part->offset + part->size;
    }
    if (fclose(fp))
	return false;

    return true;
}
