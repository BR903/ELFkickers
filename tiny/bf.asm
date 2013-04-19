;; bf.asm: Copyright (C) 1999-2001 by Brian Raiter, under the GNU
;; General Public License (version 2 or later). No warranty.
;;
;; To build:
;;	nasm -f bin -o bf bf.asm && chmod +x bf
;; To use:
;;	bf < foo.b > foo && chmod +x foo

BITS 32

;; This is the size of the data area supplied to compiled programs.

%define arraysize	30000

;; For the compiler, the text segment is also the data segment. The
;; memory image of the compiler is inside the code buffer, and is
;; modified in place to become the memory image of the compiled
;; program. The area of memory that is the data segment for compiled
;; programs is not used by the compiler. The text and data segments of
;; compiled programs are really only different areas in a single
;; segment, from the system's point of view. Both the compiler and
;; compiled programs load the entire file contents into a single
;; memory segment which is both writeable and executable.

%define	TEXTORG		0x45E9B000
%define	DATAOFFSET	0x2000
%define	DATAORG		(TEXTORG + DATAOFFSET)

;; Here begins the file image.

		org	TEXTORG

;; At the beginning of the text segment is the ELF header and the
;; program header table, the latter consisting of a single entry. The
;; two structures overlap for a space of eight bytes. Nearly all
;; unused fields in the structures are used to hold bits of code.

;; The beginning of the ELF header.

		db	0x7F, "ELF"		; ehdr.e_ident

;; The top(s) of the main compiling loop. The loop jumps back to
;; different positions, depending on how many bytes to copy into the
;; code buffer. After doing that, esi is initialized to point to the
;; epilog code chunk, a copy of edi (the pointer to the end of the
;; code buffer) is saved in ebp, the high bytes of eax are reset to
;; zero (via the exchange with ebx), and then the next character of
;; input is retrieved.

emitputchar:	add	esi, byte (putchar - decchar) - 4
emitgetchar:	lodsd
emit6bytes:	movsd
emit2bytes:	movsb
emit1byte:	movsb
compile:	lea	esi, [byte ecx + epilog - filesize]
		xchg	eax, ebx
		cmp	eax, 0x00030002		; ehdr.e_type    (0x0002)
						; ehdr.e_machine (0x0003)
		mov	ebp, edi		; ehdr.e_version
		jmp	short getchar

;; The entry point for the compiler (and compiled programs), and the
;; location of the program header table.

		dd	_start			; ehdr.e_entry
		dd	proghdr - $$		; ehdr.e_phoff

;; The last routine of the compiler, called when there is no more
;; input. The epilog code chunk is copied into the code buffer. The
;; text origin is popped off the stack into ecx, and subtracted from
;; edi to determine the size of the compiled program. This value is
;; stored in the program header table, and then is moved into edx.
;; The program then jumps to the putchar routine, which sends the
;; compiled program to stdout before falling through to the epilog
;; routine and exiting.

eof:		movsd				; ehdr.e_shoff
		xchg	eax, ecx
		pop	ecx
		sub	edi, ecx		; ehdr.e_flags
		xchg	eax, edi
		stosd
		xchg	eax, edx
		jmp	short putchar		; ehdr.e_ehsize

;; 0x20 == the size of one program header table entry.

		dw	0x20			; ehdr.e_phentsize

;; The beginning of the program header table. 1 == PT_LOAD, indicating
;; that the segment is to be loaded into memory.

proghdr:	dd	1			; ehdr.e_phnum & phdr.p_type
						; ehdr.e_shentsize
		dd	0			; ehdr.e_shnum & phdr.p_offset
						; ehdr.e_shstrndx

;; (Note that the next four bytes, in addition to containing the first
;; two instructions of the bracket routine, also comprise the memory
;; address of the text origin.)

		db	0			; phdr.p_vaddr

;; The bracket routine emits code for the "[" instruction. This
;; instruction translates to a simple "jmp near", but the target of
;; the jump will not be known until the matching "]" is seen. The
;; routine thus outputs a random target, and pushes the location of
;; the target in the code buffer onto the stack.

bracket:	mov	al, 0xE9
		inc	ebp
		push	ebp			; phdr.p_paddr
		stosd
		jmp	short emit1byte

;; This is where the size of the executable file is stored in the
;; program header table. The compiler updates this value just before
;; it outputs the compiled program. This is the only field in the two
;; headers that differs between the compiler and its compiled
;; programs. (While the compiler is reading input, the first byte of
;; this field is also used as an input buffer.)

filesize:	dd	compilersize		; phdr.p_filesz

;; The size of the program in memory. This entry creates an area of
;; bytes, arraysize in size, all initialized to zero, starting at
;; DATAORG.

		dd	DATAOFFSET + arraysize	; phdr.p_memsz

