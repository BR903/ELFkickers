/* brainfuck.c: The part that translates Brainfuck into x86 assembly.
 *
 * Copyright (C) 1999,2001,2021 by Brian Raiter, under the GNU General
 * Public License, version 2. No warranty. See COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <elf.h>
#include "elfparts.h"
#include "ebfc.h"

/*
 * The bits of machine language that, together, make up a compiled
 * program. The prolog and epilog are dependent on the type of code
 * being generated. The prologs are declared as bespoke structs so
 * that internal offsets may be computed.
 */

#define	MLBIT(size) static struct { uint8_t _dummy[size]; } const 

MLBIT(5) start = { { 0xBA, 0x01, 0x00, 0x00, 0x00 } };	/* mov  edx, 1     */

MLBIT(2) incbyte =  { { 0xFE, 0x06 } };			/* inc  [rsi]      */
MLBIT(2) decbyte =  { { 0xFE, 0x0E } };			/* dec  [rsi]      */
MLBIT(3) incptr =   { { 0x48, 0xFF, 0xC6 } };		/* inc  rsi        */
MLBIT(3) decptr =   { { 0x48, 0xFF, 0xCE } };		/* dec  rsi        */

MLBIT(6) readbyte = { { 0x31, 0xC0,			/* xor  eax, eax   */
			0x89, 0xC7,			/* mov  edi, eax   */
			0x0F, 0x05 } };			/* syscall         */
MLBIT(6) writebyte= { { 0x89, 0xD0,			/* mov  eax, edx   */
			0x89, 0xC7,			/* mov  edi, eax   */
			0x0F, 0x05 } };			/* syscall         */

MLBIT(5) jump =     { { 0xE9, 0, 0, 0, 0 } };		/* jmp  near ???   */
MLBIT(3) branch =   { { 0x3A, 0x36,			/* cmp  dh, [rsi]  */
			0x75 } };			/* jnz  short ...  */
MLBIT(4) nbranch =  { { 0x3A, 0x36,			/* cmp  dh, [rsi]  */
			0x0F, 0x85 } };			/* jnz  near ...   */

MLBIT(2) addbyte =  { { 0x80, 0x06 } };			/* add  [rsi], ... */
MLBIT(2) subbyte =  { { 0x80, 0x2E } };			/* sub  [rsi], ... */
MLBIT(3) addptr =   { { 0x48, 0x83, 0xC6 } };		/* add  rsi, ...   */
MLBIT(3) subptr =   { { 0x48, 0x83, 0xEE } };		/* sub  rsi, ...   */
MLBIT(2) zerobyte = { { 0x88, 0x36 } };			/* mov  [rsi], 0   */

MLBIT(9)
	epilog_ex = { { 0xB8, 0x3C, 0x00, 0x00, 0x00,	/* mov  eax, 60   */
			0x31, 0xFF,			/* xor  edi, edi   */
			0x0F, 0x05 } };			/* syscall         */
MLBIT(5)
	epilog_fn = { { 0x5A,				/* pop  rdx        */
			0x5F,				/* pop  rdi        */
			0x5E,				/* pop  rsi        */
			0x5D,				/* pop  rbp        */
			0xC3 } };			/* ret             */

static struct prolog_ex
    { uint8_t _movrsi[2], dataoff[8]; } const
	prolog_ex = { { 0x48, 0xBE }, { 0 } };		/* mov  rsi, ???   */

static struct prolog_fn
    { uint8_t _pushes[7];
      uint8_t _movrsi[2], dataoff[8];
      uint8_t _resetdata[12]; } const
	prolog_fn = { { 0x55,				/* push rbp        */
                        0x48, 0x89, 0xE5,               /* mov  rbp, rsp   */
                        0x56,                           /* push rsi        */
                        0x57,                           /* push rdi        */
                        0x52 },                         /* push rdx        */
                      { 0x48, 0xBE }, { 0 },		/* mov  rsi, ???   */
		      { 0x48, 0x89, 0xF7,		/* mov  rdi, rsi   */
                        0xB9, 0x00, 0x80, 0x00, 0x00,   /* mov  ecx, 8000h */
                        0x31, 0xC0,			/* xor  eax, eax   */
                        0xF3, 0xAA } };			/* rep stosb       */

MLBIT(10)
	prolog_fa = { { 0x55,				/* push rbp        */
                        0x48, 0x89, 0xE5,               /* mov  rbp, rsp   */
                        0x56,                           /* push rsi        */
                        0x57,                           /* push rdi        */
                        0x52,				/* push rdx        */
                        0x48, 0x89, 0xFE } };		/* mov  rsi, rdi   */

