;; date.asm: Copyright (C) 2001 by Brian Raiter, under the GNU General
;; Public License. No warranty.
;;
;; Usage: date [-u] [=UTIME] [+FORMAT]
;;
;; +FORMAT provides a format string to use to display the time. The
;; special sequences recognized are:
;;
;; %a %A %b %B %c %C %d %D %e %G %h %H %I %j %k %l %m %M %n %p %r
;; %s %S %t %T %u %U %V %w %W %x %X %y %Y %z %Z %%
;;
;; See date(1) and strftime(3) for the complete description of what
;; each of these sequences displays.
;;
;; The -u option indicates that the time should be displayed in UTC,
;; instead of the local time zone.
;;
;; =UTIME is a non-standard extension. It specifies a Unix time,
;; given as the number of seconds since the start of 1970 UTC, to
;; display instead of the current time.
;;
;; If the LANG environment variable is not set, or does not name an
;; available locale, the internal "C" locale data is used instead.


BITS 32

;; The collection of values associated with the calendric
;; representation of a time.

STRUC tmfmt
.ct:		resd	1	; C time (seconds since the Epoch)
.sc:		resd	1	; seconds
.mn:		resd	1	; minutes
.hr:		resd	1	; hours
.yr:		resd	1	; year
.hm:		resd	1	; hour of the meridian (1-12)
.mr:		resd	1	; meridian (0 for AM)
.wd:		resd	1	; day of the week (Sunday=0, Saturday=6)
.w1:		resd	1	; day of the week (Monday=1, Sunday=7)
.dy:		resd	1	; day of the month
.mo:		resd	1	; month (one-based)
.ws:		resd	1	; week of the year (Sunday-based)
.wm:		resd	1	; week of the year (Monday-based)
.wi:		resd	1	; week of the year according to ISO 8601:1988
.yi:		resd	1	; year for the week according to ISO 8601:1988
.yd:		resd	1	; day of the year
.ce:		resd	1	; century (zero-based)
.cy:		resd	1	; year of the century
.zo:		resd	1	; time zone offset
.zi:		resb	6	; time zone identifier
.tz:		resb	10	; time zone name
ENDSTRUC


%define	origin		0x08048000

;; The ELF header and program segement table.

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
		dd	filesize				;   p_filesz
		dd	memsize					;   p_memsz
		dd	7					;   p_flags
		dd	0x1000					;   p_align
phdrsize	equ	$ - phdr


;; mmapfile takes a filename and returns a read-only pointer to its
;; contents.
;;
;; input:
;; ebx = pointer to NUL-terminated filename
;;
;; output:
;; eax = pointer to file's contents
;; sign flag is set if an error occurred
;; ebx and ecx are altered

mmapfile:

;; After opening the file, the fstat system call is used to obtain the
;; size of the file. buf is used to hold the returned stat structure.

		xor	ecx, ecx
		lea	eax, [byte ecx + 5]
		int	0x80
		or	eax, eax
		js	.return
		xchg	eax, ebx
		lea	eax, [byte ecx + 108]
		lea	ecx, [byte edi + buf - tm]
		int	0x80

;; An mmap structure is filled out and handed off to the system call,
;; and the function returns.

		xor	eax, eax
		mov	[byte ecx + 32], ebx
		lea	ebx, [byte ecx + 16]
		mov	[ebx], eax
		mov	[byte ebx + 20], eax
		inc	eax
		mov	[byte ebx + 8], eax
		mov	[byte ebx + 12], eax
		mov	al, 90
		int	0x80
		or	eax, eax
.return:	ret


%macro pause 0
	pushf
	pusha
	xor	ebx, ebx
	lea	ecx, [byte esp + 12]
	lea	edx, [byte ebx + 1]
	lea	eax, [byte ebx + 3]
	int	0x80
	popa
	popf
%endmacro

;; The program proper begins here.

_start:

