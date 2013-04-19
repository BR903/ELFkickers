/* elf.h: definitions of ELF structures and symbols
 *
 * The usage of ELF under Linux is currently in flux, even ignoring
 * architectures beyond the x86. As a result, I have found it hard to
 * make use of the usual header files <elf.h> and <linux/elf.h> in
 * this program, as newer symbols do not appear in the header files
 * associated with earlier versions of the kernel and/or compiler.
 * Furthermore, the two header files are incompatible with each other,
 * as they have numerous overlapping (yet occasionally different)
 * typedefs, yet a number of definitions appear in one file but not
 * the other. To avoid this mess, elftoc uses this include file
 * instead. Where possible, the definitions are taken verbatim from
 * the original ELF standard itself. Other definitions are extracted
 * from recent versions of the aforementioned system header files.
 *
 * By using this header file instead, elftoc (and the source code it
 * generates) is made much more independent of the details of the
 * system it was originally compiled on.
 */

#ifndef	_elftoc_elf_h_
#define	_elftoc_elf_h_

/*
 * The platform-dependent typedefs.
 */

typedef	unsigned int	Elf32_Addr;
typedef	unsigned short	Elf32_Half;
typedef	unsigned int	Elf32_Off;
typedef	unsigned int	Elf32_Word;
typedef	signed int	Elf32_Sword;

/*
 * Definitions taken from the ELF standard.
 */

/* Figure 1-3: ELF Header
 */
#define EI_NIDENT       16

typedef struct {
    unsigned char       e_ident[EI_NIDENT];
    Elf32_Half          e_type;
    Elf32_Half          e_machine;
    Elf32_Word          e_version;
    Elf32_Addr          e_entry;
    Elf32_Off           e_phoff;
    Elf32_Off           e_shoff;
    Elf32_Word          e_flags;
    Elf32_Half          e_ehsize;
    Elf32_Half          e_phentsize;
    Elf32_Half          e_phnum;
    Elf32_Half          e_shentsize;
    Elf32_Half          e_shnum;
    Elf32_Half          e_shstrndx;
} Elf32_Ehdr;

#define ET_NONE         0  /* No file type */
#define ET_REL          1  /* Relocatable file */
#define ET_EXEC         2  /* Executable file */
#define ET_DYN          3  /* Shared object file */
#define ET_CORE         4  /* Core file */
#define ET_LOPROC  0xff00  /* Processor-specific */
#define ET_HIPROC  0xffff  /* Processor-specific */

#define EM_NONE       0  /* No machine */
#define EM_M32        1  /* AT&T WE 32100 */
#define EM_SPARC      2  /* SPARC */
#define EM_386        3  /* Intel 80386 */
#define EM_68K        4  /* Motorola 68000 */
#define EM_88K        5  /* Motorola 88000 */
#define EM_860        7  /* Intel 80860 */
#define EM_MIPS       8  /* MIPS RS3000 */

#define EV_NONE          0  /* Invalid version */
#define EV_CURRENT       1  /* Current version */

/* Figure 1-4: e_ident[] Identification Indexes
 */
#define EI_MAG0	     0  /* File identification */
#define EI_MAG1	     1  /* File identification */
#define EI_MAG2	     2  /* File identification */
#define EI_MAG3	     3  /* File identification */
#define EI_CLASS     4  /* File class */
#define EI_DATA	     5  /* Data encoding */
#define EI_VERSION   6  /* File version */
#define EI_PAD	     7  /* Start of padding bytes */

#define ELFMAG0    0x7f   /* e_ident[EI_MAG0] */
#define ELFMAG1    'E'    /* e_ident[EI_MAG1] */
#define ELFMAG2    'L'    /* e_ident[EI_MAG2] */
#define ELFMAG3    'F'    /* e_ident[EI_MAG3] */

#define ELFCLASSNONE       0  /* Invalid class */
#define ELFCLASS32         1  /* 32-bit objects */
#define ELFCLASS64         2  /* 64-bit objects */

#define ELFDATANONE        0  /* Invalid data encoding */
#define ELFDATA2LSB        1  /* little-endian */
#define ELFDATA2MSB        2  /* big-endian */

/* Figure 1-8: Special Section Indexes
 */
#define SHN_UNDEF            0
#define SHN_LORESERVE   0xff00
#define SHN_LOPROC      0xff00
#define SHN_HIPROC      0xff1f
#define SHN_ABS         0xfff1
#define SHN_COMMON      0xfff2
#define SHN_HIRESERVE   0xffff