static struct prolog_so
    { uint8_t _pushes[12];
      uint8_t ripaddr[1];
      uint8_t _getgotpc[2], gotpc[8];
      uint8_t _addtorsi[3];
      uint8_t _getdatagotoff[2], datagotoff[8];
      uint8_t _addtorsiagain[3];
      uint8_t _resetdata[12]; } const
	prolog_so = { { 0x55,				/* push rbp        */
                        0x48, 0x89, 0xE5,               /* mov  rbp, rsp   */
                        0x56,                           /* push rsi        */
                        0x57,                           /* push rdi        */
                        0x52,                           /* push rdx        */
                        0xE8, 0x00, 0x00, 0x00, 0x00 }, /* call $          */
                      { 0x5E },				/* pop  rsi        */
                      { 0x48, 0xB8 }, { 0 },		/* mov  rax, ???   */
		      { 0x48, 0x01, 0xC6 },		/* add  rsi, rax   */
                      { 0x48, 0xB8 }, { 0 },		/* mov  rax, ???   */
		      { 0x48, 0x01, 0xC6 },		/* add  rsi, rax   */
		      { 0x48, 0x89, 0xF7,		/* mov  rdi, rsi   */
                        0xB9, 0x00, 0x80, 0x00, 0x00,   /* mov  ecx, 32768 */
                        0x31, 0xC0,			/* xor  eax, eax   */
                        0xF3, 0xAA } };			/* rep stosb       */

/* The size of a Brainfuck program's data segment is set to 32k.
 */
static int const datafieldsize = 0x8000;

/* A pointer to the text buffer.
 */
static char *textbuf;

/* The amount of memory allocated for the text buffer.
 */
static int textallocsize;

/* The first byte past the program being compiled in the text buffer.
 */
static int pos;

/* Appends the given bytes to the program being compiled.
 */
static void emit(void const *bytes, int size)
{
    if (pos + size > textallocsize) {
	textallocsize += 4096;
	if (!(textbuf = realloc(textbuf, textallocsize))) {
	    fputs("Out of memory!\n", stderr);
	    exit(EXIT_FAILURE);
	}
    }
    memcpy(textbuf + pos, bytes, size);
    pos += size;
}

/* Macros to simplify calling emit with predefined bits of code.
 */
#define	emitobj(obj) (emit(&(obj), sizeof (obj)))
#define	emitarg(seq, arg) (emitobj(seq), emitobj(arg))

/* Modify previously emitted code at the given position.
 */
#define	insertobj(at, obj) (memcpy(textbuf + (at), &(obj), sizeof (obj)))

/* Translate a single, possibly repeated, command. Returns false if a
 * syntax error is detected (i.e. mismatched brackets). In addition to
 * the standard eight Brainfuck commands, this function accepts the
 * following extra values as internal pseudo-commands:
 *   NUL: emit prolog code
 *   EOF: emit epilog code
 *   'Z': zero current cell
 */
static bool translatecmd(int cmd, char arg)
{
    static int stack[256];
    static int *st;
    int32_t diff;
    int8_t bdiff;

    switch (cmd) {

      case '+':
        if (arg == 1)
            emitobj(incbyte);
        else
            emitarg(addbyte, arg);
        break;

      case '-':
        if (arg == 1)
            emitobj(decbyte);
        else
            emitarg(subbyte, arg);
        break;

      case '>':
        if (arg == 1)
            emitobj(incptr);
        else
            emitarg(addptr, arg);
        break;

      case '<':
        if (arg == 1)
            emitobj(decptr);
        else
            emitarg(subptr, arg);
        break;

      case '.':
        while (arg--)
            emitobj(writebyte);
        break;

      case ',':
        while (arg--)
            emitobj(readbyte);
        break;

      case '[':
        if ((st - stack) + arg > (int)(sizeof stack / sizeof *stack)) {
	    err("too many levels of nested loops");
	    return false;
	}
	while (arg--) {
	    emitobj(jump);
	    *st = pos;
	    ++st;
	}
	break;

      case ']':
	while (arg--) {
	    if (st == stack) {
		err("unmatched ]");
		return false;
	    }
	    --st;
	    diff = pos - *st;
            insertobj(*st - sizeof diff, diff);
            if (diff + sizeof branch + sizeof bdiff <= 0x80) {
                bdiff = -(diff + sizeof branch + sizeof bdiff);
                emitarg(branch, bdiff);
            } else {
                diff = -(diff + sizeof nbranch + sizeof diff);
                emitarg(nbranch, diff);
            }
	}
	break;

      case 0:
	st = stack;
	emitobj(start);
	break;

      case 'Z':
        emitobj(zerobyte);
        break;

      case EOF:
	if (st != stack) {
	    err("unmatched [");
	    return false;
	}
	break;
    }

    return true;
}

