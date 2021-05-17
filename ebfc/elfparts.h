/* elfparts.h: Definitions and functions supplied by the elfparts
 * library.
 *
 * Copyright (C) 1999,2001,2021 by Brian Raiter, under the GNU General
 * Public License, version 2. No warranty. See COPYING for details.
 */

#ifndef	_elfparts_h_
#define	_elfparts_h_

#include <stdint.h>
#include <stdbool.h>

/* The global offset table has its own special symbol table entry.
 */
#define	NAME_GOT "_GLOBAL_OFFSET_TABLE_"

/*
 * Data structures of the elfparts library.
 */

typedef	struct blueprint blueprint;
typedef	struct elfpart elfpart;

/* The information used to specify the contents and structure of the
 * ELF file.
 */
struct blueprint {
    int		filetype;	/* the type of file to make */
    int		partcount;	/* the number of parts */
    elfpart    *parts;		/* the parts list */
};

/* The information used to specify the contents of one part of an ELF
 * file. The complete array of elfparts make up the file's blueprint.
 * The caller will typically not modify these fields directly (except
 * in a few cases), but is free to examine them. For free-form parts
 * where the caller supplies the contents (text, data, etc), the
 * caller is expected to modify size and part appropriately. The
 * caller should also feel free to change the shname field as needed.
 */
struct elfpart {
    /* These four functions are what makes each type of part
     * different; they are supplied by the elfparts library and are
     * invoked internally. Each of these functions returns false if an
     * unrecoverable error occurs. For init(), fill(), and complete(),
     * a function sets the done field to true when they are finished.
     * If a function cannot finish because its part is dependent on
     * another part that isn't finished, then it returns without
     * setting the done field, and it will be reinvoked again later.
     */
    bool	  (*new)(elfpart*);
    bool	  (*init)(elfpart*, blueprint const*);
    bool	  (*fill)(elfpart*, blueprint const*);
    bool	  (*complete)(elfpart*, blueprint const*);
    int		    shtype;	/* the section header entry type */
    int		    entsize;	/* the size of the part's entries */
    int		    count;	/* the number of the part's entries */
    int		    size;	/* the total size of the contents */
    void	   *part;	/* buffer of the actual contents */
    char const	   *shname;	/* the section header entry name */
    size_t          offset;	/* the part's location in the file */
    intptr_t	    addr;	/* the part's load address */
    elfpart	   *link;	/* another part that this one uses */
    int		    info;	/* an extra, multipurpose field */
    short	    flags;	/* memory read-write-exec flags */
    bool	    done;	/* true when this part is done */
};

/* The available types of parts that are provided by this library. The
 * caller creates a blueprint by copying one of these objects into
 * each element of their parts array. The first part must always be a
 * part_ehdr.
 */
extern struct elfpart
    part_empty,                 /* an empty area */
    part_ehdr,                  /* the ELF header */
    part_phdrtab,               /* the program segment header table */
    part_shdrtab,               /* the section header table */
    part_strtab,                /* a string table */
    part_dynstr,                /* the dynamic string table */
    part_shstrtab,              /* the section header string table */
    part_symtab,                /* a symbol table */
    part_dynsym,                /* the dynamic symbol table */
    part_rel,                   /* a relocation table */
    part_rela,                  /* a relocation table with addends */
    part_got,                   /* the global offset table */
    part_hash,                  /* the hash table */
    part_dynamic,               /* the dynamic section */
    part_progbits,              /* a generic loadable section */
    part_text,                  /* a loadable section of code */
    part_rodata,                /* a loadable section of read-only data */
    part_data;                  /* a loadable section of writable data */

/*
 * The API of the library. For all functions with boolean return
 * values, the return value indicates success or failure; however, a
 * function will only return false if enforcevalidation() has been
 * called with a false value. See that function for more details.
 */

/* Select how the library should handle encountering an invalid state.
 * If flag is true, then an invalid state will cause the program to
 * exit after outputting an error message output to stderr. If flag is
 * false, then an invalid state will only result in a false return
 * value from the API function. In the latter case, an error message
 * will be stored internally, and can be retrieved by the caller using
 * getlasterror().
 */
extern void enforcevalidation(bool flag);

/* Return the text of a message describing the most recent error. This
 * function should only be used after a function has returned a false
 * value to indicate an invalid state.
 */
extern char const *getlasterror(void);

/* Mark a part of a blueprint as being removed. Removal allows the
 * caller to create blueprints with a superset of parts, and then
 * remove the unneeded parts from the final version of the blueprint
 * as necessary. Note that a part cannot be removed from a blueprint
 * after calling fillparts().
 */
#define removepart(part) ((part)->shtype = 0)

/* Return true if a blueprint part has not been removed.
 */
#define partispresent(part) ((part)->shtype != 0)

/* A macro for creating a for-loop that iterates over the parts of a
 * blueprint. (The "in" macro, used with the extra layer of
 * evaluation, is pure syntactic sugar.)
 */
