;; snake.asm: Copyright (C) 2001 by Brian Raiter, under the GNU
;; General Public License. No warranty.
;;
;; To build:
;;	nasm -f bin -o snake snake.asm && chmod +x snake
;;
;; Use 4, n, or the left-arrow key to turn the snake left.
;; Use 5, m, or the right-arrow key to turn the snake right.
;; Eating the blocks adds to your score and makes you grow longer.
;; Avoid the walls and your own tail.
;;
;; Note that this program assumes that it is running on a terminal
;; that has the VT100 line-drawing characters as the alternate
;; character set. If you're playing on a Linux console and you see a
;; bunch of lowercase letters instead of lines, try typing "echo -e
;; '\e(B\e)0'" before starting the game. If that doesn't help, or if
;; instead you see a jumble of non-ASCII characters, you may need to
;; switch to a console font with the full VT100 character set in order
;; to play.

BITS 32

;; Macros for accessing addresses in .text and .bss via ebp.

%define	DATAOFF(addr)		byte ebp + ((addr) - score)

;; The system calls used by this program.

%define	SC_exit			1	; exit(bl)
%define	SC_read			3	; eax = read(ebx, ecx, edx)
%define	SC_write		4	; eax = write(ebx, ecx, edx)
%define SC_pause		29	; eax = pause()
%define SC_ioctl		54	; eax = ioctl(ebx, ecx, edx)
%define SC_sigaction		67	; eax = sigaction(ebx, ecx, edx)
%define SC_gettimeofday		78	; eax = gettimeofday(ebx, ecx)
%define SC_setitimer		104	; eax = setitimer(ebx, ecx, edx)
%define SC_select		142	; eax = select(ebx, ecx, edx, esi, edi)

;; ioctl values

%define TCGETS			0x5401
%define TCSETS			0x5402

%define ICANON			0000002
%define ECHO			0000010

;; signal values

%define	SIGALRM			14

;; ASCII character names

%define	NL			012q
%define	FF			014q
%define	SO			016q
%define	SI			017q
%define	ESC			033q
%define	CSI			233q

;; How the program tracks directions. (These values are also ORed
;; together to indicate the various VT-100 line-drawing characters.)

%define	NORTH			1
%define	EAST			2
%define	SOUTH			4
%define	WEST			8

;; Dimensions of the screen and the playing area.

%define WIDTH			78
%define	HEIGHT			24
%define	CENTERPOINT		((HEIGHT - 1) / 2) * WIDTH + (WIDTH - 1) / 2
%define	RIGHTCOL		'79'
%define	BOTTOMROW		'24'

;; The number of turns it takes for the food to decay.

%define	FOOD_HALFLIFE		42

;; Indices for messages to the user.

%define	helpmsgcode		51
%define	endmsgcode		91
%define	filledmsgcode		101
%define	ouromsgcode		135
%define	philosmsgcode		172
%define	poetrymsgcode		209


%define	origin			0x08048000

;; The ELF header and program segment table.

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
		dd	$$					;   p_vaddr
		dd	$$					;   p_paddr
		dd	filesz					;   p_filesz
		dd	memsz					;   p_memsz
		dd	7					;   p_flags
		dd	0x1000					;   p_align
phdrsize	equ	$ - phdr


;; getkey retrieves a single character from standard input if one is
;; waiting to be read.
;;
;; input:
;; ecx = pointer to a byte in memory at which to store the character.
;;
;; output:
;; al = the contents of [ecx].
;; [ecx] = the character input, or zero if no character was available,
;;         or 'q' if standard input could not be read.
;; eax, ebx, and edx are altered.

getkey:

;; Call select, waiting on the first file descriptor only, and setting
;; the time to wait to zero. If select returns an error, return 'q' to
;; the caller. If select returns zero, return zero to the caller.
;; Otherwise, call read.

		xor	eax, eax
		xor	ebx, ebx
		cdq
		mov	[ecx], al
		pusha
		inc	ebx
		mov	ecx, fdset
		mov	[ecx], ebx
		xor	esi, esi
		lea	edi, [DATAOFF(timer)]
		mov	[edi], eax
		mov	[byte edi + 4], eax
		mov	al, 142
		int	0x80
		dec	eax
		popa
		jl	.return
		js	.retquit
		mov	al, SC_read
		inc	edx
		int	0x80

;; If read returned zero (i.e., EOF) or an error, return 'q' to the
;; caller. Otherwise, return the retrieved character.

		dec	eax
		jz	.return
.retquit:	mov	byte [ecx], 'q'
.return:	mov	al, [ecx]
		ret

;; zmove appends a cursor-positioning VT-100 escape sequence to the
;; given buffer.
;;
;; input:
;; eax = the zero-based coordinates to move the cursor to, in the form
;;       y * WIDTH + x.
;; edi = the buffer to write the escape sequence to.
;;
;; output:
;; edi = the position following the escape sequence.
;; eax and edx are altered.

zmove:
		mov	dl, WIDTH
		div	dl
		push	eax
		inc	eax
		aam
		bswap	eax
		add	eax, ESC | ('[00' << 8)
		stosd
		pop	eax
		mov	al, ah
		inc	eax
		aam
		bswap	eax
		shr	eax, 8
		add	eax, ';00H'
		stosd
		ret

;; addvtchar appends a VT-100 cursor-position escape sequence followed
;; by a line-drawing character to the given buffer.
;;
;; eax = the zero-based coordinates to move the cursor to.
;; dl  = the line-drawing character, given as a combination of
;;	 direction bit flags.
;; edi = the buffer to write the escape sequence to.
;;
;; output:
;; edi = the position following the line-drawing character.
;; eax, ebx, and edx are altered.

addvtchar:
		push	edx
		mov	[DATAOFF(scr) + eax], dl
		call	zmove
		pop	eax
		mov	edx, eax
		lea	ebx, [DATAOFF(vtlines)]
		xlatb
		stosb
		ret

;; refresh writes the contents of outbuf to standard output.
;;
;; input:
;; edi = one past the last character in outbuf to write.
;;
;; output:
;; edi = outbuf.
;; eax, ebx, ecx, and edx are altered.

refresh:
		mov	eax, ESC | ('[' << 8) | (BOTTOMROW << 16)
		stosd
		mov	eax, ';0H' | (SI << 24)
		stosd
		mov	edx, edi
		mov	edi, outbuf
		mov	ecx, edi
		sub	edx, ecx
		xor	ebx, ebx
		lea	eax, [byte ebx + SC_write]
		inc	ebx
		int	0x80
tick:		ret


;; The program begins here.

_start:

;; ebp is set to point to an address near the top of the .bss
;; section. It will retain this value throughout the program.

		mov	ebp, score

;; The attributes of the tty connected to standard input are
;; retrieved, and then canonical-mode and input-echoing are turned
;; off. This puts the terminal in a mode similar to what ncurses calls
;; cbreak mode. The original attributes are remembered so that they
;; can be restored at the end.

		lea	edx, [DATAOFF(termios)]
		mov	ecx, TCGETS
		xor	ebx, ebx
		lea	eax, [byte ebx + SC_ioctl]
		int	0x80
		mov	eax, [DATAOFF(termios.lflag)]
		push	eax
		and	eax, byte ~(ICANON | ECHO)
		mov	[DATAOFF(termios.lflag)], eax
		inc	ecx
		lea	eax, [byte ebx + SC_ioctl]
		int	0x80
		pop	dword [DATAOFF(termios.lflag)]

;; A do-nothing signal hander is installed for SIGALRM, and then an
;; interval timer is set up to go off every 0.2 seconds.

		lea	eax, [byte ebx + SC_sigaction]
		cdq
		mov	bl, SIGALRM
		lea	ecx, [DATAOFF(sigact)]
		int	0x80
		lea	ecx, [DATAOFF(timer)]
		mov	eax, 200 * 1000
		mov	[byte ecx + 4], eax
		mov	[byte ecx + 12], eax
		cdq
		xor	ebx, ebx
		lea	eax, [byte ebx + SC_setitimer]
		int	0x80

