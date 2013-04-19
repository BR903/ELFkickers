/* names.h: Preprocessor definitions used by the output functions.
 *
 * Copyright (C) 1999-2001 by Brian Raiter, under the GNU General
 * Public License. No warranty. See COPYING for details.
 */

#ifndef	_names_h_
#define	_names_h_

#include	"elf.h"
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
NAMES(eosabi)   = { { _(ELFOSABI_SYSV) }, { _(ELFOSABI_HPUX) },
		    { _(ELFOSABI_ARM) }, { _(ELFOSABI_STANDALONE) } };
NAMES(emachine)	= { { _(EM_NONE) }, { _(EM_M32) }, { _(EM_SPARC) },
		    { _(EM_386) }, { _(EM_68K) }, { _(EM_88K) },
		    { _(EM_486) }, { _(EM_860) }, { _(EM_MIPS) },
		    { _(EM_S370) }, { _(EM_MIPS_RS4_BE) },
		    { _(EM_RS6000) }, { _(EM_PARISC) }, { _(EM_nCUBE) },
		    { _(EM_VPP500) }, { _(EM_SPARC32PLUS) }, { _(EM_960) },
		    { _(EM_PPC) }, { _(EM_PPC64) }, { _(EM_V800) },
		    { _(EM_FR20) }, { _(EM_RH32) }, { _(EM_MMA) },
		    { _(EM_ARM) }, { _(EM_FAKE_ALPHA) }, { _(EM_SH) },
		    { _(EM_SPARCV9) }, { _(EM_TRICORE) }, { _(EM_ARC) },
		    { _(EM_H8_300) }, { _(EM_H8_300H) }, { _(EM_H8S) },
		    { _(EM_H8_500) }, { _(EM_IA_64) }, { _(EM_MIPS_X) },
		    { _(EM_COLDFIRE) }, { _(EM_68HC12) }, { _(EM_MMA) },
		    { _(EM_PCP) }, { _(EM_NCPU) }, { _(EM_NDR1) },
		    { _(EM_STARCORE) }, { _(EM_ME16) }, { _(EM_ST100) },
		    { _(EM_TINYJ) }, { _(EM_X8664) }, { _(EM_FX66) },
		    { _(EM_ST9PLUS) }, { _(EM_ST7) }, { _(EM_68HC16) },
		    { _(EM_68HC11) }, { _(EM_68HC08) }, { _(EM_68HC05) },
		    { _(EM_SVX) }, { _(EM_AT19) }, { _(EM_VAX) },
		    { _(EM_ALPHA) }, { _(EM_S390) } };
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
		    { _(SHT_GNU_verdef) }, { _(SHT_GNU_verneed) },
		    { _(SHT_GNU_versym) }, { _(SHT_LOOS), SHT_HIOS },
		    { _(SHT_LOPROC), SHT_HIPROC },
		    { _(SHT_LOUSER), SHT_HIUSER } };
NAMES(shflags)	= { { _(SHF_WRITE) }, { _(SHF_ALLOC) }, { _(SHF_EXECINSTR) },
		    { _(SHF_MERGE) }, { _(SHF_STRINGS) }, { _(SHF_INFO_LINK) },
		    { _(SHF_LINK_ORDER) }, { _(SHF_OS_NONCONFORMING) } };

/* Preprocesser symbols used in the program header table.
 */
NAMES(ptype)	= { { _(PT_NULL) }, { _(PT_LOAD) }, { _(PT_DYNAMIC) },
		    { _(PT_INTERP) }, { _(PT_NOTE) }, { _(PT_SHLIB) },
		    { _(PT_PHDR) }, { _(PT_LOOS), PT_HIOS },
		    { _(PT_LOPROC), PT_HIPROC } };
NAMES(pflags)	= { { _(PF_R) }, { _(PF_W) }, { _(PF_X) } };

/* Preprocessor symbols used in the symbol tables.
 */
NAMES(symtype)	= { { _(STT_NOTYPE) }, { _(STT_OBJECT) }, { _(STT_FUNC) },
		    { _(STT_SECTION) }, { _(STT_FILE) },
		    { _(STT_LOOS), STT_HIOS }, { _(STT_LOPROC), STT_HIPROC } };
NAMES(symbind)	= { { _(STB_LOCAL) }, { _(STB_GLOBAL) }, { _(STB_WEAK) },
		    { _(STB_LOOS), STB_HIOS }, { _(STB_LOPROC), STB_HIPROC } };

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
		    { _(DT_BIND_NOW) }, { _(DT_INIT_ARRAY) },
		    { _(DT_FINI_ARRAY) }, { _(DT_INIT_ARRAYSZ) },
		    { _(DT_FINI_ARRAYSZ) }, { _(DT_RUNPATH) }, { _(DT_FLAGS) },
		    { _(DT_ENCODING) }, { _(DT_PREINIT_ARRAY) },
		    { _(DT_PREINIT_ARRAYSZ) }, { _(DT_PLTPADSZ) },
		    { _(DT_MOVEENT) }, { _(DT_MOVESZ) }, { _(DT_FEATURE_1) },
		    { _(DT_POSFLAG_1) }, { _(DT_SYMINSZ) }, { _(DT_SYMINENT) },
		    { _(DT_SYMINFO) }, { _(DT_VERSYM) }, { _(DT_RELACOUNT) },
		    { _(DT_RELCOUNT) }, { _(DT_FLAGS_1) }, { _(DT_VERDEF) },
		    { _(DT_VERDEFNUM) }, { _(DT_VERNEED) },
		    { _(DT_VERNEEDNUM) }, { _(DT_AUXILIARY) },
		    { _(DT_FILTER) },
		    { _(DT_LOOS), DT_HIOS }, { _(DT_VALRNGLO), DT_VALRNGHI },
		    { _(DT_ADDRRNGLO), DT_ADDRRNGHI },
		    { _(DT_LOPROC), DT_HIPROC } };
NAMES(dynflag)  = { { _(DF_ORIGIN) }, { _(DF_SYMBOLIC) }, { _(DF_SYMBOLIC) },
		    { _(DF_TEXTREL) }, { _(DF_BIND_NOW) } };
NAMES(dynflag1) = { { _(DF_1_NOW) }, { _(DF_1_GLOBAL) }, { _(DF_1_GROUP) },
		    { _(DF_1_NODELETE) }, { _(DF_1_LOADFLTR) },
		    { _(DF_1_INITFIRST) }, { _(DF_1_NOOPEN) },
		    { _(DF_1_ORIGIN) }, { _(DF_1_DIRECT) }, { _(DF_1_TRANS) },
		    { _(DF_1_INTERPOSE) }, { _(DF_1_NODEFLIB) },
		    { _(DF_1_NODUMP) }, { _(DF_1_CONFALT) },
		    { _(DF_1_ENDFILTEE) } };
NAMES(dynftrf1) = { { _(DTF_1_PARINIT) }, { _(DTF_1_CONFEXP) } };
NAMES(dynposf1) = { { _(DF_P1_LAZYLOAD) }, { _(DF_P1_GROUPPERM) } };

#undef NAMES
#undef _

#endif
