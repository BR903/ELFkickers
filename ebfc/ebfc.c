/* ebfc.c: The central module.
 *
 * Copyright (C) 1999,2001,2021 by Brian Raiter, under the GNU General
 * Public License, version 2. No warranty. See COPYING for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <getopt.h>
#include <elf.h>
#include "elfparts.h"
#include "ebfc.h"

/* The online help text.
 */
static char const      *yowzitch =
    "Usage: ebfc [OPTIONS] FILENAME\n"
    "Compiles Brainfuck source code to an ELF target.\n\n"
    "  -x                    Generate a standalone executable file.\n"
    "  -l                    Generate a shared library.\n"
    "  -c                    Generate the object file for a function.\n"
    "  -xc                   Generate the object file for an executable.\n"
    "  -lc                   Generate the object file for a shared library.\n"
    "  -a, --arg             Modify the function to take a buffer argument.\n"
    "  -z, --compressed      Read the input file as compressed BF source.\n"
    "  -o, --output=FILE     Write the output to FILE.\n"
    "  -f, --function=NAME   Use NAME as the library's exported function.\n"
    "  -i, --input=NAME      Record the source filename as NAME.\n"
    "  -s, --strip           Omit non-required data from the output file.\n"
    "      --help            Display this help and exit.\n"
    "      --version         Display version information and exit.\n";

/* The version text.
 */
static char const *vourzhon =
    "ebfc, version 2.0\n"
    "Copyright (C) 1999,2021 by Brian Raiter <breadbox@muppetlabs.com>\n"
    "License GPLv2+: GNU GPL version 2 or later.\n"
    "This is free software; you are free to change and redistribute it.\n"
    "There is NO WARRANTY, to the extent permitted by law.\n";

/* The full collection of elfparts used by this program. This list
 * must match up with the enum that appears in the header file.
 */
static elfpart const *parttable[P_COUNT] = {
    [P_EHDR]     = &part_ehdr,
    [P_PHDRTAB]  = &part_phdrtab,
    [P_HASH]     = &part_hash,
    [P_DYNSYM]   = &part_dynsym,
    [P_DYNSTR]   = &part_dynstr,
    [P_TEXT]     = &part_text,
    [P_REL]      = &part_rel,
    [P_GOT]      = &part_got,
    [P_DYNAMIC]  = &part_dynamic,
    [P_DATA]     = &part_data,
    [P_COMMENT]  = &part_progbits,
    [P_SHSTRTAB] = &part_shstrtab,
    [P_SYMTAB]   = &part_symtab,
    [P_STRTAB]   = &part_strtab,
    [P_SHDRTAB]  = &part_shdrtab,
};

/* Lists indicating which elfparts are included in which file types.
 */
static int partlists[][P_COUNT] = {
    [ET_EXEC] = { P_PHDRTAB, P_TEXT, P_DATA, P_COMMENT,
                  P_SHSTRTAB, P_SHDRTAB },
    [ET_REL] =  { P_TEXT, P_REL, P_DATA, P_COMMENT,
                  P_SHSTRTAB, P_SYMTAB, P_STRTAB, P_SHDRTAB },
    [ET_DYN] =  { P_PHDRTAB, P_HASH, P_DYNSYM, P_DYNSTR, P_TEXT,
                  P_GOT, P_DYNAMIC, P_DATA, P_COMMENT, P_SHSTRTAB, P_SHDRTAB }
};

/* The name of this program, taken from argv[0]. Used for error messages.
 */
static char const *thisprog;

/* The name of the currently open file. Used for error messages.
 */
static char const *thefilename;

/* The filename to insert in the object file as the source filename.
 */
static char *srcfilename = NULL;

/* The filename to receive the object code.
 */
static char *outfilename = NULL;

/* The name of the function to put the Brainfuck program under.
 */
static char *functionname = NULL;

/* Whether or not to add extra items to the object file.
 */
static bool addextras = true;

/*
 * General-purpose functions.
 */

/* Output a formatted error message on stderr, prefixed by the input
 * filename. If fmt is NULL, then the error message supplied by
 * perror() is used. Returns false.
 */
bool err(char const *fmt, ...)
{
    va_list args;

    if (!fmt) {
	perror(thefilename);
	return false;
    }

    fprintf(stderr, "%s: ", thefilename ? thefilename : thisprog);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    va_end(args);
    return false;
}

/* A wrapper around malloc() that dies if memory is unavailable.
 */
