;; true.asm: Copyright (C) 1999 by Brian Raiter, under the GNU
;; General Public License. No warranty. See COPYING for details.

BITS 32

%define	ORIGIN		0x055F0000

		org	ORIGIN

		db	0x7F, "ELF"
		dd	1
		dd	0
		dw	ORIGIN & 0xFFFF
entry		equ	$ - $$
		pop	edi			; remove argc from stack
		add	eax, 0x00030002
		xor	ecx, ecx		; zero ecx
		jmp	short skip
		dw	(ORIGIN + entry) & 0xFFFF
skip:		pop	edi			; get argv[0]
		add	eax, 4
		pop	eax			; zero eax (using argv[1])
		dec	ecx			; set ecx to 0xFFFFFFFF
		repnz scasb			; find end of argv[0] string
		mov	bl, [byte edi - 5]	; get 4th to last char (t or a)
		inc	eax			; 1 == exit syscall no.
		and	bl, al			; set bl to 0 or 1
		int	0x80			; exit(bl)
		db	1

;; This is how the file looks when it is read as an (incomplete) ELF
;; header, beginning at offset 0:
;;
;; e_ident:	db	0x7F, "ELF"			; required
;;		db	1				; 1 = ELFCLASS32
;;		db	0				; (garbage)
;;		db	0				; (garbage)
;;		db	0x00, 0x00, 0x00, 0x00		; (unused)
;;		db	0x00, 0x00, 0x00, 0x5F, 0x05
;; e_type:	dw	2				; 2 = ET_EXE
;; e_machine:	dw	3				; 3 = EM_386
;; e_version:	dd	0x02EBC931			; (garbage)
;; e_entry:	dd	ORIGIN + entry			; program starts here
;; e_phoff:	dd	4				; phdrs located here
;; e_shoff:	dd	0xAEF24958			; (garbage)
;; e_flags:	dd	0x40FB5F8A			; (unused)
;; e_ehsize:	dw	0xC320				; (garbage)
;; e_phentsize:	dw	0x80CD				; (garbage)
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
;; p_vaddr:	dd	ORIGIN				; load at this address
;; p_paddr:	dd	0x00030002			; (unused)
;; p_filesz:	dd	0x02EBC931			; much too big, but ok
;; p_memsz:	dd	ORIGIN + entry			; even bigger
;; p_flags:	dd	4				; 4 = PF_R (no PF_X?)
;; p_align:	dd	0xAEF24958			; (garbage)
;;
;; The fields marked as unused are either specifically documented as
;; not being used, or not being used with 386-based implementations.
;; Some of the fields marked as containing garbage are not used when
;; loading and executing programs. Other fields containing garbage are
;; accepted because Linux currently doesn't examine then.
;;
;; Note that the top two bytes of the file's origin (0x5F 0x05)
;; correspond to the instructions "pop edi" and the first byte of "add
;; eax, IMM". (The latter provides a method for the program to skip
;; over the following four bytes.)
