;; factor.asm: Copyright (C) 1999 by Brian Raiter, under the GNU
;; General Public License. No warranty.
;;
;; Usage: factor [N]...
;; Print the prime factors of each N. With no arguments, read N from
;; standard input. The valid range is 0 <= N < 10^18.

BITS 32

%define	stdin		0
%define	stdout		1
%define	stderr		2

;;; The program's data

%define	buf_size	88

STRUC data
bcd80:		resb	12		; buffer for 80-bit BCD numbers
read_rel:	resd	1		; ptr to read() function
quotient:	resb	12		; buffer for 80-bit floating-points
write_rel:	resd	1		; ptr to write() function
factor:		resd	1		; number being tested for factorhood
exit_rel:	resd	1		; ptr to _exit() function
buf:		resb	buf_size	; buffer for I/O
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
;; section and the three functions imported from libc: read(),
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
read_sym	equ	3
		dd	read_name
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
		dd	dataorg + read_rel
		db	1			; R_386_32
		db	read_sym
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
read_name	equ	$ - dynstr
		db	'read', 0
write_name	equ	$ - dynstr
		db	'write', 0
dynstrsz	equ	$ - dynstr

;;
;; The program proper
;;

;; factorize is the main subroutine of the program. It is called with edi
;; pointing to an NUL-terminated string representing the number to factor.
;; Upon return, eax contains a nonzero value if an error occurred (i.e.,
;; an invalid number stored at edi).

factorize:

;; The first step is to translate the string into a number. 10.0 and 0.0
;; are pushed onto the floating-point stack, and the start of the string
;; is saved in esi.

		push	byte 10
		fild	dword [esp]
		pop	eax
		fldz
		mov	esi, edi

;; Each character in the string is checked; if it is not in the range
;; of '0' to '9' inclusive, an error message is displayed and the
;; subroutine aborts. Otherwise, the top of the stack is multiplied by
;; ten and the value of the digit is added to the product. The loop
;; exits when a NUL byte is found. The subroutine also aborts if there
;; are more than eighteen digits in the string.

.atoiloop:
		lodsb
		or	al, al
		jz	.atoiloopend
		fmul	st0, st1
		sub	al, '0'
		jc	.errbadnum
		cmp	al, 10
		jnc	.errbadnum
		mov	[ebx], eax
		fiadd	dword [ebx]
		jmp	short .atoiloop
.errbadnum:
		fcompp
		push	byte errmsgbadnumsz
		lea	eax, [byte ebx + errmsgbadnum - dataorg]
		push	eax
		push	byte stderr
		call	[byte ebx + write_rel]
		add	esp, byte 12
		mov	al, 1
		ret
.atoiloopend:
		sub	esi, edi
		cmp	esi, byte 20
		jnc	.errbadnum

;; The number is stored as a 10-byte BCD number and the itoa80
;; subroutine is used to display the number, followed by a colon. If
;; the number is less than two, no factoring should be done and the
;; subroutine skips ahead to the final stage.
							; num  junk
		fld	st0				; num  num  junk
		fxch	st2				; junk num  num
		fcomp	st0				; num  num
		fbstp	[ebx]				; num
		mov	al, ':'
		call	itoa80nospc
		xor	ecx, ecx
		cmp	[byte ebx + 8], ecx
		jnz	.numberok
		cmp	[byte ebx + 4], ecx
		jnz	.numberok
		cmp	dword [ebx], byte 2
		jc	near .earlyout
.numberok:

;; The factorconst subroutine is called three times, with bcd80 set to
;; two, three, and five, respectively.

		mov	[byte ebx + 8], ecx
		mov	[byte ebx + 4], ecx
		mov	cl, 2
		mov	[ebx], ecx
		call	factorconst			; junk num
		fstp	st0				; num
		inc	dword [ebx]
		call	factorconst			; junk num
		fstp	st0				; num
		mov	byte [ebx], 5
		call	factorconst			; junk num

;; If the number is now equal to one, the subroutine is finished and
;; exits early.

		fld1					; 1.0  junk num
		fcomp	st2				; junk num
		fnstsw	ax
		and	ah, 0x40
		jnz	.quitfactorize
		xor	eax, eax