;; The program begins by filling in the missing "C" locale
;; information. It takes advantage of the fact that all the short
;; names of the weeks and months are just the first three characters
;; of the long names. The start of the week names are copied first,
;; and then the program skips over to the month names.

		mov	esi, clocale + (8 + 7 * 4)
		mov	edi, ca0
		xor	ecx, ecx
.shortformloop:	mov	ebx, [esi + ecx*4]
		mov	eax, [esi - (8 + 7 * 4) + ebx - 1]
		shr	eax, 8
		stosd
		inc	ecx
		cmp	ecx, byte 7
		jnz	.skip
		add	ecx, byte 12
.skip:		cmp	ecx, byte 31
		jnz	.shortformloop
		xor	ecx, ecx

;;;

;; edi is initialized to point to the start of the .bss section. ebp
;; is pointed at the read-only data in the .text section, centered on
;; the list of format characters. It will retain this value throughout
;; the program.

		mov	ebp, fmtchars
		mov	edi, tm

;; Call gettimeofday(), storing the current time in the .ct field.

		mov	ebx, edi
		lea	eax, [byte ecx + 78]
		int	0x80

;; The default format string is stashed in ecx, and the command-line
;; arguments are examined.

		lea	ecx, [byte ebp + defaultfmt - fmtchars]
		pop	esi
		pop	esi

;; An argument beginning with a + provides a display format string. If
;; no such argument is present, the built-in formats is used. The
;; presence of a -u option indicates that the time should be displayed
;; for UTC instead of local time. An argument beginning with = provides
;; a C time to use instead of the current time.

.argvloop:	pop	esi
		or	esi, esi
		jz	.argvloopend
		lodsb
		cmp	al, '+'
		jnz	.notfmt
		mov	ecx, esi
		jmp	short .argvloop
.notfmt:	cmp	al, '='
		jnz	.nottime
		xor	eax, eax
		cdq
.atoiloop:	lea	edx, [edx*4 + edx]
		lea	edx, [edx*2 + eax]
		lodsb
		sub	al, '0'
		jc	.atoidone
		cmp	al, 10
		jc	.atoiloop
.atoidone:	mov	[edi], edx
		jmp	short .argvloop
.nottime:	cmp	al, '-'
		jnz	.argvloop
.optloop:	lodsb
		or	al, al
		jz	.argvloop
		cmp	al, 'u'
		jnz	.optloop
		mov	dword [byte edi + tmfmt.tz], 'UTC'
		jmp	short .optloop
.argvloopend:	mov	[byte edi + fmt - tm], ecx

;; The program stores a pointer to the default "C" locale, and then
;; copies over the first part of the localization file name.

		lea	edx, [byte ebp + clocale - fmtchars]
		mov	[byte edi + localedata - tm], edx
		mov	edx, edi
		add	edi, byte buf - tm
		lea	esi, [byte ebp + lctimeprefix - fmtchars]
		push	byte lctimeprefixlen
		pop	ecx
		rep movsb

;; Then the list of environment variables is searched for a variable
;; named LANG. If no such variable is present, the default "C" locale
;; is retained.

.searchenvloop:	pop	esi
		or	esi, esi
		jz	.useclocale
		lodsd
		cmp	eax, 'LANG'
		jnz	.searchenvloop
		lodsb
		cmp	al, '='
		jnz	.searchenvloop

;; If one is present, its value is used to complete the name of the
;; localization file, which is then loaded into memory.

.copylangloop:	lodsb
		stosb
		or	al, al
		jnz	.copylangloop
		dec	edi
		lea	esi, [byte ebp + lctimesuffix - fmtchars]
		movsd
		movsd
		stosb
		mov	edi, edx
		lea	ebx, [byte edi + buf - tm]
		call	mmapfile
		js	.useclocale
		mov	[byte edi + localedata - tm], eax
.useclocale:	mov	edi, edx