;; The code chunk for the "." instruction. eax is set to 4 to invoke
;; the write system call. ebx, the file handle to write to, is set to
;; 1 for stdout. ecx points to the buffer containing the bytes to
;; output, and edx equals the number of bytes to output. (Note that
;; the first byte of the first instruction, which is also the least
;; significant byte of the p_flags field, encodes to 0xB3. Having the
;; 2-bit set marks the memory containing the compiler, and its
;; compiled programs, as writeable.)

putchar:	mov	bl, 1			; phdr.p_flags
		mov	al, 4
		int	0x80			; phdr.p_align

;; The epilog code chunk. After restoring the initialized registers,
;; eax and ebx are both zero. eax is incremented to 1, so as to invoke
;; the exit system call. ebx specifies the process's return value.

epilog:		popa
		inc	eax
		int	0x80

;; The code chunks for the ">", "<", "+", and "-" instructions.

incptr:		inc	ecx
decptr:		dec	ecx
incchar:	inc	byte [ecx]
decchar:	dec	byte [ecx]

;; The main loop of the compiler continues here, by obtaining the next
;; character of input. This is also the code chunk for the ","
;; instruction. eax is set to 3 to invoke the read system call. ebx,
;; the file handle to read from, is set to 0 for stdin. ecx points to
;; a buffer to receive the bytes that are read, and edx equals the
;; number of bytes to read.

getchar:	mov	al, 3
		xor	ebx, ebx
		int	0x80

;; If eax is zero or negative, then there is no more input, and the
;; compiler proceeds to the eof routine.

		or	eax, eax
		jle	eof

;; Otherwise, esi is advanced four bytes (from the epilog code chunk
;; to the incptr code chunk), and the character read from the input is
;; stored in al, with the high bytes of eax reset to zero.

		lodsd
		mov	eax, [ecx]

;; The compiler compares the input character with ">" and "<". esi is
;; advanced to the next code chunk with each failed test.

		cmp	al, '>'
		jz	emit1byte
		inc	esi
		cmp	al, '<'
		jz	emit1byte
		inc	esi

;; The next four tests check for the characters "+", ",", "-", and
;; ".", respectively. These four characters are contiguous in ASCII,
;; and so are tested for by doing successive decrements of eax.

		sub	al, '+'
		jz	emit2bytes
		dec	eax
		jz	emitgetchar
		inc	esi
		inc	esi
		dec	eax
		jz	emit2bytes
		dec	eax
		jz	emitputchar

;; The remaining instructions, "[" and "]", have special routines for
;; emitting the proper code. (Note that the jump back to the main loop
;; is at the edge of the short-jump range. Routines below here
;; therefore use this jump as a relay to return to the main loop;
;; however, in order to use it correctly, the routines must be sure
;; that the zero flag is cleared at the time.)

		cmp	al, '[' - '.'
		jz	bracket
		cmp	al, ']' - '.'
relay:		jnz	compile

;; The endbracket routine emits code for the "]" instruction, as well
;; as completing the code for the matching "[". The compiler first
;; emits "cmp dh, [ecx]" and the first two bytes of a "jnz near". The
;; location of the missing target in the code for the "[" instruction
;; is then retrieved from the stack, the correct target value is
;; computed and stored, and then the current instruction's jmp target
;; is computed and emitted.

endbracket:	mov	eax, 0x850F313A
		stosd
		lea	esi, [byte edi - 8]
		pop	eax
		sub	esi, eax
		mov	[eax], esi
		sub	eax, edi
		stosd
		jmp	short relay

;; This is the entry point, for both the compiler and its compiled
;; programs. The shared initialization code sets eax and ebx to zero,
;; ecx to the beginning of the array that is the compiled programs's
;; data area, and edx to one. (This also clears the zero flag for the
;; relay jump below.) The registers are then saved on the stack, to be
;; restored at the very end.

_start:
		xor	eax, eax
		xor	ebx, ebx
		mov	ecx, DATAORG
		cdq
		inc	edx
		pusha

;; At this point, the compiler and its compiled programs diverge.
;; Although every compiled program includes all the code in this file
;; above this point, only the eleven bytes directly above are actually
;; used by both. This point is where the compiler begins storing the
;; generated code, so only the compiler sees the instructions below.
;; This routine first modifies ecx to contain TEXTORG, which is stored
;; on the stack, and then offsets it to point to filesize. edi is set
;; equal to codebuf, and then the compiler enters the main loop.

codebuf:
		mov	ch, (TEXTORG >> 8) & 0xFF
		push	ecx
		mov	cl, filesize - $$
		lea	edi, [byte ecx + codebuf - filesize]
		jmp	short relay

;; Here ends the file image.

compilersize	equ	$ - $$
