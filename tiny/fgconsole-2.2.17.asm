;; fgconsole.asm: Copyright (C) 2001 by Brian Raiter, under the GNU
;; General Public License (version 2 or later). No warranty.
;;
;; To build:
;;	nasm -f bin -o fgconsole fgconsole-2.2.17.asm && chmod +x fgconsole


BITS 32

%define	ORIGIN		0x05000000

		org	ORIGIN

		db	0x7F, "ELF"
		dd	1
		dd	0
		dd	$$
		dw	2
		dw	3
part2:		mov	ecx, esp		; point ecx at text string
		and	eax, entry << 8
		add	eax, dword 4		; 4 == write syscall no.
		mov	edx, ebx		; set edx to 2, the length of
		inc	edx			;   the text string
		int	0x80			; eax = write(ebx, ecx, edx)
		sub	eax, edx		; zero eax iff write succeeded
quit:		xchg	eax, ebx		; 1 == exit syscall no.
		int	0x80			; exit(ebx)
		dw	0x20
		dw	1
entry		equ	ORIGIN + ($ - $$)	; program begins here
		push	byte 0x0C		; 0x0C == get fg console no.
		mov	edx, esp		; Use top of stack as a buffer
		mov	al, 54			; 54 == ioctl syscall no.
		mov	cl, 0x1C		; 0x541C == TIOCLINUX
		mov	ch, 0x54		; (ebx is 0 for stdin)
		int	0x80			; eax = ioctl(ebx, ecx, edx)
		inc	ebx			; 1 == stdout, exit syscall no.
		or	eax, eax		; test the return value
		js	quit			; if negative, exit now
		add	al, '1'			; turn value into ASCII digit
		mov	ah, 10			; append a newline
		push	eax			; use stack as buffer
		xchg	eax, edi		; zero eax
		jmp	short part2

;; This is how the file looks when it is read as an ELF header,
;; beginning at offset 0:
;;
;; e_ident:	db	0x7F, "ELF"			; required
;;		db	1				; 1 = ELFCLASS32
;;		db	0				; (garbage)
;;		db	0				; (garbage)
;;		db	0x00, 0x00, 0x00, 0x00		; (unused)
;;		db	0x00, 0x00, 0x00, 0x00, 0x05
;; e_type:	dw	2				; 2 = ET_EXE
;; e_machine:	dw	3				; 3 = EM_386
;; e_version:	dd	0x0025E189			; (garbage)
;; e_entry:	dd	0x0500002E			; program starts here
;; e_phoff:	dd	4				; phdrs located here
;; e_shoff:	dd	0xCD42DA89			; (garbage)
;; e_flags:	dd	0x93D029CD			; (unused)
;; e_ehsize:	dw	0x80CD				; (garbage)
;; e_phentsize:	dw	0x20				; phdr entry size
;; e_phnum:	dw	1				; one phdr in the table
;; e_shentsize:	dw	0x0C6A				; (garbage)
;; e_shnum:	dw	0x80CD				; (garbage)
;; e_shstrndx:	dw	0xE289				; (garbage)
;;
;; This is how the file looks when it is read as a program header
;; table, beginning at offset 4:
;;
;; p_type:	dd	1				; 1 = PT_LOAD
;; p_offset:	dd	0				; read from top of file
;; p_vaddr:	dd	0x05000000			; load at this address
;; p_paddr:	dd	0x00030002			; (unused)
;; p_filesz:	dd	0x0025E189			; only slightly off ...
;; p_memsz:	dd	0x0500002E			; really off -- oh well
;; p_flags:	dd	4				; 4 = PF_R
;; p_align:	dd	0xDA89				; (garbage)
;;
;; Note that the top byte of the file's origin (0x05) corresponds to
;; the first byte of the "and eax, IMM" instruction.
;;
;; The fields marked as unused are either specifically documented as
;; not being used, or not being used with 386-based implementations.
;; Some of the fields marked as containing garbage are not used when
;; loading and executing programs. Other fields containing garbage are
;; accepted because Linux currently doesn't examine then.
