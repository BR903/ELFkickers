;; fgconsole.asm: Copyright (C) 1999-2001 by Brian Raiter, under the
;; GNU General Public License (version 2 or later). No warranty.
;;
;; To build:
;;	nasm -f bin -o fgconsole fgconsole.asm && chmod +x fgconsole


BITS 32

%define	ORIGIN		0x68000000

		org	ORIGIN

		db	0x7F, "ELF"
		dd	1
		dd	0
		dd	$$
		dw	2
		dw	3
entry		equ	ORIGIN + ($ - $$)
		mov	edx, esp		; Use top of stack as a buffer
		add	eax, entry << 8
		push	dword 4			; 4 == the write syscall no.
		xor	ebx, ebx		; 0 == stdin
		lea	eax, [byte ebx + 54]	; 54 == the ioctl syscall no.
		lea	ecx, [byte ebx + 0x1C]	; 0x541C == TIOCLINUX
		mov	dword [edx], 0x00010A0C ; 0x0C == get fg console
		mov	ch, 0x54		; complete ecx
		int	0x80			; eax = ioctl(ebx, ecx, edx)
		inc	ebx			; 1 == stdout, exit syscall no.
		or	al, al			; test the return value
		js	quit			; if negative, exit now
		add	al, '1'			; turn value into ASCII digit
		mov	byte [edx], al		; store next to the newline
		mov	ecx, edx		; move the buffer ptr to ecx
		mov	edx, ebx		; set edx to 2, the size of
		inc	edx			;   the output
		pop	eax			; set eax to 4
		int	0x80			; eax = write(ebx, ecx, edx)
		sub	eax, edx		; zero iff the write worked
quit:		xchg	eax, ebx		; 1 == the exit syscall no.
		int	0x80			; exit(ebx)

;; This is how the file looks when it is read as an ELF header,
;; beginning at offset 0:
;;
;; e_ident:	db	0x7F, "ELF"			; required
;;		db	1				; 1 = ELFCLASS32
;;		db	0				; (garbage)
;;		db	0				; (garbage)
;;		db	0x00, 0x00, 0x00, 0x00		; (unused)
;;		db	0x00, 0x00, 0x00, 0x00, 0x68
;; e_type:	dw	2				; 2 = ET_EXE
;; e_machine:	dw	3				; 3 = EM_386
;; e_version:	dd	0x0005E289			; (garbage)
;; e_entry:	dd	entry				; program starts here
;; e_phoff:	dd	4				; phdrs located here
;; e_shoff:	dd	0x438DDB31			; (garbage)
;; e_flags:	dd	0x1C4B8D36			; (unused)
;; e_ehsize:	dw	0x02C7				; (garbage)
;; e_phentsize:	dw	0x0A0C				; (garbage)
;; e_phnum:	dw	1				; one phdr in the table
;; e_shentsize:	dw	0x45B5				; (garbage)
;; e_shnum:	dw	0x80CD				; (garbage)
;; e_shstrndx:	dw	0x8043				; (garbage)
;;
;; This is how the file looks when it is read as a program header
;; table, beginning at offset 4:
;;
;; p_type:	dd	1				; 1 = PT_LOAD
;; p_offset:	dd	0				; read from top of file
;; p_vaddr:	dd	ORIGIN				; load at this address
;; p_paddr:	dd	0x00030002			; (unused)
;; p_filesz:	dd	0x0005E289			; only slightly off ...
;; p_memsz:	dd	entry				; really off -- oh well
;; p_flags:	dd	4				; 4 = PF_R (no PF_X?)
;; p_align:	dd	0xDB31				; (garbage)
;;
;; Note that the top byte of the file's origin (0x68) corresponds to
;; the "push dword" instruction.
;;
;; The fields marked as unused are either specifically documented as
;; not being used, or not being used with 386-based implementations.
;; Some of the fields marked as containing garbage are not used when
;; loading and executing programs. Other fields containing garbage are
;; accepted because Linux currently doesn't examine then.