;; If time zone information is already present, UTC is to be used and
;; the translation to local time is skipped. Otherwise, the file for
;; the local time zone is loaded.

		xor	edx, edx
		cmp	edx, [byte edi + tmfmt.tz]
		jnz	skipzoning
		lea	ebx, [byte ebp + tzfilename - fmtchars]
		call	mmapfile
		js	skipzoning

;; At the top of the localtime file is a sequence of integers
;; indicating the size of the various parts of the file. Following
;; this is a list of time changes for this zone. (See tzfile(5) for a
;; fuller description of the structure of this file.) The program runs
;; through the list backwards, finding the current subsequence of
;; linear time.

		mov	ebx, [byte eax + 32]
		bswap	ebx
		mov	esi, [byte eax + 36]
		bswap	esi
		lea	esi, [esi*2 + esi]
		mov	ecx, ebx
		add	eax, byte 44
.tmchgloop:	dec	ecx
		jz	.tmchgloopexit
		mov	edx, [eax + ecx*4]
		bswap	edx
		cmp	edx, [edi]
		jg	.tmchgloop
.tmchgloopexit:

;; The index of the current subsequence gives us the offset into the
;; next array, which itself contains indexes into the array of the
;; current attributes of the time zone. These attributes give the
;; offset from GMT in seconds (which is stored in zo), a flag
;; indicating whether or not Daylight Savings is in effect, and a
;; pointer to the current time zone's name (which is stored in tz).

		lea	eax, [eax + ebx*4]
		movzx	ecx, byte [eax + ecx]
		add	eax, ebx
		lea	ecx, [ecx*2 + ecx]
		mov	edx, [eax + ecx*2]
		bswap	edx
		mov	[byte edi + tmfmt.zo], edx
		movzx	ecx, byte [byte eax + ecx*2 + 5]
		lea	eax, [eax + esi*2]
		fild	qword [eax + ecx]
		fistp	qword [byte edi + tmfmt.tz]

;; The current offset from GMT in seconds is then changed into hours
;; and minutes. These are used to create a string of the form +HHMM,
;; stored in zi.

skipzoning:
		mov	al, '+'
		or	edx, edx
		jns	.eastward
		mov	al, '-'
		neg	edx
.eastward:	mov	[byte edi + tmfmt.zi], al
		xchg	eax, edx
		cdq
		lea	ebx, [byte edx + 60]
		div	ebx
		cdq
		div	ebx
		aam
		xchg	eax, edx
		aam
		shl	edx, 16
		lea	eax, [edx + eax + '0000']
		bswap	eax
		mov	[byte edi + tmfmt.zi + 1], eax

;; The current time is loaded into eax, the offset for the time zone
;; is applied. If this makes the current time negative, a year is
;; added to the time, and the start of the Epoch is moved back. The
;; day of the week of the start of the Epoch is stored in esi and on
;; the stack.

		mov	ecx, 1969
		push	byte 4
		pop	esi
		mov	eax, [edi]
		add	eax, [byte edi + tmfmt.zo]
		jge	.positivetime
		add	eax, 365 * 24 * 60 * 60
		dec	ecx
		dec	esi
.positivetime:	push	esi

;; eax holds the number of seconds since the Epoch. This is divided by
;; 60 to get the current number of seconds, by 60 again to get the
;; current number of minutes, and then by 24 to get the current number
;; of hours.

		cdq
		lea	ebx, [byte edx + 60]
		div	ebx
		mov	[byte edi + tmfmt.sc], edx
		cdq
		div	ebx
		mov	[byte edi + tmfmt.mn], edx
		mov	bl, 24
		cdq
		div	ebx
		mov	[byte edi + tmfmt.hr], edx

;; The hours are also tested to determine the current side of the
;; meridian, and the hours of the meridian.

		sub	edx, byte 12
		setae	byte [byte edi + tmfmt.mr]
		ja	.morning
