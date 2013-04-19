;; cat.asm: Copyright (C) 1999 by Brian Raiter, under the GNU
;; General Public License. No warranty. See COPYING for details.

BITS 32

		org	0x5D580000

;; The executable's ELF header and program header table, which overlap
;; each other slightly. The entry point for the program is within the
;; field containing the load address. The top two bytes of this
;; address are 0x58 0x5D, which correspond to the instructions "pop
;; eax" and "pop ebp". This gets argc and argv[0] from the stack. edx
;; is then initialized to one, and the program continues below.

ehdr:						; Elf32_Ehdr
		db	0x7F, "ELF", 1, 1, 1	;   e_ident
	times 9	db	0
		dw	2			;   e_type
		dw	3			;   e_machine
		dd	1			;   e_version
		dd	entry			;   e_entry
		dd	phdr - $$		;   e_phoff
		dd	0			;   e_shoff
		dd	0			;   e_flags
		dw	ehdrsize		;   e_ehsize
		dw	phdrsize		;   e_phentsize
phdr:								; Elf32_Phdr
		dw	1			;   e_phnum	;   p_type
		dw	0			;   e_shentsize
		dw	0			;   e_shnum	;   p_offset
		dw	0			;   e_shstrndx
ehdrsize	equ	$ - ehdr
		dw	0					;   p_vaddr
entry:		pop	eax
		pop	ebp
		cdq						;   p_paddr
		inc	edx
		jmp	short main
		dd	filesize				;   p_filesz
		dd	filesize				;   p_memsz
		dd	5					;   p_flags
		dd	0x1000					;   p_align
phdrsize	equ	$ - phdr

main:

;; If argc is one, then use stdin as our only input file.

		dec	eax
		jz	usestdin

;; Loop once for each filename on the command line.

loopmain:

;; If the next element of argv is NULL, then we're done.

		pop	ebx
		or	ebx, ebx
		jz	quit

;; Otherwise, use system call no. 5 to open the file. ebx is the
;; pointer to the filename, and ecx is zero to open it as read-only.
;; If the return value is non-negative, then the open was successful,
;; and the program enters the read-write loop, with the open file
;; handle saved in edi.

		xor	ecx, ecx
		lea	eax, [byte ecx + 5]
		int	0x80
usestdin:	mov	edi, eax
		or	eax, eax
		jns	loopenter

;; Otherwise, the program stores the error code in ebx, and uses
;; system call no. 1 to exit.

		neg	eax
		xchg	eax, ebx
quit:		xchg	eax, edx
		int	0x80

;; Loop once for each byte in the file.

loopinner:

;; Use system call no. 4 to write a byte. ebx is set to one to use
;; stdout.

		mov	ebx, edx
		mov	al, 4
		int	0x80

;; ecx is set to point to argv[0], which we use as a one-byte buffer.
;; The program uses system call no. 3 to read the next byte in the
;; open file, as specified by ebx. If the return value is one,
;; indicating one byte successfully received, the program loops.

loopenter:	mov	ecx, ebp
		mov	ebx, edi
		lea	eax, [byte edx + 2]
		int	0x80
		dec	eax
		jz	loopinner

;; The program has reached the end of the current file. System call
;; no. 6 is used to close the file, and the program continues on to
;; the next command-line argument.

		lea	eax, [byte edx + 5]
		int	0x80
		jmp	short loopmain

;; End of file image.

filesize	equ	$ - $$