;; factor is initialized to 7, and edi is initialized with a sequence
;; of eight four-bit values that represent the cycle of differences
;; between subsequent integers not divisible by 2, 3, or 5. The
;; subroutine then enters its main loop.

		mov	al, 7
		mov	[byte ebx + factor], eax
		fld	st1				; num  junk num
		fidiv	dword [byte ebx + factor]	; quot junk num
		mov	edi, 0x42424626

;; The loop returns to this point when the last tested value was not a
;; factor. The next value to test (for which the division operation
;; should just be finishing up) is moved into esi, and factor is
;; incremented by the value at the bottom of edi, which is first
;; advanced to the next four-bit value.

.notafactor:
		mov	esi, [byte ebx + factor]
		rol	edi, 4
		mov	eax, edi
		and	eax, byte 0x0F
		add	[byte ebx + factor], eax

;; The main loop of the factorize subroutine. The quotient from the
;; division of the number by the next potential factor is stored, and
;; the division for the next iteration is started.

.mainloop:						; quot quo0 num
		fld	st0				; quot quot quo0 num
		fxch	st2				; quo0 quot quot num
		fcomp	st0				; quot quot num
		fstp	tword [byte ebx + quotient]	; quot num
		fld	st1				; num  quot num
		fidiv	dword [byte ebx + factor]	; quo2 quot num

;; The integer portion of the quotient is isolated and tested against
;; the divisor (i.e., the potential factor). If the quotient is
;; smaller, then the loop has passed the number's square root, and no
;; more factors will be found. In this case, the program prints out
;; the current value for the number as the last factor, followed by a
;; newline character, and the subroutine ends.

		mov	edx, [byte ebx + quotient + 4]
		mov	ecx, 16383 + 31
		sub	ecx, [byte ebx + quotient + 8]
		js	.keepgoing
		mov	eax, edx
		shr	eax, cl
		cmp	eax, esi
		jnc	.keepgoing
		fxch	st2
		fbstp	[ebx]
.earlyout:	call	itoa80
.quitfactorize:	fcompp
		push	byte 1
		lea	edi, [byte ebx + newline - dataorg]
		jmp	short finalwrite

;; Now the integer portion of the quotient is shifted out. If any
;; nonzero bits are left, then the number being tested is not a
;; factor, and the program loops back.

.keepgoing:	mov	eax, [byte ebx + quotient]
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
;; division in progress is ignored. The new factor is displayed, and
;; then is tested again. If this was the first time this factor was
;; tested, then edi is reset back.
							; quo0 num  junk
		cmp	[byte ebx + factor], esi
		jz	.repeating
		ror	edi, 4
		mov	[byte ebx + factor], esi
.repeating:	fstp	st0				; num  junk
		fxch	st1				; junk num
		fild	dword [byte ebx + factor]	; fact junk num
		fld	st0				; fact fact junk num
		fbstp	[ebx]				; fact junk num
		fdivr	st0, st2			; quot junk num
		push	edi
		call	itoa80
		pop	edi
		mov	esi, [byte ebx + factor]
		jmp	short .mainloop

;; itoa80 is the numeric output subroutine. When the subroutine is
;; called, the number to be displayed should be stored in the bcd80
;; buffer, in the 10-byte BCD format. A space is prepended to the
;; output, unless itoa80nospc is called, in which case the character
;; in al is suffixed to the output.

itoa80:
		mov	al, ' '
itoa80nospc:

;; edi is pointed to buf, esi is pointed to bcd80, and ecx, the
;; counter, is initialized to eight.

		xor	ecx, ecx
		mov	cl, 8
		mov	esi, ebx
		lea	edi, [byte ebx + buf + 1]
		push	eax
		push	edi

;; The BCD number is read from top to bottom (the sign byte is
;; ignored). The two nybbles in each byte are split apart, turned into
;; ASCII digits, and stored in buf.

.loop:		mov	al, [esi + ecx]
		aam	16
		xchg	al, ah
		add	ax, 0x3030
		stosw
		dec	ecx
		jns	.loop