/* Figure 1-9: Section Header
 */
typedef struct {
    Elf32_Word        sh_name;
    Elf32_Word        sh_type;
    Elf32_Word        sh_flags;
    Elf32_Addr        sh_addr;
    Elf32_Off         sh_offset;
    Elf32_Word        sh_size;
    Elf32_Word        sh_link;
    Elf32_Word        sh_info;
    Elf32_Word        sh_addralign;
    Elf32_Word        sh_entsize;
} Elf32_Shdr;

/* Figure 1-10: Section Types, sh_type
 */
#define SHT_NULL               0
#define SHT_PROGBITS           1
#define SHT_SYMTAB             2
#define SHT_STRTAB	       3
#define SHT_RELA	       4
#define SHT_HASH	       5
#define SHT_DYNAMIC            6
#define SHT_NOTE	       7
#define SHT_NOBITS	       8
#define SHT_REL		       9
#define SHT_SHLIB             10
#define SHT_DYNSYM            11
#define SHT_LOPROC    0x70000000
#define SHT_HIPROC    0x7fffffff
#define SHT_LOUSER    0x80000000
#define SHT_HIUSER    0xffffffff

/* Figure 1-12: Section Attribute Flags, sh_flags
 */
#define SHF_WRITE             0x1
#define SHF_ALLOC             0x2
#define SHF_EXECINSTR         0x4
#define SHF_MASKPROC   0xf0000000

#define	STN_UNDEF		0

/* Figure 1-16: Symbol Table Entry
 */
typedef struct {
    Elf32_Word		st_name;
    Elf32_Addr		st_value;
    Elf32_Word		st_size;
    unsigned char	st_info;
    unsigned char	st_other;
    Elf32_Half		st_shndx;
} Elf32_Sym;

#define ELF32_ST_BIND(i)	((i)>>4)
#define ELF32_ST_TYPE(i)	((i)&0xf)
#define ELF32_ST_INFO(b, t)	(((b)<<4)+((t)&0xf))

/* Figure 1-17: Symbol Binding, ELF32_ST_BIND
 */
#define STB_LOCAL       0
#define STB_GLOBAL      1
#define STB_WEAK        2
#define STB_LOPROC     13
#define STB_HIPROC     15

/* Figure 1-18: Symbol Types, ELF32_ST_TYPE
 */
#define STT_NOTYPE       0
#define STT_OBJECT       1
#define STT_FUNC         2
#define STT_SECTION      3
#define STT_FILE         4
#define STT_LOPROC      13
#define STT_HIPROC      15

/* Figure 1-20: Relocation Entries
 */
typedef struct {
    Elf32_Addr		r_offset;
    Elf32_Word		r_info;
} Elf32_Rel;

typedef struct {
    Elf32_Addr		r_offset;
    Elf32_Word		r_info;
    Elf32_Sword		r_addend;
} Elf32_Rela;

#define ELF32_R_SYM(i)		((i)>>8)
#define ELF32_R_TYPE(i)		((unsigned char)(i))
#define ELF32_R_INFO(s, t)	(((s)<<8)+(unsigned char)(t))

/* Figure 1-22: Relocation Types
 */
#define R_386_NONE          0
#define R_386_32	    1
#define R_386_PC32	    2
#define R_386_GOT32	    3
#define R_386_PLT32	    4
#define R_386_COPY	    5
#define R_386_GLOB_DAT      6
#define R_386_JMP_SLOT      7
#define R_386_RELATIVE      8
#define R_386_GOTOFF	    9
#define R_386_GOTPC	   10

/* Figure 2-1: Program Header
 */
typedef struct {
    Elf32_Word        p_type;
    Elf32_Off         p_offset;
    Elf32_Addr        p_vaddr;
    Elf32_Addr        p_paddr;
    Elf32_Word        p_filesz;
    Elf32_Word        p_memsz;
    Elf32_Word        p_flags;
    Elf32_Word        p_align;
} Elf32_Phdr;

#define PT_NULL              0
#define PT_LOAD              1
#define PT_DYNAMIC           2
#define PT_INTERP            3
#define PT_NOTE              4
#define PT_SHLIB             5
#define PT_PHDR              6
#define PT_LOPROC   0x70000000
#define PT_HIPROC   0x7fffffff

/* Figure 2-9: Dynamic Structure
 */
typedef struct {
    Elf32_Sword d_tag;
    union {
        Elf32_Sword	d_val;
        Elf32_Addr	d_ptr;
    } d_un;
} Elf32_Dyn;

