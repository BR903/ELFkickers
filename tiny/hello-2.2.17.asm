;; hello.asm: Copyright (C) 2001 by Brian Raiter, under the GNU
;; General Public License (version 2 or later). No warranty.
;;
;; To build:
;;	nasm -f bin -o hello hello-2.2.17.asm && chmod +x hello


BITS 32

		org	0x05936000

		db	0x7F, "ELF"
		dd	1
		dd	0
		dd	$$
		dw	2
		dw	3
_start:		inc	eax			; 1 == exit syscall no.
		mov	dl, 13			; set edx to length of message
		cmp	al, _start - $$
		pusha				; save eax and ebx
		xchg	eax, ebx		; set ebx to 1 (stdout)
		add	eax, dword 4		; 4 == write syscall no.
		mov	ecx, msg		; point ecx at message
		int	0x80			; eax = write(ebx, ecx, edx)
		popa				; set eax to 1 and ebx to 0
		int	0x80			; exit(bl)
		dw	0x20
		dw	1
msg:		db	'hello, world', 10

;; This is how the file looks when it is read as an ELF header,
;; beginning at offset 0:
;;
;; e_ident:	db	0x7F, "ELF"			; required
;;		db	1				; 1 = ELFCLASS32
;;		db	0				; (garbage)
;;		db	0				; (garbage)
;;		db	0x00, 0x00, 0x00, 0x00, 0x00	; (unused)
;;		db	0x00, 0x60, 0x93, 0x05
;; e_type:	dw	2				; 2 = ET_EXE
;; e_machine:	dw	3				; 3 = EM_386
;; e_version:	dd	0x3C0DB240			; (garbage)
;; e_entry:	dd	0x05936014			; program starts here
;; e_phoff:	dd	4				; phdrs located here
;; e_shoff:	dd	0x93602EB9			; (garbage)
;; e_flags:	dd	0x6180CD05			; (unused)
;; e_ehsize:	dw	0x80CD				; (garbage)
;; e_phentsize:	dw	0x20				; phdr entry size
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
;; p_vaddr:	dd	0x05936000			; load at this address
;; p_paddr:	dd	0x00030002			; (unused)
;; p_filesz:	dd	0x3C0DB240			; a tad bit large ...
;; p_memsz:	dd	0x0593600D			; also excessive
;; p_flags:	dd	4				; 4 = PF_R (no PF_X?)
;; p_align:	dd	0x2EB9				; (garbage)
;;
;; Note that the top three bytes of the file's origin (0x60 0x93 0x68)
;; translate to "pusha", "xchg eax, ebx", and the first byte of "add
;; eax, IMM".
;;
;; The fields marked as unused are either specifically documented as
;; not being used, or not being used with 386-based implementations.
;; Some of the fields marked as containing garbage are not used when
;; loading and executing programs. Other fields containing garbage are
;; accepted because Linux currently doesn't examine then.
