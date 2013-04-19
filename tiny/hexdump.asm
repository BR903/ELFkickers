;; hexdump.asm: Copyright (C) 1999 by Brian Raiter, under the GNU
;; General Public License. No warranty. See COPYING for details.

BITS 32

linesize	equ	16			; no. of bytes in each line

;; The size and shape of the program's data.
;;
;; Each line of the program's output is formatted as follows:
;;
;; FILEOFFS: HEXL HEXL HEXL HEXL HEXL HEXL HEXL HEXL  ASCII-CHARACTERS
;;
;; The ASCII region of the output, at far right, also doubles as the
;; input buffer.

STRUC data
leftend:	resb	8			; 8 characters for the offset
		resb	2			; a colon and a space
		resb	5 * (linesize / 2)	; the hex byte display
		resb	1			; a space
leftsize	equ	$ - leftend
linein:		resb	linesize		; the ASCII characters
		resb	1			; newline character
ENDSTRUC

		org	0x08048000

;; The executable's ELF header and program header table, which overlap
;; each other slightly. The program header table defines a single
;; segment of memory, with both write and execute permissions set,
;; which is loaded with contents of the entire file plus enough space
;; after to hold the data section, as defined above. The entry point
;; for the program appears within the program header table, at an
;; unused field. ebp, which keeps track of the current offset, is
;; initialized to zero, and the program then enters the main loop.

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
		dd	$$					;   p_vaddr
entry:		xor	ebp, ebp				;   p_paddr
		jmp	short loopmain
		dd	filesize				;   p_filesz
		dd	filesize + data_size			;   p_memsz
		dd	7					;   p_flags
		dd	0x1000					;   p_align
phdrsize	equ	$ - phdr

;; Loop once for each line of output.

loopmain:

;; The left-hand side of the output buffer is initialized with spaces.
;; This will also move edi to the start of the input buffer. esi
;; retains a copy of the start of the output buffer.

		xor	ebx, ebx
		mov	edi, datasection
		mov	esi, edi
		lea	ecx, [byte ebx + leftsize]
		mov	al, ' '
		rep stosb

;; (Up to) 16 bytes are read from standard input; if no input is
;; available, the program exits. After the system call, eax will
;; contain the number of bytes actually read. ebx is set to one, and
;; the current values of all the registers are saved on the stack.

		lea	eax, [byte ebx + 3]
		mov	ecx, edi
		lea	edx, [byte ebx + linesize]
		int	0x80
		inc	ebx
		or	eax, eax
		jle	quit
		pusha

;; ecx is loaded with the address of the hexoutr subroutine. esi and
;; edi are swapped, so now esi points to the input buffer, and edi
;; points to the output buffer. ebp, which holds the current offset,
;; is moved into eax, and ebp gets the size of the input instead.

		lea	ecx, [byte esi + hexoutr - datasection]
		xchg	esi, edi
		xchg	eax, ebp

;; The offset is stored, in ASCII, at the far left of the output
;; buffer. The call to hexoutr executes hexout three times in a row,
;; storing the highest three bytes of eax and destroying eax in the
;; process. eax is then restored from the stack, ecx is advanced to
;; point directly to the hexout subroutine, and the final byte is
;; stored in the output buffer. Finally, a colon is appended.

		push	eax
		call	ecx
		pop	eax
		inc	ecx
		inc	ecx
		call	ecx
		mov	al, ':'
		stosb

;; This loop is run through once for each byte in the input.

loopline:

;; At every other iteration, edi is incremented, leaving a space in
;; the output buffer. Each byte is examined, and non-graphic values
;; are replaced in the input buffer with a dot. The byte is then given
;; to hexout, to add the hexadecimal representation to the output
;; buffer. When ebp reaches zero, the loop is done, and a newline is
;; appended.

		add	edi, ebx
		xor	ebx, byte 1
		lodsb
		cmp	al, ' '
		jb	dot
		cmp	al, '~'
		jbe	nodot
dot:		mov	byte [byte esi - 1], '.'
nodot:		call	ecx
		dec	ebp
		jnz	loopline
		mov	byte [esi], 10

;; The old register values are retrieved from the stack. eax (the size
;; of the input) is added to ebp (updating the current offset), esi
;; (the start of the output buffer) is copied into ecx, and the full
;; size of the output is calculated and stored in edx. The program
;; then write the contents of the buffer to standard output.

		popa
		add	ebp, eax
		lea	edx, [byte eax + leftsize + 1]
		mov	ecx, esi
		mov	al, 4
		int	0x80
		jnc	loopmain

quit:

;; When the program arrives here, ebx will always contain one, and eax
;; will contain zero if there was no more input, or a negative value
;; if an error occurred. Thus swapping the two registers sets them up
;; nicely for the exit system call.

		xchg	eax, ebx
		int	0x80

;; hexoutr is called via ecx, so by pushing ecx before falling through
;; to the actual subroutine, hexout, it will return to this spot.  ecx
;; will then be pushed again; however, this time ecx will have been
;; incremented, so when it returns the second time, the stack will
;; remain unmodified, and the next return will actually return to the
;; caller. As a result, hexout is executed three times in a row, and
;; ecx will have been advanced a total of three bytes. eax is rotated
;; left one byte before each execution, so hexout operates on
;; successively less significant bytes of eax. (hexoutr only runs
;; through three of the bytes since the first execution of eax destoys
;; the value of the fourth byte.)

hexoutr:
		push	ecx
		inc	ecx
		rol	eax, 8

;; The hexout subroutine stores at edi an ASCII representation of the
;; hexadecimal value in al. Both al and ah are destroyed in the
;; process. edi is advanced two bytes.

hexout:

;; The first instruction breaks apart al into two separate nybbles,
;; one each in ah and al. The high nybble is handled first, then when
;; the hexout0 "subroutine" returns, the low nybble is handled, and
;; hexout returns to the real caller.

		aam	16
		call	hexout0
hexout0:	xchg	al, ah
		cmp	al, 9
		jbe	under10
		add	al, 'A' - ('9' + 1)
under10:	add	al, '0'
		stosb
		ret

;; The program ends here. The next 64 bytes in memory is where the
;; data is stored.

datasection:

filesize	equ	$ - $$
