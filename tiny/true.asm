;; true.asm: Copyright (C) 1999-2001 by Brian Raiter, under the GNU
;; General Public License (version 2 or later). No warranty.
;;
;; To build:
;;	nasm -f bin -o true true.asm && chmod +x true
;;	ln -s true false

BITS 32

		org	0x255F0000

		db	0x7F, "ELF"
		dd	1
		dd	0
		dw	$$
_start:		pop	edi			; remove argc from stack
		and	eax, 0x00030002		; clear all but 3 bits in eax
		mov	ch, 0xFF		; set ecx to >= 0xFF00
		jmp	short skip
		dw	_start
skip:		pop	edi			; get argv[0]
		and	eax, 4			; set eax to zero
		repnz scasb			; find end of argv[0]
		inc	eax			; 1 == exit system call
		mov	bl, [byte edi - 5]	; get 4th-to-last char (t or a)
		and	bl, al			; set bl to 0 or 1
		int	0x80			; exit(bl)
		dw	0x20
		db	1

;; This is how the file looks when it is read as an (incomplete) ELF
;; header, beginning at offset 0:
;;
;; e_ident:	db	0x7F, "ELF"			; required
;;		db	1				; 1 = ELFCLASS32
;;		db	0				; (garbage)
;;		db	0				; (garbage)
;;		db	0x00, 0x00, 0x00, 0x00, 0x00	; (unused)
;;		db	0x00, 0x00, 0x5F, 0x25
;; e_type:	dw	2				; 2 = ET_EXE
;; e_machine:	dw	3				; 3 = EM_386
;; e_version:	dd	0x02EBFFB5			; (garbage)
;; e_entry:	dd	0x255F000E			; program starts here
;; e_phoff:	dd	4				; phdrs located here
;; e_shoff:	dd	0x8A40AEF2			; (garbage)
;; e_flags:	dd	0xC320FB5F			; (unused)
;; e_ehsize:	dw	0x80CD				; (garbage)
;; e_phentsize:	dw	0x20				; phdr entry size
;; e_phnum:	db	1				; one phdr in the table
;; e_shentsize:
;; e_shnum:
;; e_shstrndx:
;;
;; This is how the file looks when it is read as a program header
;; table, beginning at offset 4:
;;
;; p_type:	dd	1				; 1 = PT_LOAD
;; p_offset:	dd	0				; read from top of file
;; p_vaddr:	dd	0x255F0000			; load at this address
;; p_paddr:	dd	0x00030002			; (unused)
;; p_filesz:	dd	0x02EBFFB5			; too big, but ok
;; p_memsz:	dd	0x255F000E			; even bigger
;; p_flags:	dd	4				; 4 = PF_R
;; p_align:	dd	0x8A40AEF2			; (garbage)
;;

;; Note that the top two bytes of the file's origin (0x5F 0x25)
;; correspond to the instructions "pop edi" and the first byte of "and
;; eax, IMM".
;;
;; The fields marked as unused are either specifically documented as
;; not being used, or not being used with 386-based implementations.
;; Some of the fields marked as containing garbage are not used when
;; loading and executing programs. Other fields containing garbage are
;; accepted because Linux currently doesn't examine then.