.midnight:	add	edx, byte 12
		jz	.midnight
.morning:	mov	[byte edi + tmfmt.hm], edx

;; eax now holds the number of days since the Epoch. This is divided
;; by seven, after offsetting by the value in esi, to determine the
;; current day of the week.

		push	eax
		add	eax, esi
		mov	bl, 7
		cdq
		div	ebx
		mov	[byte edi + tmfmt.wd], edx
		xor	eax, eax
		cmpxchg	edx, ebx
		mov	[byte edi + tmfmt.w1], edx
		mov	eax, [esp]

;; A year's worth of days are successively subtracted from eax until
;; the current year is determined. The program takes advantage of the
;; fact that every 4th year is a leap year within the range of our
;; Epoch.

		mov	bh, 1
.yrloop:	mov	bl, 110
		inc	ecx
		test	cl, 3
		jz	.leap
		dec	ebx
.leap:		sub	eax, ebx
		jge	.yrloop
		add	eax, ebx
		mov	[byte edi + tmfmt.yr], ecx

;; 1900 or 2000 is subtracted to determined the century and the year
;; of the century.

		mov	ch, 20
		sub	cl, 208
		jnc	.twentieth
		dec	ch
		add	cl, 100
.twentieth:	mov	[byte edi + tmfmt.cy], cl
		mov	[byte edi + tmfmt.ce], ch

;; eax now holds the day of the year, and this is saved in esi. ebx is
;; altered to hold a string of pairs of bits indicating the length of
;; each month over 28 days. Each month's worth of days are
;; successively subtracted from eax until the current month, and thus
;; the current day of the month, is determined.

		mov	esi, eax
		add	ebx, 11000000001110111011111011101100b - 365
		xor	ecx, ecx
		cdq
.moloop:	mov	dl, 7
		shld	edx, ebx, 2
		ror	ebx, 2
		inc	ecx
		sub	eax, edx
		jge	.moloop
		add	eax, edx
		inc	eax
		add	edi, byte tmfmt.dy
		stosd
		xchg	eax, ecx
		stosd

;; The program retrieves from the stack the day of the year, the
;; number of days since the Epoch, and the day of the week at the
;; start of the Epoch, respectively. These are used to calculate the
;; day of the week of January 1st of the current year.

		pop	eax
		pop	ebx
		sub	eax, esi
		add	eax, ebx
		mov	bl, 7
		cdq
		div	ebx
		mov	ecx, edx

;; Using this, the program now determines the current week of the year
;; according to three different measurements. The first uses Sunday as
;; the start of the week, and a partial week at the beginning of the
;; year is considered to be week zero. The second is the same, except
;; that it uses Monday as the start of the week.

		xor	eax, eax
		cmpxchg	ecx, ebx
		lea	eax, [esi + ecx]
		cdq
		div	ebx
		stosd
		dec	ecx
		jnz	.mondaynot1st
		mov	cl, bl
.mondaynot1st:	lea	eax, [esi + ecx]
		cdq
		div	ebx
		stosd

;; Finally, the ISO-8601 week number uses Monday as the start of the
;; week, and requires every week counted to be the full seven days. A
;; partial week at the end of the year of less than four days is
;; counted as week 1 of the following year; likewise, a partial week
;; at the start of the year of less than four days is counted as week
;; 52 or 53 of the previous year. In order to cover all possibilities,
;; the program must examine the current day of the week, and whether
;; the current year and/or the previous year was a leap year. Outside
;; of these special cases, the ISO-8601 week number will either be
;; equal to or one more than the value previously calculated.

		mov	ebx, [byte edi + tmfmt.yr - tmfmt.wi]
		mov	dl, 3
		and	dl, bl
		sub	ecx, byte 4
		adc	al, 0
		jnz	.fullweek
		dec	ebx
		mov	al, 52
		or	ecx, ecx
		jz	.add1stweek
		dec	ecx
		jnz	.wifound
		dec	edx
		jnz	.wifound