;; The scr buffer is initialized by adding the outer walls of the
;; playing field.

		lea	edi, [DATAOFF(scr)]
		mov	al, SOUTH | EAST
		stosb
		mov	al, EAST | WEST
		push	byte WIDTH - 2
		pop	ecx
		rep stosb
		mov	al, SOUTH | WEST
		stosb
		mov	esi, edi
		mov	al, NORTH | SOUTH
		stosb
		add	edi, byte WIDTH - 2
		stosb
		mov	cl, ((HEIGHT - 3) * WIDTH) & 0xFF
		mov	ch, ((HEIGHT - 3) * WIDTH) >> 8
		rep movsb
		mov	al, NORTH | EAST
		stosb
		mov	al, EAST | WEST
		mov	cl, WIDTH - 2
		rep stosb
		mov	byte [edi], NORTH | WEST

;; edi is initialized to point to the output buffer. It will continue
;; to point into this buffer for the rest of the program.

		mov	edi, outbuf

;; The snake is set to begin in the middle of the playing field, and
;; is set to extend by one unit. A food block is placed there as well,
;; but having already expired so that the program will create a new
;; food block on the first iteration of the main loop.

		mov	eax, CENTERPOINT
		mov	[snake], eax
		mov	byte [DATAOFF(scr) + eax], EAST | WEST
		mov	[DATAOFF(foodpos)], eax

;; The current time is used to initialize our pseduorandom-number
;; generator, and is also used to select between EAST and WEST as the
;; snake's starting direction of movement. A Ctrl-L keystroke is
;; inserted into the input queue, so that the program will redraw
;; everything on the first iteration of the main loop.

		lea	ebx, [DATAOFF(rndseed)]
		lea	eax, [byte ecx + SC_gettimeofday]
		int	0x80
		add	eax, [ebx]
		mov	al, WEST
		jp	.gowest
		mov	al, EAST
.gowest:	mov	[DATAOFF(dir)], al
		mov	byte [DATAOFF(key + 1)], FF

;; The main loop, where the program will remain until the game ends.

mainloop:

;; Examine the input queue. If a keystroke is waiting in there, then
;; remove it and use it as this iteration's keystroke. (If two
;; keystrokes are waiting there, remove the first one and move the
;; second one into its position.)

		lea	ecx, [DATAOFF(key)]
		mov	al, [byte ecx + 1]
		or	al, al
		jz	.readkey
		xor	edx, edx
		cmp	dl, [byte ecx + 2]
		jz	.retlast
		xchg	dl, [byte ecx + 2]
.retlast:	mov	[byte ecx + 1], dl
.retkey:	mov	[ecx], al
		jmp	short .endkeys

;; Otherwise, check for an incoming character. If anything besides ESC
;; is returned (including zero, indicating no keys have been pressed),
;; then proceed with that key. Otherwise, check for a second
;; character. If anything besides '[' or 'O' is returned, then put the
;; character in the input queue and proceed with the ESC. Otherwise,
;; check for a third character. If anything besides 'C' or 'D' is
;; returned, then put it and the previous character into the input
;; queue and proceed with the ESC. Otherwise, replace the arrow-key
;; sequence with an 'm' or an 'n', as appropriate.

.readkey:	call	getkey
		cmp	al, ESC
		jnz	.endkeys
		inc	ecx
		call	getkey
		cmp	al, '['
		jz	.getthird
		cmp	al, 'O'
		jnz	.endkeys
.getthird:	inc	ecx
		call	getkey
		cmp	al, 'C'
		jz	.acceptarrow
		cmp	al, 'D'
		jnz	.endkeys
.acceptarrow:	add	al, 'm' - 'C'
		movzx	eax, al
		mov	[byte ecx - 2], eax
.endkeys:

