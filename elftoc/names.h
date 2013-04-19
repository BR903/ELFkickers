/* names.h: Preprocessor definitions used by the output functions.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#ifndef	_names_h_
#define	_names_h_

#include	<linux/elf.h>
#include	"outtools.h"

#define	NAMES(rn)	static names const rn##_names []
#define	_(token)	#token, token

/* Preprocessor symbols used in the ELF header.
 */
NAMES(eclass)	= { { _(ELFCLASSNONE) }, { _(ELFCLASS32) },
		    { _(ELFCLASS64) } };
NAMES(edata)	= { { _(ELFDATANONE) }, { _(ELFDATA2LSB) },
		    { _(ELFDATA2MSB) } };
NAMES(eversion)	= { { _(EV_NONE) }, { _(EV_CURRENT) } };
NAMES(emachine)	= { { "EM_NONE", 0 }, { "EM_M32", 1 }, { "EM_SPARC", 2 },
		    { "EM_386", 3 }, { "EM_68K", 4 }, { "EM_88K", 5 },
		    { "EM_486", 6 }, { "EM_860", 7 }, { "EM_MIPS", 8 },
		    { "EM_MIPS_RS4_BE", 10 }, { "EM_SPARC64", 11 },
		    { "EM_PARISC", 15 }, { "EM_SPARC32PLUS", 18 },
		    { "EM_PPC", 20 }, { "EM_ALPHA", 0x9026 } };
NAMES(elftype)	= { { _(ET_NONE) }, { _(ET_REL) }, { _(ET_EXEC) },
		    { _(ET_DYN) }, { _(ET_CORE) },
		    { _(ET_LOPROC), ET_HIPROC } };

/* Preprocessor symbols used in the section header table.
 */
NAMES(section)	= { { _(SHN_UNDEF) }, { _(SHN_ABS) }, { _(SHN_COMMON) },
		    { _(SHN_LOPROC), SHN_HIPROC },
		    { _(SHN_LORESERVE), SHN_HIRESERVE } };
NAMES(shtype)	= { { _(SHT_NULL) }, { _(SHT_PROGBITS) }, { _(SHT_SYMTAB) },
		    { _(SHT_STRTAB) }, { _(SHT_RELA) }, { _(SHT_HASH) },
		    { _(SHT_DYNAMIC) }, { _(SHT_NOTE) }, { _(SHT_NOBITS) },
		    { _(SHT_REL) }, { _(SHT_SHLIB) }, { _(SHT_DYNSYM) },
		    { _(SHT_LOPROC), SHT_HIPROC },
		    { _(SHT_LOUSER), SHT_HIUSER } };
NAMES(shflags)	= { { _(SHF_WRITE) }, { _(SHF_ALLOC) }, { _(SHF_EXECINSTR) } };

/* Macros used in the program header table.
 */
NAMES(ptype)	= { { _(PT_NULL) }, { _(PT_LOAD) }, { _(PT_DYNAMIC) },
		    { _(PT_INTERP) }, { _(PT_NOTE) }, { _(PT_SHLIB) },
		    { _(PT_PHDR) }, { _(PT_LOPROC), PT_HIPROC } };
NAMES(pflags)	= { { _(PF_R) }, { _(PF_W) }, { _(PF_X) } };

/* Preprocessor symbols used in the symbol tables.
 */
NAMES(symtype)	= { { _(STT_NOTYPE) }, { _(STT_OBJECT) }, { _(STT_FUNC) },
		    { _(STT_SECTION) }, { _(STT_FILE) },
		    { "STT_LOPROC", 13, 15 } };
NAMES(symbind)	= { { _(STB_LOCAL) }, { _(STB_GLOBAL) }, { _(STB_WEAK) },
		    { "STB_LOPROC", 13, 15 } };

/* Preprocessor symbols used in the relocation sections.
 */
NAMES(reltype)	= { { _(R_386_NONE) }, { _(R_386_32) }, { _(R_386_PC32) },
		    { _(R_386_GOT32) }, { _(R_386_PLT32) },{ _(R_386_COPY) },
		    { _(R_386_GLOB_DAT) }, { _(R_386_JMP_SLOT) },
		    { _(R_386_RELATIVE) }, { _(R_386_GOTOFF) },
		    { _(R_386_GOTPC) } };

/* Preprocessor symbols used in the dynamic section.
 */
NAMES(dyntag)	= { { _(DT_NULL) }, { _(DT_NEEDED) }, { _(DT_PLTRELSZ) },
		    { _(DT_PLTGOT) }, { _(DT_HASH) }, { _(DT_STRTAB) },
		    { _(DT_SYMTAB) }, { _(DT_RELA) }, { _(DT_RELASZ) },
		    { _(DT_RELAENT) }, { _(DT_STRSZ) }, { _(DT_SYMENT) },
		    { _(DT_INIT) }, { _(DT_FINI) }, { _(DT_SONAME) },
		    { _(DT_RPATH) }, { _(DT_SYMBOLIC) }, { _(DT_REL) },
		    { _(DT_RELSZ) }, { _(DT_RELENT) }, { _(DT_PLTREL) },
		    { _(DT_DEBUG) }, { _(DT_TEXTREL) }, { _(DT_JMPREL) },
		    { _(DT_LOPROC), DT_HIPROC } };

#undef NAMES
#undef _

#endif