.add1stweek:	inc	eax
.fullweek:	cmp	al, 53
		jnz	.wifound
		cmp	dl, 1
		mov	edx, esi
		sbb	dl, 104
		cmp	dl, [byte edi + tmfmt.wd - tmfmt.wi]
		jle	.wifound
		mov	al, 1
		inc	ebx
.wifound:	stosd
		xchg	eax, ebx
		stosd
		inc	esi
		xchg	eax, esi
		stosd

;; All representable values have now been calculated. edi is changed
;; to point to the output buffer. The program then proceeds to render
;; the format string into buf by calling ftime.

		lea	ebx, [byte edi - tmfmt.ce]
		mov	esi, [byte ebx + fmt - tm]
		lea	edi, [byte ebx + buf - tm]
		push	edi
		call	ftime.loop

;; A newline is appended, the string is output to stdout, and the
;; program exits.

		mov	al, 10
		stosb
		pop	ecx
		mov	edx, edi
		sub	edx, ecx
		xor	ebx, ebx
		lea	eax, [byte ebx + 4]
		inc	ebx
		int	0x80
		xor	eax, eax
		xchg	eax, ebx
		int	0x80


;; ftime formats the current time according to a format specification.
;;
;; input:
;; ebx = pointer to the tm structure
;; esi = pointer to the format string (relative to ebp, unless the
;;       function is entered at ftime.loop)
;; edi = output buffer
;;
;; output:
;; edi = the position immediately following the output string
;; eax, ecx, edx, and esi are altered

ftime:

		add	esi, ebp
		jmp	short .loop

;; Whenever a % character is encountered in the format string, the
;; program scans for the character immediately following in the
;; fmtchars array. If it is not found, the % is treated like any other
;; character and copied into outbuf verbatim. The program returns when
;; a NUL byte is found.

.nofmt:		mov	byte [edi], '%'
		inc	edi
.literal:	stosb
.loop:		lodsb
		or	al, al
		jz	return
		cmp	al, '%'
		jnz	.literal
		cmp	byte [esi], 0
		jz	.literal
		lodsb
		push	edi
		mov	edi, ebp
		push	byte fmtcharcount
		pop	ecx
		repnz scasb
		pop	edi
		jnz	.nofmt

;; If the character is found, the value in ecx is used as an offset
;; into three arrays. The fmtfuns array indicates which function in
;; the funclist array to call in order to render the value. The
;; fmtargs array indicates which field in tmfmt to load into eax
;; before calling the function. Finally the fmtpars array provides a
;; value, stored in both esi and edx, that is specific to the function
;; called.

		push	esi
		movzx	eax, byte [byte ebp + fmtargs - fmtchars + ecx]
		mov	eax, [ebx + eax]
		movzx	esi, byte [byte ebp + fmtpars - fmtchars + ecx]
		mov	edx, esi
		mov	cl, [byte ebp + fmtfuns - fmtchars + ecx]

;; The chosen function is called and ftime continues on to the next
;; character in the format string.

		call	[byte ebp + funclist - fmtchars + ecx]
		pop	esi
		jmp	short .loop


;; ftimelocale calls ftime for a format specification stored in the
;; LC_TIME file.
;;
;; input:
;; ebx = pointer to the tm structure
;; edx = offset of the index of the format in the LC_TIME file
;; edi = output buffer
;;
;; output:
;; edi = the position immediately following the output string
;; eax, ecx, edx, and esi are altered

ftimelocale:	mov	esi, [byte ebx + localedata - tm]
		add	esi, [esi + edx]
		jmp	short ftime.loop


;; itoa renders a number in decimal ASCII.
;;
;; input:
;; eax = the number to render
;; dl & 0F = the minimum width to display
;; dl & F0 = the character to pad with on the left

itoa:

;; First, the number is rendered as decimal ASCII in reverse, in a
;; private buffer, using successive divisions by 10.

		lea	esi, [byte ebx + itoabuf - tm + 16]
		push	esi
		push	edx
		push	byte 10
		pop	ecx
.loop:		cdq
		div	ecx
		add	dl, '0'
		dec	esi
		mov	[esi], dl
		or	eax, eax
		jnz	.loop

;; If the string created is shorter than the size given in the low
;; nybble in edx, extra characters are first written to edi to pad out
;; the field to the minimum required length. Then the number is copied
;; from the private buffer and the function returns.

		pop	eax
		mov	cl, 0x0F
		and	cl, al
		add	ecx, esi
		sub	ecx, [esp]
		jle	.nopadding
		and	al, 0xF0
		rep stosb
.nopadding:	pop	ecx
		sub	ecx, esi
		rep movsb
return:		ret


;; putlocalstr selects a string from a list stored in the LC_TIME file
;; and copies it to an output buffer.
;;
;; input:
;; eax = index of the string to copy
;; edx = offset of the list of indexes in the LC_TIME file

putlocalestr:	mov	esi, [byte ebx + localedata - tm]
		add	edx, esi
		add	esi, [edx + eax*4]
.loop:		lodsb
		stosb
		or	al, al
		jnz	.loop
		dec	edi
		ret


;; putstr simply copies a string from the tmfmt structure to an output
;; buffer.

putstr:		add	esi, ebx
.loop:		lodsb
		stosb
		or	al, al
		jnz	.loop
		dec	edi
		ret


;; putchar copies a single character to the output buffer.

putchar:	xchg	eax, edx
		stosb
		ret


;; read-only data


;; The byte array of indexes into funclist, given the function for
;; each format character.

fmtfuns:	db	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	; s S M H k d e m Y C
		db	0, 0, 0, 0, 0, 0, 0, 0, 0, 0	; I l u w U W V G j y
		db	4, 4, 4, 4, 4, 4		; a A h b B p
		db	8, 8, 8				; c x X
		db	12, 12, 12			; r D T
		db	16, 16				; z Z
		db	20, 20, 20			; t n %

;; The byte array of offsets into tmfmt, indicating which field to use
;; for each format. Formats after %p do not use a tmfmt field, and so
;; the values for those formats are omitted.

fmtargs:	db	tmfmt.ct, tmfmt.sc, tmfmt.mn, tmfmt.hr, tmfmt.hr
		db	tmfmt.dy, tmfmt.dy, tmfmt.mo, tmfmt.yr, tmfmt.ce
		db	tmfmt.hm, tmfmt.hm, tmfmt.w1, tmfmt.wd, tmfmt.ws
		db	tmfmt.wm, tmfmt.wi, tmfmt.yi, tmfmt.yd, tmfmt.cy
		db	tmfmt.wd, tmfmt.wd, tmfmt.mo, tmfmt.mo, tmfmt.mo
		db	tmfmt.mr

;; The default format displayed when none is supplied by the user.

defaultfmt:	db	'%a %b %d %T %Z %Y'

;; The byte array of function-specific data for each format. This
;; value is loaded in to edx and esi before calling the function. The
;; values in the first two rows are for itoa, and provide a minimum
;; field width and a padding character (either space or '0'). The
;; values in the next two rows are for ftimelocale, and give an offset
;; into the words memory to an array of pointers to strings. The fifth
;; row, for ftimelocale, contains offsets relative to ebp to format
;; strings. The sixth row, for putlocalestr, contains offsets relative
;; to tmfmt to constant strings. The final row, for putchar, specifies
;; literal characters.

fmtpars:	db	0, 0x32, 0x32, 0x32, 0x22, 0x32, 0x22, 0x32, 0, 0
		db	0x32, 0x22, 0, 0, 0x32, 0x32, 0x32, 0, 0x33, 0x32
		db	0x08, 0x24, 0x3C, 0x3C, 0x6C, 0xA0
		db	0xA8, 0xAC, 0xB0
		db	percentr - fmtchars
		db	percentD - fmtchars, percentT - fmtchars
		db	tmfmt.zi, tmfmt.tz
		db	9, 10

