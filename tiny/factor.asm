;; factor.asm: Copyright (C) 1999-2001 by Brian Raiter, under the GNU
;; General Public License (version 2 or later). No warranty.
;;
;; To build:
;;	nasm -f bin -o factor factor.asm && chmod +x factor
;;
;; Usage: factor [N]...
;; Prints the prime factors of each N. With no arguments, reads N
;; from standard input. The valid range is 0 <= N < 2^64.

BITS 32

%define	stdin		0
%define	stdout		1
%define	stderr		2

;; The program's data

%define	iobuf_size	96

STRUC data
factor:		resd	2		; number being tested for factorhood
getchar_rel:	resd	1		; pointer to getchar() function
write_rel:	resd	1		; pointer to write() function
buf:		resd	3		; general-purpose numerical buffer
exit_rel:	resd	1		; pointer to _exit() function
iobuf:		resb	iobuf_size	; buffer for I/O
ENDSTRUC


		org	0x08048000

;; The ELF header and the program header table. The last eight bytes
;; of the former coexist with the first eight bytes of the former.
;; The program header table contains three entries. The first is the
;; interpreter pathname, the third is the _DYNAMIC section, and the
;; middle one contains everything in the file.

		db	0x7F, 'ELF'
		db	1			; ELFCLASS32
		db	1			; ELFDATA2LSB
		db	1			; EV_CURRENT
	times 9	db	0
		dw	2			; ET_EXEC
		dw	3			; EM_386
		dd	1			; EV_CURRENT
		dd	_start
		dd	phdrs - $$
		dd	0
		dd	0
		dw	0x34			; sizeof(Elf32_Ehdr)
		dw	0x20			; sizeof(Elf32_Phdr)
phdrs:		dd	3			; PT_INTERP
		dd	interp - $$
		dd	interp
		dd	interp
		dd	interpsz
		dd	interpsz
		dd	4			; PF_R
		dd	1
		dd	1			; PT_LOAD
		dd	0
		dd	$$
		dd	$$
		dd	filesz
		dd	memsz
		dd	7			; PF_R | PF_W | PF_X
		dd	0x1000
		dd	2			; PT_DYNAMIC
		dd	dynamic - $$
		dd	dynamic
		dd	dynamic
		dd	dynamicsz
		dd	dynamicsz
		dd	6			; PF_R | PF_W
		dd	4

;; The hash table. Essentially a formality. The last entry in the hash
;; table, a 1, overlaps with the next structure.

hash:
		dd	1
		dd	5
		dd	4
		dd	0, 2, 3, 0

;; The _DYNAMIC section. Indicates the presence and location of the
;; dynamic symbol section (and associated string table and hash table)
;; and the relocation section. The final DT_NULL entry in the dynamic
;; section overlaps with the next structure.

dynamic:
		dd	1,  libc_name		; DT_NEEDED
		dd	4,  hash		; DT_HASH
		dd	5,  dynstr		; DT_STRTAB
		dd	6,  dynsym		; DT_SYMTAB
		dd	10, dynstrsz		; DT_STRSZ
		dd	11, 0x10		; DT_SYMENT
		dd	17, reltext		; DT_REL
		dd	18, reltextsz		; DT_RELSZ
		dd	19, 0x08		; DT_RELENT
dynamicsz	equ	$ - dynamic + 8

;; The dynamic symbol table. Entries are included for the _DYNAMIC
;; section and the three functions imported from libc: getchar(),
;; write(), and _exit().

dynsym:
		dd	0
		dd	0
		dd	0
		dw	0
		dw	0
dynamic_sym	equ	1
		dd	dynamic_name
		dd	dynamic
		dd	0
		dw	0x11			; STB_GLOBAL, STT_OBJECT
		dw	0xFFF1			; SHN_ABS
exit_sym	equ	2
		dd	exit_name
		dd	0
		dd	0
		dw	0x12			; STB_GLOBAL, STT_FUNC
		dw	0
getchar_sym	equ	3
		dd	getchar_name
		dd	0
		dd	0
		dw	0x22			; STB_WEAK, STT_FUNC
		dw	0
write_sym	equ	4
		dd	write_name
		dd	0
		dd	0
		dw	0x22			; STB_WEAK, STT_FUNC
		dw	0