static void *allocate(size_t size)
{
    void *p = malloc(size);
    if (!p) {
        fputs("memory allocation failed\n", stderr);
        exit(EXIT_FAILURE);
    }
    return p;
}

/* A version of strdup() that dies if memory is unavailable.
 */
static char *strallocate(char const *str)
{
    size_t size;
    if (!str)
        return NULL;
    size = strlen(str) + 1;
    return memcpy(allocate(size), str, size);
}

/*
 * The functions that build the ELF file.
 */

/* Initialize connections between parts of the object file.
 */
static void connectparts(blueprint *bp)
{
    if (partispresent(&bp->parts[P_HASH]))
        sethashsyms(bp, P_HASH, P_DYNSYM);
    if (partispresent(&bp->parts[P_DYNSYM]))
        setsymstrings(bp, P_DYNSYM, P_DYNSTR);
    if (partispresent(&bp->parts[P_SYMTAB]))
        setsymstrings(bp, P_SYMTAB, P_STRTAB);
    if (partispresent(&bp->parts[P_REL])) {
        setrelsection(bp, P_REL, P_TEXT);
        setrelsyms(bp, P_REL, P_SYMTAB);
    }
    if (partispresent(&bp->parts[P_SHDRTAB]) &&
                partispresent(&bp->parts[P_SHSTRTAB]))
        setshdrstrs(bp, P_SHDRTAB, P_SHSTRTAB);
}

/* Set the names of the source file, the object file, and the
 * function. Names that were not set explicitly by the user are
 * derived from the name of the input file.
 */
static bool setnames(blueprint const *bp, int codetype, char const *filename)
{
    char const *base;
    int len, i;

    if (!filename || !filename[0])
        return false;
    base = strrchr(filename, '/');
    if (base)
        ++base;
    else
        base = filename;
    len = strlen(base);
    if (!len)
        return false;

    if (!srcfilename)
        srcfilename = strallocate(base);

    if (len > 2 && !memcmp(base + len - 2, ".b", 2))
	len -= 2;
    else if (len > 3 && !memcmp(base + len - 3, ".bf", 3))
        len -= 3;

    if (!outfilename) {
	if (bp->filetype == ET_REL) {
            outfilename = allocate(len + 3);
            sprintf(outfilename, "%*.*s.o", len, len, base);
	} else if (bp->filetype == ET_DYN) {
            outfilename = allocate(len + 7);
	    sprintf(outfilename, "lib%*.*s.so", len, len, base);
        } else if (bp->filetype == ET_EXEC && base[len] != '\0') {
            outfilename = allocate(len + 1);
            memcpy(outfilename, base, len);
            outfilename[len] = '\0';
        } else {
            outfilename = strallocate("a.out");
        }
    }

    if (!functionname) {
	if (codetype == CODE_EXEC) {
            functionname = strallocate("_start");
	} else {
            functionname = allocate(len + 1);
            memcpy(functionname, base, len);
            functionname[len] = '\0';
            if (!isalpha(functionname[0]))
                functionname[0] = '_';
	    for (i = 1 ; i < len ; ++i)
		if (!isalnum(functionname[i]))
		    functionname[i] = '_';
	}
    }

    return true;
}

/* Fill in the comment and text sections of the blueprint.
 */
static bool populateparts(blueprint *bp, int codetype,
                          char const *filename, int compressed)
{
    static char const comment[] = "\0ELF Brainfuck Compiler 2.0";

    if (addextras) {
        if (partispresent(&bp->parts[P_COMMENT])) {
	    bp->parts[P_COMMENT].shname = ".comment";
	    bp->parts[P_COMMENT].size = sizeof comment;
            bp->parts[P_COMMENT].part = allocate(sizeof comment);
            memcpy(bp->parts[P_COMMENT].part, comment, sizeof comment);
	}
        if (partispresent(&bp->parts[P_SYMTAB]))
	    addtosymtab(&bp->parts[P_SYMTAB], srcfilename,
			STB_LOCAL, STT_FILE, SHN_ABS);
    } else {
        if (bp->filetype == ET_EXEC) {
            removepart(&bp->parts[P_SHDRTAB]);
            removepart(&bp->parts[P_SHSTRTAB]);
        }
    }

    thefilename = filename;
    if (!compilebrainfuck(filename, bp, codetype, functionname, compressed))
	return false;
    thefilename = NULL;

    return true;
}