;; If a 'q' was retrieved, exit the program at once. If a Ctrl-L was
;; retrieved, then redraw the screen.

		cmp	al, 'q'
		jz	near leavegame
		cmp	al, FF
		jnz	.continue

;; The screen is erased, and the VT-100 line-drawing character set is
;; selected. The scr array is read, and each element is translated
;; into one of the line-drawing characters and output, with a newline
;; inserted at the end of each row. Finally, the normal character set
;; is re-selected, and the program jumps to the routine to display the
;; current score.

		mov	eax, ESC | ('[H' << 8) | (ESC << 24)
		stosd
		mov	eax, '[J' | (SO << 16) | (SO << 24)
		stosd
		lea	esi, [DATAOFF(scr)]
		lea	ebx, [DATAOFF(vtlines)]
		push	byte HEIGHT
		pop	ecx
.initoutloop:	mov	ch, WIDTH
.lineoutloop:	lodsb
		xlatb
		stosb
		dec	ch
		jnz	.lineoutloop
		dec	ecx
		jz	.outloopend
		mov	al, 10
		stosb
		jmp	short .initoutloop
.outloopend:	mov	al, SI
		stosb
		jmp	near .drawscore
.continue:

;; The pointers to the positions of the snake's head and tail are
;; retrieved. If the snake is currently growing, decrement the growth
;; counter and continue on. Otherwise, erase the end of the tail and
;; advance the tail pointer.

		mov	esi, [DATAOFF(ringhead)]
		mov	ebx, [DATAOFF(ringtail)]
		dec	dword [DATAOFF(growing)]
		jns	.growing
		inc	dword [DATAOFF(growing)]
		mov	eax, [snake + ebx*4]
		mov	dl, 0
		call	addvtchar
		inc	byte [DATAOFF(ringtail)]
.growing:

;; The position of the snake's head is retrieved, and the current
;; direction of the snake's movement is used to advance the head to
;; the next position, which is stored in esi.

		mov	esi, [snake + esi*4]
		mov	al, [DATAOFF(dir)]
		rcr	eax, 1
		jnc	.notnorth
		sub	esi, byte WIDTH
.notnorth:	rcr	eax, 1
		adc	esi, byte 0
		rcr	eax, 1
		jnc	.notsouth
		add	esi, byte WIDTH
.notsouth:	rcr	eax, 1
		sbb	esi, byte 0
.notwest:

;; The location of the food block is compared with the new position of
;; the head. If they intersect, the program jumps ahead. Otherwise,
;; the life counter for the food is decremented. If it reaches zero,
;; it is reset, and the value of the food is decremented. If that
;; reaches zero, the program skips ahead to the newfood routine.
;; Otherwise, the new value of the food is displayed at the food's
;; location.

		mov	eax, [DATAOFF(foodpos)]
		sub	eax, esi
		jns	.positive
		neg	eax
.positive:	dec	eax
		jle	.eating
		dec	byte [DATAOFF(foodlife)]
		jnz	.endfoodrelay
		dec	byte [DATAOFF(foodvalue)]
		jz	.newfood
		mov	byte [DATAOFF(foodlife)], FOOD_HALFLIFE
		mov	eax, [DATAOFF(foodpos)]
		call	zmove
		mov	eax, ESC | ('[7m' << 8)
		stosd
		mov	eax, '0 ' | (ESC << 16) | ('[' << 24)
		add	al, [DATAOFF(foodvalue)]
		stosd
		mov	eax, '27m' | (SI << 24)
		stosd
.endfoodrelay:	jmp	near .endfood

;; Eat the food! The current value of the food is added to the score,
;; and to the snake's growth counter, and the new score is displayed
;; to the right of the playing field in decimal.

.eating:	mov	ecx, [DATAOFF(foodvalue)]
		add	[DATAOFF(growing)], ecx
		shl	dword [DATAOFF(growing)], 1
		add	[DATAOFF(score)], ecx
