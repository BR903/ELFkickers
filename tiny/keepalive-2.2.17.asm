;; keepalive.asm: Copyright (C) 2001 by Brian Raiter, under the GNU
;; General Public License (version 2 or later). No warranty.
;;
;; To build:
;;	nasm -f bin -o keepalive keepalive.asm && chmod +x keepalive


BITS 32

%define	origin		0x05070000

		org	origin
  
		db	0x7F, "ELF"		; timespec begins at the "F"
		dd	1			; specifying 0x146 seconds
		dd	0			; and 0 nanoseconds
		dd	$$
ding		equ	$ - 2
		dw	2
		dw	3
entry		equ	origin + ($ - $$)
		inc	ebx			; set ebx to 1 (stdout)
		inc	edx			; edx gets 1, length of output
		cmp	eax, entry << 8
		add	eax, dword 4		; 4 == write syscall no.
		mov	ecx, ding		; point ecx at ASCII BEL
repeat:		pusha				; save state
		int	0x80			; 1. lather the bell
		xchg	eax, edi		; zero eax
		cmp	eax, 0x00010020
		mov	bl, 162			; 162 == nanosleep syscall no.
		mov	cl, 3			; point ecx at timespec
		xchg	eax, ecx		; set ebx to timespec pointer
		xchg	eax, ebx		; set eax to syscall no.
		int	0x80			; 2. rinse for 326 seconds
		popa				; restore state
		jmp	short repeat		; 3. repeat

;; This is how the file looks when it is read as an ELF header,
;; beginning at offset 0:
;;
;; e_ident:	db	0x7F, "ELF"			; required
;;		db	1				; 1 = ELFCLASS32
;;		db	0				; (garbage)
;;		db	0				; (garbage)
;;		db	0x00, 0x00, 0x00, 0x00		; (unused)
;;		db	0x00, 0x00, 0x00, 0x07, 0x05
;; e_type:	dw	2				; 2 = ET_EXE
;; e_machine:	dw	3				; 3 = EM_386
;; e_version:	dd	0x03EB4243			; (garbage)
;; e_entry:	dd	0x05070014			; program starts here
;; e_phoff:	dd	4				; phdrs located here
;; e_shoff:	dd	0x07001AB9			; (garbage)
;; e_flags:	dd	0x80CD6005			; (unused)
;; e_ehsize:	dw	0x3D97				; (garbage)
;; e_phentsize:	dw	0x20				; phdr entry size
;; e_phnum:	dw	1				; one phdr in the table
;; e_shentsize:	dw	0xA2B3				; (garbage)
;; e_shnum:	dw	0x03B1				; (garbage)
;; e_shstrndx:	dw	0x9391				; (garbage)
;;
;; This is how the file looks when it is read as a program header
;; table, beginning at offset 4:
;;
;; p_type:	dd	1				; 1 = PT_LOAD
;; p_offset:	dd	0				; read from top of file
;; p_vaddr:	dd	0x05070000			; load at this address
;; p_paddr:	dd	0x00030002			; (unused)
;; p_filesz:	dd	0x03EB4243			; larger than actual
;; p_memsz:	dd	0x05070014			; also excessive
;; p_flags:	dd	4				; 4 = PF_R
;; p_align:	dd	0x1AB9				; (garbage)
;;
;; Note that the top byte of the file's origin (0x05) is the first
;; byte of the "add eax, IMM" instruction, and that the next byte
;; (0x07) is the ASCII BEL character.
;;
;; The fields marked as unused are either specifically documented as
;; not being used, or not being used with 386-based implementations.
;; Some of the fields marked as containing garbage are not used when
;; loading and executing programs. Other fields containing garbage are
;; accepted because Linux currently doesn't examine then.
