;;; bf.asm: a tiny Brainfuck compiler, for Intel Linux.
;;;
;;; Copyright (C) 1999 by Brian Raiter, under the terms of the GNU
;;; General Public License, version 2 or any later version.

BITS 32

; This is the size of the data area supplied to compiled programs.

%define arraysize	30000

; For the compiler, the text segment is also the data segment. The
; memory image of the compiler is inside the code buffer, and is
; modified in place to become the memory image of the compiled
; program. The area of memory that is the data segment for compiled
; programs is not used by the compiler. The text and data segments of
; compiled programs are really only different areas in a single
; segment, from the system's point of view. Both the compiler and
; compiled programs load the entire file contents into a single memory
; segment which is both writeable and executable.

%define	TEXTORG		0x45E9B000
%define	DATAOFFSET	0x2000
%define	DATAORG		(TEXTORG + DATAOFFSET)

; Here begins the file image.

		org	TEXTORG

; At the beginning of the text segment is the ELF header and the
; program header table, the latter consisting of a single entry. The
; two structures overlap for a space of eight bytes. Nearly all unused
; fields in the structures are used to hold bits of code.

; The beginning of the ELF header.

		db	0x7F, "ELF"		; ehdr.e_ident

; The top(s) of the main compiling loop. The loop jumps back to
; different positions, depending on how many bytes to copy into the
; code buffer. After doing that, esi is initialized to point to the
; epilog code chunk, a copy of edi (the pointer to the end of the code
; buffer) is saved in ebp, the high bytes of eax are reset to zero
; (via the exchange with ebx), and then the next character of input is
; retrieved.

emitgetchar:	lodsd
emit6bytes:	movsd
emit2bytes:	movsb
emit1byte:	movsb
compile:	lea	esi, [byte ecx + epilog - filesize]
		mov	ebp, edi
		xchg	eax, ebx
		jmp	short getchar

; 2 == ET_EXE, indicating an executable ELF file.
; 3 == EM_386, indicating an Intel-386 target architecture.

		dw	2			; ehdr.e_type
		dw	3			; ehdr.e_machine

; The code chunk for the "]" instruction, minus the last four bytes.

branch:		cmp	dh, [ecx]		; ehdr.e_version
		db	0x0F, 0x85

; The entry point for the compiler (and compiled programs), and the
; location of the program header table.

		dd	entrypoint		; ehdr.e_entry
		dd	proghdr - $$		; ehdr.e_phoff

; This is the entry point, for both the compiler and its compiled
; programs. The shared initialization code sets ecx to the beginning
; of the array that is the compiled programs's data area, and eax,
; ebx, and ecx to zero, and then continues at initpt2.

entrypoint:
		xor	eax, eax		; ehdr.e_shoff
		xor	ebx, ebx
		mov	ecx, DATAORG		; ehdr.e_flags
		cdq				; ehdr.e_ehsize
		jmp	short initpt2		; ehdr.e_phentsize

; The beginning of the program header table. 1 == PT_LOAD, indicating
; that the segment is to be loaded into memory.

proghdr:	dd	1			; ehdr.e_phnum & phdr.p_type
						; ehdr.e_shentsize
		dd	0			; ehdr.e_shnum & phdr.p_offset
						; ehdr.e_shstrndx

; (Note that the next four bytes, in addition to containing the first
; two instructions of the bracket routine, also comprise the memory
; address of the text origin.)

		db	0			; phdr.p_vaddr

; The bracket routine emits code for the "[" instruction. This
; instruction translates to a simple "jmp near", but the target of the
; jump will not be known until the matching "]" is seen. The routine
; thus outputs a random target, and pushes the location of the target
; in the code buffer onto the stack.

bracket:	mov	al, 0xE9
		inc	ebp
		push	ebp			; phdr.p_paddr
		stosd
		jmp	short emit1byte

; This is where the size of the executable file is stored in the
; program header table. The compiler updates this value just before it
; outputs the compiled program. This is the only field in any of the
; headers that differs between the compiler and its compiled programs.
; (While the compiler is reading input, the first byte of this field
; is also used as an input buffer.)

filesize:	dd	compilersize		; phdr.p_filesz

; The size of the program in memory. This entry creates an area of
; bytes, arraysize in size, all initialized to zero, starting at
; DATAORG.

		dd	DATAOFFSET + arraysize	; phdr.p_memsz

; The rest of the eof routine. eax points to the location in the
; program header table which needs to contain the compiled file's
; size, and so this value is moved to edi. The size of the compiled
; program is stored at filesize, and then this value is moved into
; edx. ecx has already been set to TEXTORG, and so the routine then
; proceeds to send the compiled program to stdout, after which it
; exits. (Note that the first instruction encodes to 0x97. The three
; low bits are the three low bits of the p_flags field, and mark the
; memory image of the compiler and its compiled programs as being
; readable, writeable, and executable.)