;; The end of the string is stored in edx, and then edi is set to
;; point to the first character of the string that is not a zero
;; (unless the string is all zeros, in which case the last zero is
;; retained.

		mov	edx, edi
		pop	edi
		add	ecx, byte 19
		mov	al, '0'
		repz scasb
		dec	edi

;; If al contains a space, it is added to the start of the string.
;; Otherwise, al is added to the end.

		pop	eax
		cmp	al, ' '
		jz	.prefix
		mov	[edx], al
		inc	edx
		jmp	short .suffix
.prefix:	dec	edi
		mov	[edi], al
.suffix:

;; The string is written to standard output, and the subroutine ends.

		sub	edx, edi
		push	edx
finalwrite:	push	edi
		push	byte stdout
		call	[byte ebx + write_rel]
		add	esp, byte 12
		dec	eax
		ret

;; Here is the program's entry point.

_start:

;; argc and argv[0] are removed from the stack and discarded. ebx is
;; initialized to point to the data.

		pop	edi
		pop	ebx
		mov	ebx, dataorg

;; If argv[1] is NULL, then the program proceed to the input loop. If
;; argv[1] begins with a dash, then the help message is displayed.
;; Otherwise, the program begins readings the command-line arguments.

		pop	edi
		or	edi, edi
		jz	.inputloop
		cmp	byte [edi], '-'
		jz	.dohelp

;; The factorize subroutine is called once for each command-line
;; argument, and then the program exits, with the exit code being
;; the return value from the last call to factorize.

.argloop:
		call	factorize
		pop	edi
		or	edi, edi
		jnz	.argloop
.mainexit:	push	eax
		call	[byte ebx + exit_rel]

;; The input loop routine. edi is pointed to buf, and esi is
;; initialized to one less than the size of buf.

.inputloop:
		lea	edi, [byte ebx + buf]
		push	byte buf_size - 1
		pop	esi

;; The program reads and discards one character at a time, until a
;; non-whitespace character is seen (or until no more input is
;; available, in which case the program exits).

.preinloop:
		call	readchar
		jns	.mainexit
		mov	al, [edi]
		cmp	al, ' '
		jz	.preinloop
		cmp	al, 9
		jc	.incharloop
		cmp	al, 14
		jc	.preinloop

;; The first non-whitespace character is stored at the beginning of
;; buf. The program continues to read characters until there is no
;; more input, there is no more room in buf, or until another
;; whitespace character is found.

.incharloop:
		inc	edi
		dec	esi
		jz	.infinish
		call	readchar
		jns	.infinish
		mov	al, [edi]
		cmp	al, ' '
		jz	.infinish
		cmp	al, 9
		jc	.incharloop
		cmp	al, 14
		jnc	.incharloop

;; A NUL is appended to the string obtained from standard input, the
;; factorize subroutine is called, and the program loops.

.infinish:
		mov	byte [edi], 0
		lea	edi, [byte ebx + buf]
		call	factorize
		jmp	short .inputloop

;; The program displays the help message on standard output and exits
;; with an exit value of zero.

.dohelp:
		push	byte 0
		push	dword [byte ebx + exit_rel]
		push	byte helpmsgsz
		lea	edi, [byte ebx + helpmsg - dataorg]
		jmp	short finalwrite

;; The readchar subroutine reads a single byte from standard input and
;; stores it in edi. Upon return, the sign flag is cleared if an error
;; occurred or if no input was available.

readchar:
		push	byte 1
		push	edi
		push	byte stdin
		call	[byte ebx + read_rel]
		add	esp, byte 12
		neg	eax
return:		ret

;; The factorconst subroutine, called by factorize, repeatedly divides
;; the number at the top of the floating-point stack by the integer
;; (which must be under 10) stored in bcd80 as long as the number
;; continues to divide evenly. For each successful division, the
;; number is also displayed on standard output. Upon return, the
;; quotient of the failed division is at the top of the floating-point
;; stack, and the factored number is below that.

factorconst:
		fld	st0				; num  num
		fidiv	dword [ebx]			; quot num
		fld	st0				; quot quot num
		frndint					; quoi quot num
		fcomp	st1				; quot num
		fnstsw	ax
		and	ah, 0x40
		jz	return
		fxch	st1				; num  quot
		fstp	st0				; quot
		call	itoa80
		jmp	short factorconst

;; Messages to the user.

helpmsg:	db	'factor: version 1.1', 10
		db	'Print the prime factors of arguments/input. '
		db	'Maximum value is 10^18 - 1.', 10
helpmsgsz	equ	$ - helpmsg
errmsgbadnum:	db	'invalid number'
newline:	db	10
errmsgbadnumsz	equ	$ - errmsgbadnum

;; End of file image.

filesz		equ	$ - $$

dataorg		equ	$$ + ((filesz + 3) & ~3)

memsz		equ	dataorg + data_size - $$