#define foreachpart(x) _foreachpart(x)
#define in ,
#define _foreachpart(p, b) \
    for (elfpart *(p) = (b)->parts ; (p) - (b)->parts < (b)->partcount ; ++(p))

/*
 * The five main stages of blueprint building. Each of these functions
 * must be called in the process of building up an ELF file image. The
 * process is broken up into stages to allow the caller to insert
 * their own content at any point in the process.
 */

/* Calls new for all the parts in the blueprint's part list. This
 * function should be called once the part list has been created,
 * before making any modifications to the parts themselves.
 */
extern bool newparts(blueprint *bp);

/* Calls init for all the parts in the blueprint's part list. This
 * function should be called after the caller has tailored the
 * information in the elfpart structures (e.g., the link and info
 * fields), but before using any of the part-specific functions listed
 * below.
 */
extern bool initparts(blueprint *bp);

/* Calls fill for all the parts in the blueprint's part list. This
 * function should be called after the caller has finished adding to
 * (or removing from) the parts. When this function returns, every
 * part will have determined its final size (but not necessarily its
 * actual contents).
 */
extern bool fillparts(blueprint *bp);

/* Fills in the values for the addr and offset fields for all the
 * parts in the blueprint's part list. (Unlike the other four stages,
 * this function always succeeds.)
 */
extern void measureparts(blueprint *bp);

/* Calls complete for all the parts in the blueprint's part list. This
 * function should be called when the caller has completely finished
 * altering the contents of the parts. When this function returns, the
 * parts will be in their final state, and may be written out.
 */
extern bool completeparts(blueprint *bp);

/*
 * Functions to connect parts to each other. These functions should be
 * called after newparts() and before initparts().
 */

/* Set the part containing the section header string table.
 */
extern bool setshdrstrs(blueprint *bp, int shdrsindex, int strsindex);

/* Set the string table associated with a symbol table.
 */
extern bool setsymstrings(blueprint *bp, int symsindex, int strsindex);

/* Set the hash table's symbol table.
 */
extern bool sethashsyms(blueprint *bp, int hashindex, int symsindex);

/* Set the symbol table associated with a rel or rela section.
 */
extern bool setrelsyms(blueprint *bp, int relindex, int symsindex);

/* Set the section that a rel or rela section modifies.
 */
extern bool setrelsection(blueprint *bp, int relindex, int sectindex);

/*
 * Functions for adding content to blueprint parts. Unless otherwise
 * indicated, these functions should be called after using initparts()
 * and before calling fillparts().
 */

/* Add a string to a string table. The return value is the index of
 * the string in the table.
 */
extern int addtostrtab(elfpart *part, char const *str);

/* Add a symbol to a symbol table. The symbol's value is initialized
 * to zero; the other data associated with the symbol are supplied by
 * the function's arguments. The symbol's name is automatically added
 * to the appropriate string table. The return value is an index of
 * the symbol in the table (see below for comments on the index
 * value). If the symbol's binding is local, then the returned index
 * will actually be a negative number, indicating a placeholder value
 * (since its actual value cannot be determined until after
 * fillparts() is invoked).
 */
extern int addtosymtab(elfpart *part, char const *str,
		       int bind, int type, int shndx);

/* Look up a symbol that is already in the given symbol table. The
 * return value is the index of the symbol, or zero if the symbol is
 * not in the table. The index will be negative for local symbols,
 * since the true index cannot be determined until all the global
 * symbols have been added. This function can be used after
 * fillparts() has been called, whereupon a negative index can be
 * converted to a real index by subtracting it from the value in the
 * symbol table's info field, less one.
 */
extern int getsymindex(elfpart const *part, char const *name);

/* Set the value of a symbol in a symbol table. The return value is
 * false if the given symbol could not be found in the table. Note
 * that it is safe to use this function after fillparts().
 */
extern bool setsymvalue(elfpart *part, char const *name, intptr_t value);

/* Add an entry to a relocation section. The return value is the index
 * of the entry.
 */
extern int addentrytorel(elfpart *part, unsigned int offset,
                         int sym, int type);

/* Add an entry to a relocation section. The given symbol is added to
 * the appropriate symbol table if it is not already present. (If the
 * symbol is already present, its data is not changed, and the last
 * three arguments are ignored.) The return value is the index of the
 * entry.
 */
extern int addsymbolrel(elfpart *part, unsigned int offset, int reltype,
                        char const *sym, int symbind, int symtype,
                        int shndx);

/*
 * Functions for use after fillparts() has been invoked.
 */

/* Set the entry point for an executable ELF. The addr argument needs
 * to be an actual address, so it should be used after measureparts().
 */
extern void setexecentry(blueprint *bp, intptr_t addr);

/* Output the bluepint contents as an ELF image to the given filename.
 * A false return value indicates an I/O error (in which case errno is
 * set appropriately). This function should be used after
 * completeparts().
 */
extern bool outputelf(blueprint const *bp, char const *filename);

#endif