#define DT_NULL                  0
#define DT_NEEDED		 1
#define DT_PLTRELSZ		 2
#define DT_PLTGOT		 3
#define DT_HASH		 	 4
#define DT_STRTAB		 5
#define DT_SYMTAB		 6
#define DT_RELA		 	 7
#define DT_RELASZ		 8
#define DT_RELAENT		 9
#define DT_STRSZ		10
#define DT_SYMENT		11
#define DT_INIT			12
#define DT_FINI			13
#define DT_SONAME		14
#define DT_RPATH		15
#define DT_SYMBOLIC		16
#define DT_REL			17
#define DT_RELSZ		18
#define DT_RELENT		19
#define DT_PLTREL		20
#define DT_DEBUG		21
#define DT_TEXTREL		22
#define DT_JMPREL		23
#define DT_LOPROC       0x70000000
#define DT_HIPROC       0x7fffffff

/*
 * Additional definitions from <elf.h> and <linux/elf.h>.
 */

#undef EI_PAD

#define EI_OSABI	7		/* OS ABI identification */
#define EI_ABIVERSION	8		/* ABI version */
#define EI_PAD		9		/* Byte index of padding bytes */

#define ELFOSABI_SYSV		0	/* UNIX System V ABI */
#define ELFOSABI_HPUX		1	/* HP-UX */
#define ELFOSABI_ARM		97	/* ARM */
#define ELFOSABI_STANDALONE	255	/* Standalone (embedded) application */

#define EM_486           6              /* Perhaps disused */
#define EM_S370		 9		/* Amdahl */
#define EM_MIPS_RS3_LE	10		/* MIPS R3000 little-endian */
#define EM_MIPS_RS4_BE	10		/* MIPS R4000 big-endian */
#define EM_RS6000	11		/* RS6000 */

#define EM_PARISC	15		/* HPPA */
#define EM_nCUBE	16		/* nCUBE */
#define EM_VPP500	17		/* Fujitsu VPP500 */
#define EM_SPARC32PLUS	18		/* Sun's "v8plus" */
#define EM_960		19		/* Intel 80960 */
#define EM_PPC		20		/* PowerPC */
#define EM_PPC64	21		/* PowerPC 64-bit */

#define EM_V800		36		/* NEC V800 series */
#define EM_FR20		37		/* Fujitsu FR20 */
#define EM_RH32		38		/* TRW RH32 */
#define EM_MMA		39		/* Fujitsu MMA */
#define EM_ARM		40		/* ARM */
#define EM_FAKE_ALPHA	41		/* Digital Alpha */
#define EM_SH		42		/* Hitachi SH */
#define EM_SPARCV9	43		/* SPARC v9 64-bit */
#define EM_TRICORE	44		/* Siemens Tricore */
#define EM_ARC		45		/* Argonaut RISC Core */
#define EM_H8_300	46		/* Hitachi H8/300 */
#define EM_H8_300H	47		/* Hitachi H8/300H */
#define EM_H8S		48		/* Hitachi H8S */
#define EM_H8_500	49		/* Hitachi H8/500 */
#define EM_IA_64	50		/* Intel Merced */
#define EM_MIPS_X	51		/* Stanford MIPS-X */
#define EM_COLDFIRE	52		/* Motorola Coldfire */
#define EM_68HC12	53		/* Motorola M68HC12 */
/* #define EM_MMA          54              Fujitsu MMA Multimedia Accelerator*/
#define EM_PCP		55		/* Siemens PCP */
#define EM_NCPU		56		/* Sony nCPU embeeded RISC */
#define EM_NDR1		57		/* Denso NDR1 microprocessor */
#define EM_STARCORE	58		/* Motorola Start*Core processor */
#define EM_ME16		59		/* Toyota ME16 processor */
#define EM_ST100	60		/* STMicroelectronic ST100 processor */
#define EM_TINYJ	61		/* Advanced Logic Corp. Tinyj emb.fam*/
#define EM_X8664	62		/* AMD x86-64 */

#define EM_FX66		66		/* Siemens FX66 microcontroller */
#define EM_ST9PLUS	67		/* STMicroelectronics ST9+ 8/16 mc */
#define EM_ST7		68		/* STmicroelectronics ST7 8 bit mc */
#define EM_68HC16	69		/* Motorola MC68HC16 microcontroller */
#define EM_68HC11	70		/* Motorola MC68HC11 microcontroller */
#define EM_68HC08	71		/* Motorola MC68HC08 microcontroller */
#define EM_68HC05	72		/* Motorola MC68HC05 microcontroller */
#define EM_SVX		73		/* Silicon Graphics SVx */
#define EM_AT19		74		/* STMicroelectronics ST19 8 bit mc */
#define EM_VAX		75		/* Digital VAX */

