/* dynamic.c: Mapping the dynamic section's tags to small numbers.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#ifndef	_dynamic_h_
#define	_dynamic_h_

enum {
    N_DT_NULL, N_DT_NEEDED, N_DT_PLTRELSZ, N_DT_PLTGOT, N_DT_HASH, N_DT_STRTAB,
    N_DT_SYMTAB, N_DT_RELA, N_DT_RELASZ, N_DT_RELAENT, N_DT_STRSZ,
    N_DT_SYMENT, N_DT_INIT, N_DT_FINI, N_DT_SONAME, N_DT_RPATH, N_DT_SYMBOLIC,
    N_DT_REL, N_DT_RELSZ, N_DT_RELENT, N_DT_PLTREL, N_DT_DEBUG, N_DT_TEXTREL,
    N_DT_JMPREL, N_DT_BIND_NOW, N_DT_INIT_ARRAY, N_DT_FINI_ARRAY,
    N_DT_INIT_ARRAYSZ, N_DT_FINI_ARRAYSZ, N_DT_RUNPATH, N_DT_FLAGS,
    N_DT_ENCODING, N_DT_PREINIT_ARRAY, N_DT_PREINIT_ARRAYSZ, N_DT_PLTPADSZ,
    N_DT_MOVEENT, N_DT_MOVESZ, N_DT_FEATURE_1, N_DT_POSFLAG_1, N_DT_SYMINSZ,
    N_DT_SYMINENT, N_DT_SYMINFO, N_DT_VERSYM, N_DT_RELACOUNT, N_DT_RELCOUNT,
    N_DT_FLAGS_1, N_DT_VERDEF, N_DT_VERDEFNUM, N_DT_VERNEED, N_DT_VERNEEDNUM,
    N_DT_AUXILIARY, N_DT_FILTER,
    N_DT_COUNT
};

/* Translates a tag from the dynamic section into the corresponding
 * enum value.
 */
extern int getdyntagid(int tag);

#endif