.drawscore:	mov	eax, [DATAOFF(score)]
		mov	bl, 10
		mov	edx, '[:;' | ((RIGHTCOL & 0x00FF) << 24)
.updscoreloop:	div	bl
		push	eax
		mov	al, ESC
		stosb
		xchg	eax, edx
		dec	ah
		stosd
		xchg	eax, edx
		mov	al, (RIGHTCOL & 0xFF00) >> 8
		stosb
		add	ax, ('H' - ((RIGHTCOL & 0xFF00) >> 8)) | ('0' << 8)
		stosw
		pop	eax
		mov	ah, 0
		or	eax, eax
		jnz	.updscoreloop

;; If the program arrived here due to a Ctrl-L keystroke, then the
;; program skips ahead again to the food-block display routine.

.newfood:	mov	eax, [DATAOFF(foodpos)]
		mov	ecx, [DATAOFF(foodvalue)]
		cmp	byte [DATAOFF(key)], FF
		jz	.drawfood

;; The old food block is erased.

		dec	eax
		call	zmove
		mov	al, ' '
		stosb
		stosb
		stosb

;; The scr array is examined, and every location where there are three
;; blanks in a row is added to the spaces array. (The new position of
;; the snake's head is temporarily marked so that the food will not be
;; placed there.) If the playing field has no such location, the
;; program exits the main loop.

		xor	[DATAOFF(scr) + esi], al
		xor	ebx, ebx
		mov	ecx, WIDTH * (HEIGHT - 1)
.spaceloop:	mov	[spaces + ebx*4], ecx
		cmp	dword [DATAOFF(scr - 2) + ecx], 256
		adc	ebx, byte 0
		dec	ecx
		jnz	.spaceloop
		xor	[DATAOFF(scr) + esi], al
		mov	dl, filledmsgcode
		or	ebx, ebx
		jz	near endgame

;; Otherwise, a new pseudorandom number is generated, and scaled to be
;; between zero (inclusive) and seven times the number of locations in
;; the spaces array (exclusive).

.spaceforrent:	push	ebx
		fld	dword [DATAOFF(seventothem31)]
		fimul	dword [esp]
		mov	eax, [DATAOFF(rndseed)]
		mov	edx, 1103515245
		mul	edx
		add	eax, 12345
		shl	eax, 1
		shr	eax, 1
		mov	[DATAOFF(rndseed)], eax
		fimul	dword [DATAOFF(rndseed)]
		fsub	dword [DATAOFF(onehalf)]
		fistp	dword [esp]
		pop	eax

;; The random number is divided by seven. The quotient is the index to
;; the location for the new food block, and the remainder plus three
;; is the starting value for the food block.

		cdq
		mov	cl, 7
		div	ecx
		mov	eax, [spaces + eax*4]
		mov	[DATAOFF(foodpos)], eax
		lea	ecx, [byte edx + 3]
		mov	[DATAOFF(foodvalue)], cl
		mov	byte [DATAOFF(foodlife)], FOOD_HALFLIFE

;; The new food block is display at its chosen location. If the
;; program arrived here due to a Ctrl-L keystroke, it immediately
;; flushes the output buffer and returns to the top of the main loop.

.drawfood:	dec	eax
		call	zmove
		mov	eax, ESC | ('[7m' << 8)
		stosd
		mov	eax, ' 0 ' | (ESC << 24)
		add	ah, cl
		stosd
		mov	eax, '[27m'
		stosd
		cmp	byte [DATAOFF(key)], FF
		jnz	.endfood
		call	refresh
		jmp	near mainloop
.endfood:	mov	al, SO
		stosb

;; dl is set to the current direction of movement, and dh is set to
;; the opposite direction.

		mov	dl, [DATAOFF(dir)]
		mov	dh, dl
		shl	dh, 4
		or	dh, dl
		shr	dh, 2
		and	dh, 15