/* This is an interim value that we will use until the committee comes
 * up with a final number.
 */
#define EM_ALPHA	0x9026
#define EM_S390         0xA390		/* IBM S390 */

#define SHT_LOOS	 0x60000000	/* Start OS-specific */
#define SHT_LOSUNW	 0x6ffffffa	/* Sun-specific low bound.  */
#define SHT_SUNW_move	 0x6ffffffa
#define SHT_SUNW_COMDAT  0x6ffffffb
#define SHT_SUNW_syminfo 0x6ffffffc
#define SHT_GNU_verdef	 0x6ffffffd	/* Version definition section.  */
#define SHT_GNU_verneed	 0x6ffffffe	/* Version needs section.  */
#define SHT_GNU_versym	 0x6fffffff	/* Version symbol table.  */
#define SHT_HISUNW	 0x6fffffff	/* Sun-specific high bound.  */
#define SHT_HIOS	 0x6fffffff	/* End OS-specific type */

#define SHF_MERGE	     0x00000010	/* Might be merged */
#define SHF_STRINGS	     0x00000020	/* Contains nul-terminated strings */
#define SHF_INFO_LINK	     0x00000040	/* `sh_info' contains SHT index */
#define SHF_LINK_ORDER	     0x00000080	/* Preserve order after combining */
#define SHF_OS_NONCONFORMING 0x00000100	/* Non-standard OS specific handling
					   required */
#define SHF_MASKOS	     0x0ff00000	/* OS-specific.  */

#define STB_LOOS	10		/* Start of OS-specific */
#define STB_HIOS	12		/* End of OS-specific */

#define STT_LOOS	10		/* Start of OS-specific */
#define STT_HIOS	12		/* End of OS-specific */

#define PT_LOOS		0x60000000	/* Start of OS-specific */
#define PT_HIOS		0x6fffffff	/* End of OS-specific */

#define PF_X		(1 << 0)	/* Segment is executable */
#define PF_W		(1 << 1)	/* Segment is writable */
#define PF_R		(1 << 2)	/* Segment is readable */
#define PF_MASKOS	0x0ff00000	/* OS-specific */
#define PF_MASKPROC	0xf0000000	/* Processor-specific */

#define	DT_BIND_NOW	   24	      /* Process relocations of object */
#define	DT_INIT_ARRAY	   25	      /* Array with addresses of init fct */
#define	DT_FINI_ARRAY	   26	      /* Array with addresses of fini fct */
#define	DT_INIT_ARRAYSZ	   27	      /* Size in bytes of DT_INIT_ARRAY */
#define	DT_FINI_ARRAYSZ	   28	      /* Size in bytes of DT_FINI_ARRAY */
#define DT_RUNPATH	   29	      /* Library search path */
#define DT_FLAGS	   30	      /* Flags for the object being loaded */
#define DT_ENCODING	   32	      /* Start of encoded range */
#define DT_PREINIT_ARRAY   32	      /* Array with addresses of preinit fct*/
#define DT_PREINIT_ARRAYSZ 33	      /* size in bytes of DT_PREINIT_ARRAY */
#define DT_LOOS		   0x60000000 /* Start of OS-specific */
#define DT_HIOS		   0x6fffffff /* End of OS-specific */

/*
 * All-new categories of definitions from <elf.h> and <linux/elf.h>.
 */

/* How to extract and insert information held in the st_other field.  */

#define ELF32_ST_VISIBILITY(o)	((o) & 0x03)

/* Symbol visibility specification encoded in the st_other field.  */
#define STV_DEFAULT	0		/* Default symbol visibility rules */
#define STV_INTERNAL	1		/* Processor specific hidden class */
#define STV_HIDDEN	2		/* Sym unavailable in other modules */
#define STV_PROTECTED	3		/* Not preemptible, not exported */

/* The syminfo section if available contains additional information about
   every dynamic symbol.  */

typedef struct
{
  Elf32_Half si_boundto;		/* Direct bindings, symbol bound to */
  Elf32_Half si_flags;			/* Per symbol flags */
} Elf32_Syminfo;