;; The complete list of format characters, in reverse from the
;; corresponding values used in the three preceding byte arrays.

fmtchars:	db	'%ntZzTDrXxcpBbhAayjGVWUwulICYmedkHMSs'
fmtcharcount	equ	$ - fmtchars

;; The list of different functions called to render a format.

funclist:	dd	itoa, putlocalestr, ftimelocale, ftime, putstr, putchar

;; The (incomplete) pathname of the localization file.

lctimeprefix:	db	'/usr/share/locale/'
lctimeprefixlen	equ	$ - lctimeprefix
lctimesuffix	equ	$ - 1
		db	'LC_TIME'

;; The format used to render the %D, %T, and %r formats.

percentD:	db	'%m/%d/%y', 0
percentT:	db	'%H:%M:%S', 0
percentr:	db	'%I:%M:%S %p', 0

;; The pathname of the time zone file.

tzfilename:	db	'/etc/localtime', 0

;; The "C" locale data.

clocale		equ	$ - 8

%define	O(p)	((p) - origin) - (clocale - $$)

		dd	O(ca0), O(ca1), O(ca2), O(ca3), O(ca4), O(ca5), O(ca6)
		dd	O(cA0), O(cA1), O(cA2), O(cA3), O(cA4), O(cA5), O(cA6)
		dd	O(cb00), O(cb01), O(cb02), O(cb03), O(cb04), O(cb05)
		dd	O(cb06), O(cb07), O(cb08), O(cb09), O(cb10), O(cb11)
		dd	O(cB00), O(cB01), O(cB02), O(cB03), O(cB04), O(cB05)
		dd	O(cB06), O(cB07), O(cB08), O(cB09), O(cB10), O(cB11)
		dd	O(cp0), O(cp1), O(cpc), O(percentD), O(percentT)

cA0:		db	'Sunday', 0
cA1:		db	'Monday', 0
cA2:		db	'Tuesday', 0
cA3:		db	'Wednesday', 0
cA4:		db	'Thursday', 0
cA5:		db	'Friday', 0
cA6:		db	'Saturday', 0
cB00:		db	'January', 0
cB01:		db	'February', 0
cB02:		db	'March', 0
cB03:		db	'April', 0
cB04:		db	'May', 0
cB05:		db	'June', 0
cB06:		db	'July', 0
cB07:		db	'August', 0
cB08:		db	'September', 0
cB09:		db	'October', 0
cB10:		db	'November', 0
cB11:		db	'December', 0
cp0:		db	'AM', 0
cp1:		db	'PM', 0
cpc:		db	'%a %b %e %T %Y'


filesize	equ	$ - $$


ABSOLUTE origin + filesize

resb 1
ALIGNB 4

;; The remainder of the "C" locale data, generated at runtime.

ca0:		resb	4
ca1:		resb	4
ca2:		resb	4
ca3:		resb	4
ca4:		resb	4
ca5:		resb	4
ca6:		resb	4
cb00:		resb	4
cb01:		resb	4
cb02:		resb	4
cb03:		resb	4
cb04:		resb	4
cb05:		resb	4
cb06:		resb	4
cb07:		resb	4
cb08:		resb	4
cb09:		resb	4
cb10:		resb	4
cb11:		resb	4


;; The tmfmt structure.

tm:		resb	tmfmt_size

;; The pointer to the top-level format string.

fmt:		resd	1

;; The pointer to the localization file's contents.

localedata:	resd	1

;; itoa's private temporary buffer.

itoabuf:	resb	16

;; The output buffer that the formatting string is rendered into.

buf:		resb	8192


memsize		equ	$ - origin