/* A wrapper around the real compiling function that detects repeated
 * commands and collapses them into a single call to translatecmd().
 */
static bool compilecmdwithcollapse(int cmd, int count)
{
    static int lastcmd = -1;
    static int cmdcount = 0;

    if (cmd == lastcmd && cmdcount + count < 0x80) {
        cmdcount += count;
        return true;
    }

    if (cmdcount > 0) {
        if (!translatecmd(lastcmd, cmdcount))
            return false;
        lastcmd = -1;
        cmdcount = 0;
    }
    if (cmd == 0 || cmd == EOF)
        return translatecmd(cmd, count);

    lastcmd = cmd;
    cmdcount = count;
    return true;
}

/* Compile a single, possibly repeated Brainfuck command. The purpose
 * of this function is to detect the Brainfuck idiom of "[-]" for
 * zeroing a cell and transform it into a single pseudo-command.
 */
static bool compilecmd(int cmd, int count)
{
    static char const *zerocmd = "[-]";
    static int const zerocmdlen = 3;
    static int bufcount = 0;
    int i;

    if (count == 1 && cmd == zerocmd[bufcount]) {
        ++bufcount;
        if (bufcount == zerocmdlen) {
            bufcount = 0;
            return compilecmdwithcollapse('Z', 1);
        }
        return true;
    } else if (bufcount) {
        for (i = 0 ; i < bufcount ; ++i)
            if (!compilecmdwithcollapse(zerocmd[i], 1))
                return false;
        bufcount = 0;
    }
    return compilecmdwithcollapse(cmd, count);
}

/* Read a Brainfuck source file and call compilecmd() on the contents.
 */
static bool compileuncompressed(FILE *fp)
{
    static bool cmdlookup[128] = {
        ['+'] = true,   ['['] = true,
        ['-'] = true,   [']'] = true,
        ['<'] = true,   ['.'] = true,
        ['>'] = true,   [','] = true,
    };
    int cmd;

    if (!compilecmd(0, 0))
        return false;
    while ((cmd = fgetc(fp)) != EOF) {
        if (cmd < 0 || cmd >= 128 || !cmdlookup[cmd])
	    continue;
        if (!compilecmd(cmd, 1))
            return false;
    }
    return compilecmd(EOF, 0);
}

/* Read a compressed Brainfuck source file and call compilecmd() on
 * the contents. Bytes are uncompressed using the following schema:
 *
 * 000: +  001: -  010: <  011: >  100: [  101: ]  110: ,  111: .
 *
 * 00xxxXXX = singleton:  xxx (when xxx == XXX)
 * 00xxxyyy = pair:       xxx then yyy
 * 10xxyyzz = triple:     0xx then 0yy then 0zz
 * 01xxxyyy = repetition: yyy repeated 2+xxx times (2-9)
 * 11xxxxyy = repetition: 0yy repeated 2+xxxx times (2-17)
 */
static bool compilecompressed(FILE *fp)
{
    static char const *cmdlist = "+-<>[],.";
    int byte;

    if (!compilecmd(0, 0))
	return false;
    while ((byte = fgetc(fp)) != EOF) {
	switch (byte & 0xC0) {
	  case 0x00:
	    if (!compilecmd(cmdlist[(byte >> 3) & 7], 1))
		return false;
	    if (((byte >> 3) & 7) != (byte & 7))
		if (!compilecmd(cmdlist[byte & 7], 1))
		    return false;
	    break;
	  case 0x80:
	    if (!compilecmd(cmdlist[(byte >> 4) & 3], 1)
			|| !compilecmd(cmdlist[(byte >> 2) & 3], 1)
			|| !compilecmd(cmdlist[byte & 3], 1))
		return false;
	    break;
	  case 0x40:
	    if (!compilecmd(cmdlist[byte & 7], 2 + ((byte >> 3) & 7)))
		return false;
	    break;
	  case 0xC0:
	    if (!compilecmd(cmdlist[byte & 3], 2 + ((byte >> 2) & 15)))
		return false;
	    break;
	}
    }
    return compilecmd(EOF, 0);
}

/* Add entries to the relocation section and/or the symbol table,
 * depending on what kind of file is being generated.
 */
