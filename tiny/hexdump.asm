;; hexdump.asm: Copyright (C) 2001 Brian Raiter <breadbox@muppetlabs.com>,
;; under the terms of the GNU General Public License.
;;
;; Usage: hexdump [FILE]...
;; With no parameters, hexdump reads from standard input.
;; hexdump returns zero on success, or an error code otherwise.

BITS 32

;; The traditional hexdump utility only displays the graphic
;; characters in the standard ASCII set (32-126). By uncommenting this
;; line, hexdump will also display the graphic characters in the
;; ISO-8859 set (160-255).

;%define ISO_8859

;; Number of bytes displayed per line.

%define linesize	16


;; The top two bytes of the origin are the first two bytes of the
;; eaxfinish routine ("xchg eax, ebx" and half of "xor eax, eax").

%define	origin		0x31930000

;; The executable's ELF header and program header table, which overlap
;; each other slightly. The program header table defines a single
;; segment of memory, with both write and execute permissions set,
;; which is loaded with contents of the entire file plus enough space
;; after to hold the data section, as defined above. The entry point
;; for the program appears within the program header table, at an
;; unused field. ebp, which keeps track of the current offset, is
;; initialized to zero, and the program then enters the main loop.

		org	origin

ehdr:						; Elf32_Ehdr
		db	0x7F, "ELF", 1, 1, 1	;   e_ident
	times 9	db	0
		dw	2			;   e_type
		dw	3			;   e_machine
		dd	1			;   e_version
		dd	_start			;   e_entry
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
		dw	$$					;   p_vaddr
eaxfinish:	xchg	eax, ebx
finish:		xor	eax, eax				;   p_paddr
		inc	eax
		int	0x80
		dd	filesize				;   p_filesz
		dd	memsize					;   p_memsz
		dd	7					;   p_flags
		dd	0x1000					;   p_align
phdrsize	equ	$ - phdr


;; When the program arrives here, eax will contain a negative value
;; if an error occurred, or zero if the program ran to completion. The
;; negative of eax is moved to ebx, where it is passed to the exit
;; system call, and the program is done.

quit:		neg	eax
		jmp	short eaxfinish

_start:

;; argc and argv[0] are removed from the stack. If argc is one, the
;; program uses stdin as the input file. Otherwise, the next four
;; bytes are skipped over (3D is the "cmp eax, IMM" instruction).

		pop	eax
		pop	ebx
		dec	eax
		jz	usestdin
		db	0x3D

;; When the program arrives here, nothing is left to read from the
;; current input file. The descriptor is closed and the program
;; moves on to the next file.

eof:		mov	al, 6
		int	0x80

;; Loop once for each file specified.

fileloop:

;; Get the next argument from the stack; if it is NULL, then the
;; program exits. Otherwise the open system call is used to get a
;; file descriptor, which is saved in ebx. ebp, which is used to
;; hold the current file offset, is reset to zero.

		pop	ebx
		or	ebx, ebx
		jz	finish
		xor	ecx, ecx
		lea	eax, [byte ecx + 5]
		int	0x80
		or	eax, eax
		js	quit
usestdin:	xchg	eax, ebx
		xor	ebp, ebp

;; Loop once for each line of output.

lineloop:

;; The left-hand side of the output buffer is initialized with spaces.
;; This will also move edi to the start of the input buffer. esi
;; retains a copy of the start of the output buffer.

		mov	edi, format
		mov	esi, edi
		push	byte leftsize
		pop	ecx
		mov	al, ' '
		rep stosb

;; (Up to) 16 bytes are read from the file; if no input is available,
;; the program exits the loop. After the system call, eax will will
;; contain the number of bytes actually read. The current values of
;; all the registers are saved on the stack.

		mov	ecx, edi
		push	byte 3
		push	byte linesize
		pop	edx
		pop	eax
		int	0x80
		or	eax, eax
		js	quit
		jz	eof
		cdq
		pusha

;; ecx is loaded with the address of the hexoutr subroutine. esi and
;; edi are swapped, so that esi points to the input buffer and edi to
;; the output buffer. ebp, which holds the current offset, is moved
;; into eax, and ebp gets the size of the input instead.

		lea	ecx, [byte ecx + hexoutr - linein]
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

;; Loop once for each byte in the input.

byteloop:

;; At every other iteration, edi is incremented, leaving a space in
;; the output buffer. Each byte is examined, and non-graphic values
;; are replaced in the input buffer with a dot. The byte is then given
;; to hexout, to add the hexadecimal representation to the output
;; buffer. When ebp reaches zero, the loop is done, and a newline is
;; appended.

		xor	edx, byte 1
		add	edi, edx
		lodsb
		cmp	al, 0x7F
		jz	dot
%ifdef ISO_8859
		test	al, 0x60
		jnz	nodot
%else
		cmp	al, ' '
		jge	nodot
%endif
dot:		mov	byte [byte esi - 1], '.'
nodot:		call	ecx
		dec	ebp
		jnz	byteloop
		mov	byte [esi], 10

;; The old register values are retrieved from the stack. eax (the size
;; of the input) is added to ebp (updating the current offset), esi
;; (the start of the output buffer) is copied into ecx, and the full
;; size of the output is calculated and stored in edx. The program
;; then write the contents of the buffer to standard output.

		popa
		push	ebx
		lea	ebx, [byte edx + 1]
		add	ebp, eax
		lea	edx, [byte eax + leftsize + 1]
		mov	ecx, esi
		mov	al, 4
		int	0x80
		pop	ebx
		or	eax, eax
		jg	lineloop

;; hexoutr is called via ecx, so by pushing ecx before falling through
;; to the actual subroutine, hexout, it will return to this spot. ecx
;; will then be pushed again; however, this time ecx will have been
;; incremented, so when it returns the second time, the stack will
;; remain unmodified, and the next return will actually return to the
;; caller. As a result, hexout is executed three times in a row, and
;; ecx will have been advanced a total of three bytes. eax is rotated
;; left one byte before each execution, so hexout operates on
;; successively less significant bytes of eax. (hexoutr only runs
;; through three of the bytes since the first execution destroys the
;; value of the fourth byte of eax.)

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
		cmp	al, 10
		sbb	al, 0x69
		das
		stosb
		ret


filesize	equ	$ - $$


ABSOLUTE origin + filesize


format:

;; The size and shape of the program's data.
;;
;; Each line of the program's output is formatted as follows:
;;
;; FILEOFFS: HEXL HEXL HEXL HEXL HEXL HEXL HEXL HEXL  ASCII-CHARACTERS
;;
;; The ASCII region of the output, at far right, also doubles as the
;; input buffer.

leftend:	resb	8			; 8 characters for the offset
		resb	2			; a colon and a space
		resb	5 * (linesize / 2)	; the hex byte display
		resb	1			; a space
leftsize	equ	$ - leftend
linein:		resb	linesize		; the ASCII characters
		resb	1			; newline character


memsize		equ	$ - origin
