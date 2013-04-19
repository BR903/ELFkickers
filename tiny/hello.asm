;; hello.asm: Copyright (C) 1999-2001 by Brian Raiter, under the GNU
;; General Public License (version 2 or later). No warranty.
;;
;; To build:
;;	nasm -f bin -o hello hello.asm && chmod +x hello


BITS 32

		org	0x68504000

		db	0x7F, "ELF"
		dd	1
		dd	0
		db	0
		inc	eax			; the program begins here,
		push	eax			;   though these instructions
		push	dword 0x00030002	;   serve no purpose
		xor	eax, eax		; zero eax
		lea	edx, [byte eax + 13]	; 13 == size of output buffer
		inc	eax			; eax now equals 1
		push	eax			; 1 == the exit syscall no.
		push	dword 4			; 4 == the write syscall no.
		mov	ecx, msgtext		; point ecx at output buffer
		xchg	eax, ebx		; 1 == stdout
done:		pop	eax			; set eax to the syscall no.
		int	0x80			; make the syscall
		dec	ebx			; 0 == successful exit code
		jmp	short done		; do next syscall
		dw	1
msgtext:	db	'hello, world', 10

;; This is how the file looks when it is read as an ELF header,
;; beginning at offset 0:
;;
;; e_ident:	db	0x7F, "ELF"			; required
;;		db	1				; 1 = ELFCLASS32
;;		db	0				; (garbage)
;;		db	0				; (garbage)
;;		db	0x00, 0x00, 0x00, 0x00, 0x00	; (unused)
;;		db	0x00, 0x40, 0x50, 0x68
;; e_type:	dw	2				; 2 = ET_EXE
;; e_machine:	dw	3				; 3 = EM_386
;; e_version:	dd	0x508DC031			; (garbage)
;; e_entry:	dd	0x6850400D			; program starts here
;; e_phoff:	dd	4				; phdrs located here
;; e_shoff:	dd	0x50402EB9			; (garbage)
;; e_flags:	dd	0xCD589368			; (unused)
;; e_ehsize:	dw	0x4B80				; (garbage)
;; e_phentsize:	dw	0xFAEB				; (garbage)
;; e_phnum:	dw	1				; one phdr in the table
;; e_shentsize:	dw	0x6568				; (garbage)
;; e_shnum:	dw	0x6C6C				; (garbage)
;; e_shstrndx:	dw	0x2C6F				; (garbage)
;;
;; This is how the file looks when it is read as a program header
;; table, beginning at offset 4:
;;
;; p_type:	dd	1				; 1 = PT_LOAD
;; p_offset:	dd	0				; read from top of file
;; p_vaddr:	dd	0x68504000			; load at this address
;; p_paddr:	dd	0x00030002			; (unused)
;; p_filesz:	dd	0x508D30C1			; a tad bit large ...
;; p_memsz:	dd	0x6850400D			; also excessive
;; p_flags:	dd	4				; 4 = PF_R (no PF_X?)
;; p_align:	dd	0x2EB9				; (garbage)
;;
;; Note that the top three bytes of the file's origin (0x40 0x50 0x68)
;; translate to "inc eax", "push eax", and the first byte of "push
;; dword". Note also that the program begins at offset 13, which is
;; the same number as the size of the output buffer.
;;
;; The fields marked as unused are either specifically documented as
;; not being used, or not being used with 386-based implementations.
;; Some of the fields marked as containing garbage are not used when
;; loading and executing programs. Other fields containing garbage are
;; accepted because Linux currently doesn't examine then.
