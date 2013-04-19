/* dynamic.c: Map of the dynamic section's tags to small numbers.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	"gen.h"
#include	"elf.h"
#include	"dynamic.h"

static Elf32_Sword dyntags[N_DT_COUNT];
static int dyntagsinitialized = FALSE;

#define	init(tag)	dyntags[N_##tag] = tag;

int getdyntagid(Elf32_Sword tag)
{
    int n;

    if (!dyntagsinitialized) {
	init(DT_AUXILIARY);		init(DT_PREINIT_ARRAYSZ);
	init(DT_BIND_NOW);		init(DT_REL);
	init(DT_DEBUG);			init(DT_RELA);
	init(DT_ENCODING);		init(DT_RELACOUNT);
	init(DT_FEATURE_1);		init(DT_RELAENT);
	init(DT_FILTER);		init(DT_RELASZ);
	init(DT_FINI);			init(DT_RELCOUNT);
	init(DT_FINI_ARRAY);		init(DT_RELENT);
	init(DT_FINI_ARRAYSZ);		init(DT_RELSZ);
	init(DT_FLAGS);			init(DT_RPATH);
	init(DT_FLAGS_1);		init(DT_RUNPATH);
	init(DT_HASH);			init(DT_SONAME);
	init(DT_INIT);			init(DT_STRSZ);
	init(DT_INIT_ARRAY);		init(DT_STRTAB);
	init(DT_INIT_ARRAYSZ);		init(DT_SYMBOLIC);
	init(DT_JMPREL);		init(DT_SYMENT);
	init(DT_MOVEENT);		init(DT_SYMINENT);
	init(DT_MOVESZ);		init(DT_SYMINFO);
	init(DT_NEEDED);		init(DT_SYMINSZ);
	init(DT_NULL);			init(DT_SYMTAB);
	init(DT_PLTGOT);		init(DT_TEXTREL);
	init(DT_PLTPADSZ);		init(DT_VERDEF);
	init(DT_PLTREL);		init(DT_VERDEFNUM);
	init(DT_PLTRELSZ);		init(DT_VERNEED);
	init(DT_POSFLAG_1);		init(DT_VERNEEDNUM);
	init(DT_PREINIT_ARRAY);		init(DT_VERSYM);
	dyntagsinitialized = TRUE;
    }

    for (n = 0 ; n < N_DT_COUNT ; ++n)
	if (dyntags[n] == tag)
	    return n;

    return 0;
}