/* Possible values for si_boundto.  */
#define SYMINFO_BT_SELF		0xffff	/* Symbol bound to self */
#define SYMINFO_BT_PARENT	0xfffe	/* Symbol bound to parent */
#define SYMINFO_BT_LOWRESERVE	0xff00	/* Beginning of reserved entries */

/* Possible bitmasks for si_flags.  */
#define SYMINFO_FLG_DIRECT	0x0001	/* Direct bound symbol */
#define SYMINFO_FLG_PASSTHRU	0x0002	/* Pass-thru symbol for translator */
#define SYMINFO_FLG_COPY	0x0004	/* Symbol is a copy-reloc */
#define SYMINFO_FLG_LAZYLOAD	0x0008	/* Symbol bound to object to be lazy
					   loaded */
/* Syminfo version values.  */
#define SYMINFO_NONE		0
#define SYMINFO_CURRENT		1

/* Legal values for the note segment descriptor types for object files.  */

#define NT_VERSION	1		/* Contains a version string.  */

/* DT_* entries which fall between DT_VALRNGHI & DT_VALRNGLO use the
   Dyn.d_un.d_val field of the Elf*_Dyn structure.  This follows Sun's
   approach.  */
#define DT_VALRNGLO	0x6ffffd00
#define DT_PLTPADSZ	0x6ffffdf9
#define DT_MOVEENT	0x6ffffdfa
#define DT_MOVESZ	0x6ffffdfb
#define DT_FEATURE_1	0x6ffffdfc	/* Feature selection (DTF_*).  */
#define DT_POSFLAG_1	0x6ffffdfd	/* Flags for DT_* entries, effecting
					   the following DT_* entry.  */
#define DT_SYMINSZ	0x6ffffdfe	/* Size of syminfo table (in bytes) */
#define DT_SYMINENT	0x6ffffdff	/* Entry size of syminfo */
#define DT_VALRNGHI	0x6ffffdff

/* DT_* entries which fall between DT_ADDRRNGHI & DT_ADDRRNGLO use the
   Dyn.d_un.d_ptr field of the Elf*_Dyn structure.

   If any adjustment is made to the ELF object after it has been
   built these entries will need to be adjusted.  */
#define DT_ADDRRNGLO	0x6ffffe00
#define DT_SYMINFO	0x6ffffeff	/* syminfo table */
#define DT_ADDRRNGHI	0x6ffffeff

/* The versioning entry types.  The next are defined as part of the
   GNU extension.  */
#define DT_VERSYM	0x6ffffff0

#define DT_RELACOUNT	0x6ffffff9
#define DT_RELCOUNT	0x6ffffffa

/* These were chosen by Sun.  */
#define DT_FLAGS_1	0x6ffffffb	/* State flags, see DF_1_* below.  */
#define	DT_VERDEF	0x6ffffffc	/* Address of version definition
					   table */
#define	DT_VERDEFNUM	0x6ffffffd	/* Number of version definitions */
#define	DT_VERNEED	0x6ffffffe	/* Address of table with needed
					   versions */
#define	DT_VERNEEDNUM	0x6fffffff	/* Number of needed versions */
#define DT_VERSIONTAGIDX(tag)	(DT_VERNEEDNUM - (tag))	/* Reverse order! */
#define DT_VERSIONTAGNUM 16

/* Sun added these machine-independent extensions in the "processor-specific"
   range.  Be compatible.  */
#define DT_AUXILIARY    0x7ffffffd      /* Shared object to load before self */
#define DT_FILTER       0x7fffffff      /* Shared object to get values from */
#define DT_EXTRATAGIDX(tag)	((Elf32_Word)-((Elf32_Sword) (tag) <<1>>1)-1)
#define DT_EXTRANUM	3

/* Values of `d_un.d_val' in the DT_FLAGS entry.  */
#define DF_ORIGIN	0x00000001	/* Object may use DF_ORIGIN */
#define DF_SYMBOLIC	0x00000002	/* Symbol resolutions starts here */
#define DF_TEXTREL	0x00000004	/* Object contains text relocations */
#define DF_BIND_NOW	0x00000008	/* No lazy binding for this object */

/* State flags selectable in the `d_un.d_val' element of the DT_FLAGS_1
   entry in the dynamic section.  */