;; Having dispensed with the food block, the program now checks to see
;; if anything else is already at the new location of the snake's
;; head. If not, the new location is stored and the pointer to the
;; head is advanced.

		mov	al, [DATAOFF(scr) + esi]
		or	al, al
		jnz	collide
		inc	byte [DATAOFF(ringhead)]
		mov	eax, [DATAOFF(ringhead)]
		mov	[snake + eax*4], esi

;; The keystroke is retrieved, and, if appropriate, the program shifts
;; the snake's direction of movement to the right or the left.

		mov	al, dl
		shl	al, 4
		or	dl, al
		mov	al, [DATAOFF(key)]
		cmp	al, '4'
		jz	.left
		cmp	al, '6'
		jz	.right
		or	al, 'a' - 'A'
		cmp	al, 'n'
		jz	.left
		cmp	al, 'm'
		jnz	.endturn
.right:		shr	dl, 2
.left:		shr	dl, 1
.endturn:	and	dl, 15
		mov	[DATAOFF(dir)], dl

;; The opposite of the old direction and the new direction are
;; combined to indicate the line-drawing character to display at the
;; current location.

		or	dl, dh
		mov	eax, esi
		call	addvtchar

;; The contents of the output buffer are displayed, and the program
;; goes to sleep until the next SIGALRM arrives, whereupon it begins
;; the next iteration of the main loop.

		call	refresh
		push	byte SC_pause
		pop	eax
		int	0x80
		jmp	near mainloop

;; Crash! The snake has hit something, either a wall or its own
;; tail. The opposite of the direction of travel is combined with
;; whatever is already here to produce the updated line-drawing
;; character for this location. The program also checks to see if the
;; snake actually swallowed its own tail, as opposed to merely running
;; into it, and makes a smartaleck comment if so.

collide:	test	al, dh
		pushf
		or	al, dh
		mov	dl, al
		mov	eax, esi
		call	addvtchar
		popf
		jz	.noloop
		mov	dl, ouromsgcode
		mov	eax, [DATAOFF(ringtail)]
		cmp	esi, [snake + eax*4]
		jz	endgame
.noloop:

;; If the game ended with a score of zero, the program provides a
;; brief help message in case the user doesn't know how the game
;; works. Other conditions of the game produce different messages.

		mov	eax, [DATAOFF(score)]
		mov	dl, helpmsgcode
		or	eax, eax
		jz	endgame
		mov	dl, poetrymsgcode
		cmp	eax, byte 100
		jz	endgame
		mov	dl, philosmsgcode
		cmp	dword [DATAOFF(growing)], byte 15
		ja	endgame
		mov	dl, endmsgcode

;; The game is over, and a closing message needs to be displayed. The
;; program reads the text from the messages array and into the output
;; buffer, which is then flushed.

endgame:	mov	ebx, msgs
		movzx	edx, dl
		lea	esi, [ebx + edx]
		mov	eax, SI | (ESC << 8) | ('[1' << 16)
		stosd
.msgloop:	lodsd
		dec	esi
		or	al, al
		jz	leavegame
		mov	cl, 4
		xchg	eax, edx
.msgcharloop:	mov	al, 63
		and	al, dl
		xlatb
		stosb
		shr	edx, 6
		dec	cl
		jnz	.msgcharloop
		jmp	short .msgloop
leavegame:	call	refresh

;; The program restores the tty to its original settings, and exits.

		lea	edx, [DATAOFF(termios)]
		mov	ecx, TCSETS
		xor	ebx, ebx
		lea	eax, [byte ebx + SC_ioctl]
		int	0x80
		xchg	eax, ebx
		inc	eax
		int	0x80


;; The messages array.

msgs:

dd	0x49484745, 0x534F4E4A, 0x58575554, 0x2161205B, 0x65646362, 0x68672766
dd	0x2C6C6B69, 0x6F2F6E6D, 0x72317030, 0x34337332, 0x76753574, 0x79387736
dd	0xB01B0F3B, 0x39490AC8, 0x36C7A735, 0x1E74D8DF, 0xE3637CD7, 0x96D7E38C
dd	0xA835FA0D, 0x368D763A, 0x74E36851, 0xD6623352, 0xB000C685, 0x72310AC9
dd	0x49D00D4D, 0x0A293000, 0xD7B5F171, 0xFA0D4E34, 0x368AA135, 0xD43535E8
dd	0x235B49F7, 0x04D95D38, 0x4E728A2A, 0xB000C4F7, 0x3A860AE8, 0x7E37D07E
dd	0xDF48D96A, 0xDA157654, 0x353AD868, 0xEB4E37D4, 0xD7583634, 0x365617A0
dd	0x004DC396, 0xC80AE8B0, 0x7B4DA0E5, 0x48D5D161, 0xDD3654DF, 0xA6193687
dd	0x35370D69, 0x654D939C, 0xF68D4DC3, 0xC634D675, 0xB22C2000, 0xE0A4CC82
dd	0x08A23322, 0x3320AC8B, 0xCD34B829, 0xD4D939D1, 0x4D939D1C, 0x4EB4D0CD
dd	0xD7E83687, 0x36F39A84, 0x9364E747, 0x2C2488CC, 0x0CD342B2, 0x5D4C4354
dd	0x34ED34E7, 0x35D60D4E, 0x5B69A4D7, 0xF40D5DF3, 0xA9FB4DBD, 0xD794D49A
dd	0x364E40D4, 0xC8D34D35, 0x22C2688C, 0xD0CD342B, 0x36874EB4, 0x9A84D7E8
dd	0xE74736F3, 0x4CD6D364, 0x6860DA0E, 0x735274E3, 0x70D4EB39, 0xD939136F
dd	0xCC8D34D4, 0xB22C2788, 0xD728D342, 0xD34335D4, 0xA0DA1D3A, 0x684D635F
dd	0xF3526233, 0x39368D51, 0xCD6E54EB, 0xD4E5A8D0, 0x64E8CD38, 0x8CC8D353
dd	0x2B22C298, 0x2E0A4CC8

ALIGN 4, db 0

;; Two floating-point values, used in the scaling of our pseudorandom
;; numbers.

onehalf:	dd	0x3F000000			; 0.5
seventothem31:	dd	0x31600000			; 7 * 2^-31

;; The table used to translate direction bitflags to VT-100
;; line-drawing characters.

vtlines:	db	' xqmxxltqjqvkuwn'

;; The structure passed to the sigaction system call. (The other three
;; structure fields are at the top of the .bss section, where they are
;; automatically initialized to zero.)

sigact:		dd	tick

;; The end of the .text section, and of the executable file's contents.

filesz		equ	$ - $$


;; Zero-initialized data.

ABSOLUTE origin + filesz

		resd	3			; the remainder of sigact


;; The player's current score.

score:		resd	1

;; The snake's current direction of movement.

dir:		resd	1

;; The value of the current food block, its decay counter, and its
;; position in the playing field.

foodvalue:	resd	1
foodlife:	resd	1
foodpos:	resd	1

;; Pointers to the where the position of the snake's head and tail are
;; stored in the ring buffer.

ringhead:	resd	1
ringtail:	resd	1

;; The snake's growth counter.

growing:	resd	1

;; The current seed value for the pseudorandom-number generator.

rndseed:	resd	2

;; The input queue.

key:		resb	4

;; The attributes of the user's tty.

termios:	resd	3
.lflag:		resd	1
		resb	44

;; This chunk of memory doubles as a timeval structure and an
;; itimerval structure.

timer:		resd	4

;; The scr array. The snake and the walls of the playing field are
;; stored in this array as direction bitflags.

scr:		resb	WIDTH * HEIGHT

;; The buffer used for storing the program's output.

outbuf:		resb	4692

;; The ring buffer containing the locations that the snake currently
;; passes through.

snake:		resd	256

;; The spaces array, used while selecting a new food block location.

spaces:		resd	WIDTH * HEIGHT

;; The file description bit array used by the select system call.

fdset:		resd	32


memsz		equ	$ - origin