;; The relocation table. The addresses of the three functions imported
;; from libc are stored in the program's data area.

reltext:
		dd	dataorg + write_rel
		db	1			; R_386_32
		db	write_sym
		dw	0
		dd	dataorg + getchar_rel
		db	1			; R_386_32
		db	getchar_sym
		dw	0
		dd	dataorg + exit_rel
		db	1			; R_386_32
		db	exit_sym
		dw	0
reltextsz	equ	$ - reltext

;; The interpreter pathname. The final NUL byte appears in the next
;; section.

interp:
		db	'/lib/ld-linux.so.2'
interpsz	equ	$ - interp + 1

;; The string table for the dynamic symbol table.

dynstr:
		db	0
libc_name	equ	$ - dynstr
		db	'libc.so.6', 0
dynamic_name	equ	$ - dynstr
		db	'_DYNAMIC', 0
exit_name	equ	$ - dynstr
		db	'_exit', 0
getchar_name	equ	$ - dynstr
		db	'getchar', 0
write_name	equ	$ - dynstr
		db	'write', 0
dynstrsz	equ	$ - dynstr


;;
;; The program proper
;;


;; The factorconst subroutine, called by factorize, repeatedly divides
;; the number at the top of the floating-point stack by the integer
;; stored in buf as long as the number continues to divide evenly. For
;; each successful division, the number is also displayed on standard
;; output. Upon return, the quotient of the failed division is at the
;; top of the floating-point stack, just above the factored number.

factorconst:						; num  +
		fild	dword [ebx]			; div  num  +
.loop:		fld	st1				; num  div  num  +
		fdiv	st0, st1			; quot div  num  +
		fld	st0				; quot quot div  num  +
		frndint					; quoi quot div  num  +
		fcomp	st1				; quot div  num  +
		fnstsw	ax
		and	ah, 0x40
		jz	factorize.return
		fstp	st2				; div  quot +
		call	ebp
		jmp	short .loop


;; factorize is the main subroutine of the program. It is called with esi
;; pointing to an NUL-terminated string representing the number to factor.
;; Upon return, eax contains a nonzero value if an error occurred (i.e.,
;; an invalid number stored at esi).

factorize:

;; The first step is to translate the string into a number. 10.0 and 0.0
;; are pushed onto the floating-point stack.

		xor	eax, eax
		fild	word [byte ebx + ten - dataorg]	; 10.0
		fldz					; num  10.0

;; Each character in the string is checked; if it is not in the range
;; of '0' to '9' inclusive, an error message is displayed and the
;; subroutine aborts. Otherwise, the top of the stack is multiplied by
;; ten and the value of the digit is added to the product. The loop
;; exits when a NUL byte is found.

.atoiloop:
		lodsb
		or	al, al
		jz	.atoiloopend
		fmul	st0, st1			; n10  10.0
		sub	al, '0'
		jc	.errbadnum
		cmp	al, 10
		jnc	.errbadnum
		mov	[byte ebx + buf], eax
		fiadd	dword [byte ebx + buf]		; num  10.0
		jmp	short .atoiloop
.errbadnum:
		push	byte errmsgbadnumsz
		lea	eax, [byte ebx + errmsgbadnum - dataorg]
		push	eax
		push	byte stderr
		call	[byte ebx + write_rel]
		add	esp, byte 12
		mov	al, 1
.return:	fcompp
		ret
.atoiloopend:

;; The number's exponent is examined, and if the number is 2^64 or
;; greater, it is rejected.

		fld	st0				; num  num  10.0
		fstp	tword [byte ebx + buf]		; num  10.0
		mov	eax, [byte ebx + buf + 8]
		cmp	ax, 64 + 0x3FFF
		jge	.errbadnum

;; The number is displayed, followed by a colon. If the number is one
;; or zero, no factoring should be done and the subroutine skips ahead
;; to the end.
							; num  junk
		mov	dl, ':'
		call	itoa64nospc
		fxch	st1				; junk num
		fld1					; 1.0  junk num
		fcom	st2
		fstsw	ax
		and	ah, 0x45
		cmp	ah, 1
		jnz	.earlyout
		fcompp					; num

;; The factorconst subroutine is called three times, with the number
;; in buf set to two, three, and five, respectively.

		mov	edi, factorconst
		xor	edx, edx
		mov	dl, 2
		mov	[ebx], edx
		call	edi
		inc	dword [ebx]
		call	edi
		mov	byte [ebx], 5
		call	edi