#define DF_1_NOW	0x00000001	/* Set RTLD_NOW for this object.  */
#define DF_1_GLOBAL	0x00000002	/* Set RTLD_GLOBAL for this object.  */
#define DF_1_GROUP	0x00000004	/* Set RTLD_GROUP for this object.  */
#define DF_1_NODELETE	0x00000008	/* Set RTLD_NODELETE for this object.*/
#define DF_1_LOADFLTR	0x00000010	/* Trigger filtee loading at runtime.*/
#define DF_1_INITFIRST	0x00000020	/* Set RTLD_INITFIRST for this object*/
#define DF_1_NOOPEN	0x00000040	/* Set RTLD_NOOPEN for this object.  */
#define DF_1_ORIGIN	0x00000080	/* $ORIGIN must be handled.  */
#define DF_1_DIRECT	0x00000100	/* Direct binding enabled.  */
#define DF_1_TRANS	0x00000200
#define DF_1_INTERPOSE	0x00000400	/* Object is used to interpose.  */
#define DF_1_NODEFLIB	0x00000800	/* Ignore default lib search path.  */
#define DF_1_NODUMP	0x00001000
#define DF_1_CONFALT	0x00002000
#define DF_1_ENDFILTEE	0x00004000

/* Flags for the feature selection in DT_FEATURE_1.  */
#define DTF_1_PARINIT	0x00000001
#define DTF_1_CONFEXP	0x00000002

/* Flags in the DT_POSFLAG_1 entry effecting only the next DT_* entry.  */
#define DF_P1_LAZYLOAD	0x00000001	/* Lazyload following object.  */
#define DF_P1_GROUPPERM	0x00000002	/* Symbols from next object are not
					   generally available.  */

/* Version definition sections.  */

typedef struct
{
  Elf32_Half	vd_version;		/* Version revision */
  Elf32_Half	vd_flags;		/* Version information */
  Elf32_Half	vd_ndx;			/* Version Index */
  Elf32_Half	vd_cnt;			/* Number of associated aux entries */
  Elf32_Word	vd_hash;		/* Version name hash value */
  Elf32_Word	vd_aux;			/* Offset in bytes to verdaux array */
  Elf32_Word	vd_next;		/* Offset in bytes to next verdef
					   entry */
} Elf32_Verdef;

/* Legal values for vd_version (version revision).  */
#define VER_DEF_NONE	0		/* No version */
#define VER_DEF_CURRENT	1		/* Current version */

/* Legal values for vd_flags (version information flags).  */
#define VER_FLG_BASE	0x1		/* Version definition of file itself */
#define VER_FLG_WEAK	0x2		/* Weak version identifier */

/* Auxialiary version information.  */

typedef struct
{
  Elf32_Word	vda_name;		/* Version or dependency names */
  Elf32_Word	vda_next;		/* Offset in bytes to next verdaux
					   entry */
} Elf32_Verdaux;

/* Version dependency section.  */

typedef struct
{
  Elf32_Half	vn_version;		/* Version of structure */
  Elf32_Half	vn_cnt;			/* Number of associated aux entries */
  Elf32_Word	vn_file;		/* Offset of filename for this
					   dependency */
  Elf32_Word	vn_aux;			/* Offset in bytes to vernaux array */
  Elf32_Word	vn_next;		/* Offset in bytes to next verneed
					   entry */
} Elf32_Verneed;

/* Legal values for vn_version (version revision).  */
#define VER_NEED_NONE	 0		/* No version */
#define VER_NEED_CURRENT 1		/* Current version */

/* Auxiliary needed version information.  */

typedef struct
{
  Elf32_Word	vna_hash;		/* Hash value of dependency name */
  Elf32_Half	vna_flags;		/* Dependency specific information */
  Elf32_Half	vna_other;		/* Unused */
  Elf32_Word	vna_name;		/* Dependency name string offset */
  Elf32_Word	vna_next;		/* Offset in bytes to next vernaux
					   entry */
} Elf32_Vernaux;

/* Legal values for vna_flags.  */
#define VER_FLG_WEAK	0x2		/* Weak version identifier */

/* Defined types of notes for Solaris.  */

/* Value of descriptor (one word) is desired pagesize for the binary.  */
#define ELF_NOTE_PAGESIZE_HINT	1

/* Defined note types for GNU systems.  */

/* ABI information.  The descriptor consists of words:
   word 0: OS descriptor
   word 1: major version of the ABI
   word 2: minor version of the ABI
   word 3: subminor version of the ABI
*/
#define ELF_NOTE_ABI		1

/* Known OSes.  These value can appear in word 0 of an ELF_NOTE_ABI
   note section entry.  */
#define ELF_NOTE_OS_LINUX	0
#define ELF_NOTE_OS_GNU		1
#define ELF_NOTE_OS_SOLARIS2	2

#endif