eofpt2:		xchg	eax, edi		; phdr.p_flags
		stosd
		xchg	eax, edx

; The code chunk for the "." instruction. eax == 4 to invoke the write
; system call. ebx is the file handle to write to, ecx points to the
; buffer containing the bytes to output, and edx equals the number of
; bytes to output.

putchar:	mov	al, 4			; phdr.p_align
		mov	bl, 1
		int	0x80

; The epilog code chunk. After restoring the initialized registers,
; eax and ebx are both zero. eax is incremented to 1, so as to invoke
; the exit system call. ebx specifies the process's return value.

epilog:		popa
		inc	eax
		int	0x80

; The code chunks for the ">", "<", "+", and "-" instructions.

incptr:		inc	ecx
decptr:		dec	ecx
incchar:	inc	byte [ecx]
decchar:	dec	byte [ecx]

; The main loop of the compiler continues here, by obtaining the next
; character of input. This is also the code chunk for the ","
; instruction. eax == 3 to invoke the read system call. ebx contains
; the file handle to read from, ecx points to a buffer to receive the
; bytes that are read, and edx equals the number of bytes to read.

getchar:	mov	al, 3
		xor	ebx, ebx
		int	0x80

; If eax is zero or negative, then there is no more input, and the
; compiler proceeds to the eof routine.

		or	eax, eax
		jle	eof

; Otherwise, esi is advanced four bytes (from the epilog code chunk to
; the incptr code chunk), and the character read from the input is
; stored in al, with the high bytes of eax reset to zero.

		lodsd
		mov	eax, [ecx]

; The compiler compares the input character with ">" and "<". esi is
; advanced to the next code chunk with each failed test.

		cmp	al, '>'
		jz	emit1byte
		inc	esi
		cmp	al, '<'
		jz	emit1byte
		inc	esi

; The next four tests check for the characters "+", ",", "-", and ".",
; respectively. These four characters are contiguous in ASCII, and so
; are tested for by doing successive decrements of eax.

		sub	al, '+'
		jz	emit2bytes
		dec	eax
		jz	emitgetchar
		inc	esi
		inc	esi
		dec	eax
		jz	emit2bytes
		add	esi, byte putchar - decchar
		dec	eax
		jz	emit6bytes

; The remaining instructions, "[" and "]", have special routines for
; emitting the proper code. (Note that the short jump back to the
; beginning of the main loop is barely in range. Routines below here
; therefore use this jump as a relay to return to the main loop;
; however, in order to use it correctly, the routines must be sure
; that the zero flag is cleared at the time.)

		cmp	al, '[' - '.'
		jz	bracket
		cmp	al, ']' - '.'
relay:		jnz	compile

; The endbracket routine emits code for the "]" instruction, as well
; as completing the code for the matching "[". The compiler first
; emits an "or [ecx], dh" and the first two bytes of a "jnz near".
; The location of the missing target in the code for the "["
; instruction is then retrieved from the stack, the correct target
; value is computed and stored, and then the current instruction's
; jmp target is computed and emitted.

endbracket:	add	esi, byte branch - putchar
		movsd
		lea	esi, [byte edi - 8]
		pop	eax
		sub	esi, eax
		mov	[eax], esi
		sub	eax, edi
		stosd
		jmp	short relay

; The last routine of the compiler, called when there is no more
; input. The epilog code chunk is copied into the code buffer. The
; text origin is popped off the stack and subtracted from edi to
; determine the size of the compiled program. The routine then
; continues at eofpt2.

eof:		movsd
		xchg	eax, ecx
		pop	ecx
		sub	edi, ecx
		jmp	short eofpt2

; The initialization process continues here. The value of edx is
; bumped up to one, and the registers are saved on the stack (to be
; retrieved at the very end).

initpt2:	inc	edx
		pusha

; At this point, the compiler and its compiled programs diverge.
; Although every compiled program includes all the code in this file
; above this point, only the routine starting at entrypoint is
; actually used by both. This point is where the compiler begins
; storing the generated code, so only the compiler sees the
; instructions below. This routine first modifies ecx to contain
; TEXTORG, which is stored on the stack, and then offsets it to point
; to filesize. edi is set equal to codebuf, and then the compiler
; enters the main loop.

codebuf:	mov	ch, (TEXTORG >> 8) & 0xFF
		push	ecx
		mov	cl, filesize - $$
		lea	edi, [byte ecx + codebuf - filesize]
		jmp	short relay

; Here ends the file image.

compilersize	equ	$ - $$