;; If the number is now equal to one, the subroutine is finished and
;; exits immediately.

		fld1					; 1.0  num
		fcom	st1
		fnstsw	ax
		and	ah, 0x40
		jnz	.quitfactorize

;; factor is initialized to 7, and edi is initialized with a sequence
;; of eight four-bit values that represent the cycle of differences
;; between subsequent integers not divisible by 2, 3, or 5. The
;; subroutine then enters its main loop.
							; junk num
		mov	byte [ebx], 7
		fild	qword [ebx]			; fact junk num
		fdivr	st0, st2			; quot junk num
		mov	edi, 0x42424626

;; The loop returns to this point when the last tested value was not a
;; factor. The next value to test (for which the division operation
;; should just be finishing up) is moved into esi, and factor is
;; incremented by the value at the bottom of edi, which is first
;; advanced to the next four-bit value. If it overflows, then we have
;; exhausted all possible factors, and end.

.notafactor:
		mov	esi, [ebx]
		rol	edi, 4
		mov	eax, edi
		and	eax, byte 0x0F
		add	[ebx], eax
		jc	.earlyout

;; The main loop of the factorize subroutine. The quotient from the
;; division of the number by the next potential factor is stored, and
;; the division for the next iteration is started.

.mainloop:						; quot quo0 num
		fst	st1				; quot quot num
		fstp	tword [byte ebx + buf]		; quot num
		fild	qword [ebx]			; fact quot num
		fdivr	st0, st2			; quo2 quot num

;; The integer portion of the quotient is isolated and tested against
;; the divisor (i.e., the potential factor). If the quotient is
;; smaller, then the loop has passed the number's square root, and no
;; more factors will be found. In this case, the program prints out
;; the current value for the number as the last factor, followed by a
;; newline character, and the subroutine ends.

		mov	edx, [byte ebx + buf + 4]
		mov	ecx, 31 + 0x3FFF
		sub	ecx, [byte ebx + buf + 8]
		js	.keepgoing
		mov	eax, edx
		shr	eax, cl
		cmp	eax, esi
		jnc	.keepgoing
.earlyout:	fxch	st2
		call	ebp
		fstp	st0
.quitfactorize:	fcompp
		push	byte 1
		lea	esi, [byte ebx + ten - dataorg]
		jmp	short finalwrite

;; Now the integer portion of the quotient is shifted out. If any
;; nonzero bits are left, then the number being tested is not a
;; factor, and the program loops back.

.keepgoing:	mov	eax, [byte ebx + buf]
		neg	ecx
		js	.shift32
		xchg	eax, edx
		xor	eax, eax
.shift32:	shld	edx, eax, cl
		shl	eax, cl
		or	eax, edx
		jnz	.notafactor

;; Otherwise, a new factor has been found. The number being factored
;; is therefore replaced with the quotient, and the result of the
;; division in progress is junked. The new factor is displayed, and
;; then is tested again. If this was the first time this factor was
;; tested, then edi is reset back.
							; junk num  num0
		cmp	[ebx], esi
		jz	.repeating
		ror	edi, 4
		mov	[ebx], esi
.repeating:	ffree	st2				; junk num
		fild	qword [ebx]			; fact junk num
		call	ebp
		fdivr	st0, st2			; quot junk num
		mov	esi, [ebx]
		jmp	short .mainloop


;; itoa64 is the numeric output subroutine. When the subroutine is
;; called, the number to be displayed should be on the top of the
;; floating-point stack, and there should be no more than four other
;; numbers on the stack. A space is prefixed to the output, unless
;; itoa64nospc is called, in which case the character in dl is
;; suffixed to the output.

itoa64:
		mov	dl, ' '
itoa64nospc:

;; A copy of the number is made, and 10 is placed on the stack. esi is
;; pointed to the end of the buffer that will hold the decimal
;; representation, with ecx pointing just past the end.
							; num  +
		fld	st0				; num  num  +
		fild	word [byte ebx + ten - dataorg]	; 10.0 num  num  +
		lea	esi, [byte ebx + iobuf + 32]
		mov	ecx, esi
		dec	esi