static void addrelocations(blueprint const *bp, int codetype,
			   char const *function)
{
    if (bp->filetype == ET_DYN)
        addtosymtab(&bp->parts[P_DYNSYM], function,
                    STB_GLOBAL, STT_FUNC, P_TEXT);
    if (bp->filetype != ET_REL)
        return;

    addtosymtab(&bp->parts[P_SYMTAB], function, STB_GLOBAL, STT_FUNC, P_TEXT);

    switch (codetype) {
      case CODE_EXEC:
	addsymbolrel(&bp->parts[P_REL],
                     offsetof(struct prolog_ex, dataoff), R_X86_64_64,
                     NULL, STB_LOCAL, STT_OBJECT, P_DATA);
	break;
      case CODE_FUNC:
	addsymbolrel(&bp->parts[P_REL],
                     offsetof(struct prolog_fn, dataoff), R_X86_64_64,
                     NULL, STB_LOCAL, STT_OBJECT, P_DATA);
	break;
      case CODE_SOFUNC:
	addentrytorel(&bp->parts[P_REL], offsetof(struct prolog_so, gotpc),
                      getsymindex(&bp->parts[P_SYMTAB], NAME_GOT),
                      R_X86_64_GOTPC64);
	addsymbolrel(&bp->parts[P_REL],
                     offsetof(struct prolog_so, datagotoff), R_X86_64_GOTOFF64,
                     NULL, STB_LOCAL, STT_OBJECT, P_DATA);
	break;
      case CODE_FUNCARG:
        removepart(&bp->parts[P_REL]);
        break;
    }
}

/* Compile the contents of the given file into the text segment of the
 * blueprint, as a global function with the given name. Relocations
 * and other fixups are added as appropriate, though with incomplete
 * values. The return value is false if a fatal error occurs.
 */
bool compilebrainfuck(char const *filename, blueprint *bp,
                      int codetype, char const *function, bool compressed)
{
    FILE *fp;

    if (!(fp = fopen(filename, compressed ? "rb" : "r")))
	return err(NULL);

    textbuf = bp->parts[P_TEXT].part;
    textallocsize = bp->parts[P_TEXT].size;
    pos = 0;

    switch (codetype) {
      case CODE_EXEC:		emitobj(prolog_ex);	break;
      case CODE_FUNC:		emitobj(prolog_fn);	break;
      case CODE_SOFUNC:		emitobj(prolog_so);	break;
      case CODE_FUNCARG:	emitobj(prolog_fa);	break;
    }

    if (!(compressed ? compilecompressed(fp) : compileuncompressed(fp)))
	return false;

    if (fclose(fp))
	err(NULL);

    switch (codetype) {
      case CODE_EXEC:		emitobj(epilog_ex);	break;
      case CODE_FUNC:		emitobj(epilog_fn);	break;
      case CODE_SOFUNC:		emitobj(epilog_fn);	break;
      case CODE_FUNCARG:	emitobj(epilog_fn);	break;
    }

    bp->parts[P_TEXT].size = pos;
    bp->parts[P_TEXT].part = textbuf;

    if (codetype == CODE_FUNCARG) {
        bp->parts[P_DATA].size = 0;
        removepart(&bp->parts[P_DATA]);
    } else {
        bp->parts[P_DATA].size = datafieldsize;
        if (!(bp->parts[P_DATA].part = calloc(datafieldsize, 1)))
            return err("Out of memory!");
    }

    addrelocations(bp, codetype, function);
    return true;
}

/* Complete the relocations and other fixups that were added by the
 * previous function.
 */
bool createfixups(blueprint *bp, int codetype, char const *function)
{
    intptr_t off, addr;

    textbuf = bp->parts[P_TEXT].part;

    if (codetype == CODE_SOFUNC) {
	off = offsetof(struct prolog_so, gotpc);
	addr = offsetof(struct prolog_so, ripaddr);
	if (bp->filetype == ET_REL)
	    addr = off - addr;
	else
	    addr = bp->parts[P_GOT].addr - (bp->parts[P_TEXT].addr + addr);
	insertobj(off, addr);
    }

    switch (codetype) {
      case CODE_EXEC:	off = offsetof(struct prolog_ex, dataoff);	break;
      case CODE_FUNC:	off = offsetof(struct prolog_fn, dataoff);	break;
      case CODE_SOFUNC:	off = offsetof(struct prolog_so, datagotoff);	break;
    }
    addr = 0;
    if (bp->filetype != ET_REL) {
        if (codetype == CODE_EXEC)
            addr = bp->parts[P_DATA].addr;
        else if (codetype == CODE_SOFUNC)
            addr = bp->parts[P_DATA].addr - bp->parts[P_GOT].addr;
    }
    if (codetype != CODE_FUNCARG)
        insertobj(off, addr);

    if (bp->filetype == ET_REL)
	setsymvalue(&bp->parts[P_SYMTAB], function, 0);
    else if (bp->filetype == ET_DYN)
	setsymvalue(&bp->parts[P_DYNSYM], function, bp->parts[P_TEXT].addr);
    else if (bp->filetype == ET_EXEC)
        setexecentry(bp, bp->parts[P_TEXT].addr);

    return true;
}