/* The top-level compiling function. Work through the stages of
 * creating the ELF file image, compiling the source code, and writing
 * out the file. The output file also has the executable bits in its
 * mode set accordingly.
 */
static bool compile(blueprint *bp, int codetype,
                    char const *filename, int compressed)
{
    struct stat	s;

    enforcevalidation(true);

    newparts(bp);

    connectparts(bp);
    initparts(bp);

    if (!setnames(bp, codetype, filename))
	return false;
    if (!populateparts(bp, codetype, filename, compressed))
	return false;

    fillparts(bp);
    measureparts(bp);

    if (!createfixups(bp, codetype, functionname))
        return false;

    completeparts(bp);

    thefilename = outfilename;
    if (!outputelf(bp, outfilename)) {
	err(NULL);
	remove(outfilename);
	return false;
    }
    if (stat(outfilename, &s) == 0) {
	if (bp->filetype == ET_EXEC)
	    s.st_mode |= S_IXUSR | S_IXGRP | S_IXOTH;
	else
	    s.st_mode &= ~(S_IXUSR | S_IXGRP | S_IXOTH);
	chmod(outfilename, s.st_mode);
    } else {
	err(NULL);
    }
    thefilename = NULL;

    return true;
}

/* main() uses the command-line options to set up the correct
 * blueprint, and then kicks off the compilation process.
 */
int main(int argc, char *argv[])
{
    static char const *optstring = "acf:i:lo:sxz";
    static struct option const options[] = {
        { "output", required_argument, NULL, 'o' },
        { "input", required_argument, NULL, 'i' },
        { "function", required_argument, NULL, 'f' },
        { "arg", no_argument, NULL, 'a' },
        { "strip", no_argument, NULL, 's' },
        { "help", no_argument, NULL, 'H' },
        { "version", no_argument, NULL, 'V' },
        { 0, 0, 0, 0 }
    };

    blueprint b;
    char const *filename;
    bool buildobjfile, bufferarg, compressed;
    int codetype;
    int ch, i;

    thisprog = argv[0];
    b.filetype = ET_NONE;
    codetype = CODE_NONE;
    buildobjfile = false;
    bufferarg = false;
    compressed = false;
    while ((ch = getopt_long(argc, argv, optstring, options, NULL)) != EOF) {
	switch (ch) {
	  case 'c':	buildobjfile = true;			break;
	  case 'x':	codetype = CODE_EXEC;			break;
	  case 'l':	codetype = CODE_SOFUNC;			break;
	  case 'z':	compressed = true;			break;
	  case 'a':	bufferarg = true;			break;
	  case 's':	addextras = false;			break;
	  case 'f':	functionname = strallocate(optarg);	break;
          case 'i':	srcfilename = strallocate(optarg);	break;
	  case 'o':	outfilename = strallocate(optarg);	break;
	  case 'H':	fputs(yowzitch, stdout);		return 0;
	  case 'V':	fputs(vourzhon, stdout);		return 0;
	  default:
            err("(try \"--help\" for more information)");
            exit(EXIT_FAILURE);
	}
    }
    if (optind + 1 != argc) {
	err(optind == argc ? "no source file specified."
			   : "multiple source files specified.");
	exit(EXIT_FAILURE);
    }
    filename = argv[optind];

    if (buildobjfile) {
        if (!codetype)
            codetype = CODE_FUNC;
        b.filetype = ET_REL;
    } else {
	if (!codetype)
	    codetype = CODE_EXEC;
	b.filetype = codetype == CODE_EXEC ? ET_EXEC : ET_DYN;
    }
    if (bufferarg) {
        if (codetype == CODE_EXEC) {
            err("executable file format cannot take an argument");
            exit(EXIT_FAILURE);
        }
        codetype = CODE_FUNCARG;
    }

    b.partcount = P_COUNT;
    b.parts = allocate(P_COUNT * sizeof *b.parts);
    memset(b.parts, 0, b.partcount * sizeof *b.parts);
    b.parts[0] = *parttable[P_EHDR];
    for (i = 0 ; partlists[b.filetype][i] ; ++i) {
        int type = partlists[b.filetype][i];
        b.parts[type] = *parttable[type];
    }
    if (!addextras)
        removepart(&b.parts[P_COMMENT]);

    return compile(&b, codetype, filename, compressed) ? EXIT_SUCCESS
						       : EXIT_FAILURE;
}