;; At each iteration, the number is reduced modulo 10. This remainder
;; is subtracted from the number (and stored in iobuf as an ASCII
;; digit). The difference is then divided by ten, and if the quotient
;; is zero the loop exits. Otherwise, the quotient replaces the number
;; for the next iteration.

.loop:
		fld	st1				; num  10.0 num  num  +
		fprem					; rem  10.0 num  num  +
		fist	dword [ecx]			; rem  10.0 num  num  +
		fsubr	st0, st2			; 10n2 10.0 num  num  +
		ftst
		fstsw	ax
		fstp	st2				; 10.0 10n2 num  +
		fdiv	st1, st0			; 10.0 num2 num  +
		mov	al, '0'
		add	al, [ecx]
		mov	[esi], al
		dec	esi
		and	ah, 0x40
		jz	.loop
		fcompp					; num  +

;; If al contains a space, it is added to the start of the string.
;; Otherwise, the character is added to the end.

		mov	[esi], dl
		cmp	dl, ' '
		jz	.prefix
		mov	[ecx], dl
		inc	ecx
		inc	esi
.prefix:

;; The string is written to standard output, and the subroutine ends.

		sub	ecx, esi
		push	ecx
finalwrite:	push	esi
		push	byte stdout
		call	[byte ebx + write_rel]
		add	esp, byte 12
		dec	eax
		ret


;; Here is the program's entry point.

_start:

;; argc and argv[0] are removed from the stack and discarded. ebx is
;; initialized to point to the data.

		pop	esi
		pop	ebx
		mov	ebx, dataorg
		mov	ebp, itoa64

;; If argv[1] is NULL, then the program proceed to the input loop. If
;; argv[1] begins with a dash, then the help message is displayed.
;; Otherwise, the program begins readings the command-line arguments.

		pop	esi
		or	esi, esi
		jz	.inputloop
		cmp	byte [esi], '-'
		jz	.dohelp

;; The factorize subroutine is called once for each command-line
;; argument, and then the program exits, with the exit code being
;; the return value from the last call to factorize.

.argloop:
		call	factorize
		pop	esi
		or	esi, esi
		jnz	.argloop
.mainexit:	push	eax
		call	[byte ebx + exit_rel]

;; The input loop routine. edi is pointed to iobuf, and esi is
;; initialized to one less than the size of iobuf.

.inputloop:
		lea	edi, [byte ebx + iobuf]
		push	byte iobuf_size - 1
		pop	esi

;; The program reads and discards one character at a time, until a
;; non-whitespace character is seen (or until no more input is
;; available, in which case the program exits).

.preinloop:
		call	[byte ebx + getchar_rel]
		or	eax, eax
		js	.mainexit
		cmp	al, ' '
		jz	.preinloop
		cmp	al, 9
		jc	.incharloop
		cmp	al, 14
		jc	.preinloop

;; The first non-whitespace character is stored at the beginning of
;; iobuf. The program continues to read characters until there is no
;; more input, there is no more room in iobuf, or until another
;; whitespace character is found.

.incharloop:
		stosb
		dec	esi
		jz	.infinish
		call	[byte ebx + getchar_rel]
		or	eax, eax
		js	.infinish
		cmp	al, ' '
		jz	.infinish
		cmp	al, 9
		jc	.incharloop
		cmp	al, 14
		jnc	.incharloop
.infinish:

;; A NUL is appended to the string obtained from standard input, the
;; factorize subroutine is called, and the program loops.

		mov	byte [edi], 0
		lea	esi, [byte ebx + iobuf]
		call	factorize
		jmp	short .inputloop

;; The program displays the help message on standard output and exits
;; with an exit value of zero.

.dohelp:
		push	byte 0
		push	dword [byte ebx + exit_rel]
		push	byte helpmsgsz
		lea	esi, [byte ebx + helpmsg - dataorg]
		jmp	short finalwrite


;; Messages to the user.

helpmsg:	db	'Finds the prime factors of args/input', 10
		db	'v1.2', 10
helpmsgsz	equ	$ - helpmsg
errmsgbadnum:	db	'invalid number'
ten:		db	10
errmsgbadnumsz	equ	$ - errmsgbadnum

;; End of file image.

filesz		equ	$ - $$

dataorg		equ	$$ + ((filesz + 16) & ~15)

memsz		equ	dataorg + data_size - $$
