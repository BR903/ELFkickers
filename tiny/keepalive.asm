;; keepalive.asm: Copyright (C) 2000-2001 by Brian Raiter, under the
;; GNU General Public License (version 2 or later). No warranty.
;;
;; To build:
;;	nasm -f bin -o keepalive keepalive-2.2.17.asm && chmod +x keepalive


BITS 32

%define	origin		0x68070000
  
		org	origin
  
		db	0x7F, "ELF"		; timespec begins at the "F"
		dd	1			; specifying 0x146 seconds
		dd	0			; and 0 nanoseconds
		dd	$$
ding		equ	$ - 2
		dw	2
		dw	3
entry		equ	origin + ($ - $$)
repeat:		xor	edx, edx		; zero edx
		add	eax, entry << 8
		push	dword 4
		inc	edx			; edx == 1 (length of output)
		mov	ecx, ding		; ecx points to ASCII BEL
		mov	ebx, edx		; ebx == 1 (stdout)
		pop	eax			; eax == 4 (write syscall no.)
		int	0x80			; 1. lather the bell
		jmp	short skip
		db	0
skip:		xchg	eax, ecx		; move ecx into ebx
		xchg	eax, ebx		; and set eax < 256
		mov	al, 162			; eax == nanosleep syscall no.
		mov	bl, 3			; ebx points to timespec
		xor	ecx, ecx		; ecx == NULL
		int	0x80			; 2. rinse for 326 seconds
		jmp	short repeat		; 3. repeat

;; This is how the file looks when it is read as an ELF header,
;; beginning at offset 0:
;;
;; e_ident:	db	0x7F, "ELF"			; required
;;		db	1				; 1 = ELFCLASS32
;;		db	0				; (garbage)
;;		db	0				; (garbage)
;;		db	0x00, 0x00, 0x00, 0x00, 0x00	; (unused)
;;		db	0x00, 0x00, 0x07, 0x68
;; e_type:	dw	2				; 2 = ET_EXE
;; e_machine:	dw	3				; 3 = EM_386
;; e_version:	dd	0x0005D231			; (garbage)
;; e_entry:	dd	0x68070014			; program starts here
;; e_phoff:	dd	4				; phdrs located here
;; e_shoff:	dd	0x003AB942			; (garbage)
;; e_flags:	dd	0xD3896800			; (unused)
;; e_ehsize:	dw	0xCD58				; (garbage)
;; e_phentsize:	dw	0xEB80				; (garbage)
;; e_phnum:	dw	1				; one phdr in the table
;; e_shentsize:	dw	0x9391				; (garbage)
;; e_shnum:	dw	0xA2B0				; (garbage)
;; e_shstrndx:	dw	0x03B3				; (garbage)
;;
;; This is how the file looks when it is read as a program header
;; table, beginning at offset 4:
;;
;; p_type:	dd	1				; 1 = PT_LOAD
;; p_offset:	dd	0				; read from top of file
;; p_vaddr:	dd	0x68070000			; load at this address
;; p_paddr:	dd	0x00030002			; (unused)
;; p_filesz:	dd	0x0005D231			; only slightly off ...
;; p_memsz:	dd	0x68070014			; really off -- oh well
;; p_flags:	dd	4				; 4 = PF_R (no PF_X?)
;; p_align:	dd	0xB942				; (garbage)
;;
;; Note that the top byte of the file's origin (0x68) corresponds to
;; the "push dword" instruction, while the second byte (0x07) is an
;; ASCII BEL.
;;
;; The fields marked as unused are either specifically documented as
;; not being used, or not being used with 386-based implementations.
;; Some of the fields marked as containing garbage are not used when
;; loading and executing programs. Other fields containing garbage are
;; accepted because Linux currently doesn't examine then.
