;; ls.asm: Copyright (C) 2001 by Brian Raiter <breadbox@muppetlabs.com>,
;; under the GNU General Public License. No warranty. See COPYING.
;;
;; To build:
;;	nasm -f bin -o ls ls.asm && chmod +x ls
;;
;; Usage: ls [-abdilsBCFNR1] [--] [FILE]...
;; -C  columnar output; default if stdout is a tty
;; -1  one file per output line; default if stdout is not a tty
;; -l  "long" output
;; -F  show file type suffixes
;; -i  show inode numbers
;; -s  show size of files in 1K-blocks
;; -d  don't expand directories
;; -a  show all (hidden) files
;; -B  don't show files ending in ~ (backups)
;; -N  don't replace non-graphic characters with a question mark
;; -b  show non-graphic characters as octal escape sequences
;; -R  recursively list contents of subdirectories
;;
;; Differences from GNU ls:
;; * Columnar output isn't very intelligent. (Real columns would
;;   require loading the entire directory contents before displaying.)
;; * There are no sorting options. (Ditto previous obstacle.)
;; * There is no option to print uid/gid by name. (This would require
;;   reading and parsing the appropriate /etc files.)
;; * Showing the file's modification age is less useful than the
;;   modification time. (This would require reading /etc/localtime to
;;   get the proper offset from UTC.)

BITS 32

STRUC dirent
.d_ino:		resd	1
.d_off:		resd	1
.d_reclen:	resw	1
.d_name:	resb	256
ENDSTRUC

;; The top word of the origin is the instruction "push byte 1".

%define	origin			0x016A0000

%define	pathbufsize		4096			; maximum path length
%define	direntsbufsize		8192			; a good buffer size

;; ebp is used to point to the itoa subroutine and nearby data.

%define	DATAOFF(pos)		byte ebp + ((pos) - _ebp_pos_)


;; The ELF header.

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
		dw	(origin & 0x000FFFF)			;   p_vaddr
_start:		push	byte 1
		push	byte 54					;   p_paddr
		jmp	short main
		dd	filesize				;   p_filesz
		dd	memsize					;   p_memsz
		dd	7					;   p_flags
		dd	0x1000					;   p_align
phdrsize	equ	$ - phdr



;; This is the top-level routine of the program. It parses the
;; command-line options, and then calls ls on the remaining arguments.

main:

;; ebp is initialized as our global pointer into the bss area.

		mov	ebp, itoa

;; If calling ioctl on stdout returns a non-zero value, then stdout is
;; not a tty. This determines whether the default output format is
;; columnar (-C) or one-file-per-line (-1).

		pop	eax
		pop	ebx
		mov	ecx, ebx
		mov	ch, 0x54
		lea	edx, [DATAOFF(pathbuf)]
		int	0x80
		mov	[DATAOFF(oneperln)], al

;; After skipping argc and argv[0], the command-line arguments are
;; examined as options until: none are left, an argument that doesn't
;; start with a dash is found, or an argument that starts with a
;; double-dash is found. Each option maps to a byte that is used as a
;; boolean value that is zero by default (with the exception of -1, as
;; noted above, and -C, which causes -l and -1 to be turned off if
;; they were previously on).

		xor	edx, edx
		pop	esi
		pop	esi
.argloop:	pop	esi
		or	esi, esi
		jz	.noargs
		cmp	byte [esi], '-'
		jnz	.argloopend
		lodsb
		cmp	al, [esi]
		jz	.argloopquit
.optloop:	lodsb
		or	al, al
		jz	.argloop
		lea	edi, [DATAOFF(options)]
		lea	ecx, [byte edx + optioncount]
		repnz scasb
		mov	[DATAOFF(optionflags) + ecx], al
		cmp	al, 'C'
		jnz	.optloop
		mov	[DATAOFF(longdisp)], edx
		jmp	short .optloop

;; If a double-dash was encountered, then the program skips to the
;; next argument. If there are no arguments left, a pointer to the
;; string "." is pushed onto the stack. If there is more than one
;; argument present at this point, edx will be set to a non-zero
;; value.

.argloopquit:	pop	esi
		or	esi, esi
		jnz	.argloopend
.noargs:	push	byte '.'
		mov	esi, esp
		push	edx
.argloopend:	mov	edx, [esp]

;; ls is called for each command-line argument remaining. The extra
;; return value from ls is discarded, except to cause edx to be
;; non-zero after the first iteration (if there is more than one
;; iteration.)

.lsiterate:	call	ls
		pop	edx
		pop	esi
		or	esi, esi
		jnz	.lsiterate

;; The directory headings flag is turned on, and any subdirectories
;; stored from the previous calls to ls are displayed in full.

		mov	byte [DATAOFF(dirheads)], ':'
		mov	esi, dirqueue
		call	lsrecurse

;; The last error value, if any, is retrieved, and the program returns
;; to the caller.

		xchg	eax, ebx
		mov	ebx, [DATAOFF(retval)]
		int	0x80


;; ls produces output for a single file or directory.
;;
;; input:
;; esi = buffer containing file name
;; edx = boolean: zero indicates that directories should be expanded
;;       in full, non-zero indicates that a directory should be added
;;       to the directory queue and nothing should be displayed
;;
;; output:
;; stack = pointer to byte following file name
;; all registers (except ebp) are altered

ls:

;; The filename is copied to a special buffer, and the length is
;; determined. The position following the filename in the source
;; buffer is saved on the stack above the return value.

		lea	edi, [DATAOFF(pathbuf)]
		mov	ebx, edi
.savenameloop:	lodsb
		stosb
		or	al, al
		jnz	.savenameloop
		pop	eax
		push	esi
		push	eax
		mov	esi, ebx
		sub	edi, ebx
		dec	edi

;; lstat is called. If an error is returned, the program returns at
;; once. If the file is not a directory, the program jumps to lsfile.
;; If the file is a directory and edx is zero, the program jumps to
;; lsdir. Otherwise, the program falls through to dirpush.

		call	dolstat
		jc	quit1
		jnz	lsfile
		cmp	al, [DATAOFF(hidedirs)]
		jnz	lsfile
		or	edx, edx
		jz	near lsdir


;; dirpush adds a directory to the directory queue.
;;
;; input:
;; esi = pathname
;;
;; output:
;; esi = first byte after pathname
;; eax, ebx, and edi are altered

dirpush:

;; The pathname is copied to the end of the directory queue. The
;; directory queue is then expanded by calling brk to ensure
;; sufficient space for the next call.

		mov	edi, [DATAOFF(dirqueuetop)]
.copypathloop:	lodsb
		stosb
		or	al, al
		jnz	.copypathloop
		mov	[DATAOFF(dirqueuetop)], edi
		xor	ebx, ebx
		lea	eax, [byte ebx + 45]
		mov	bh, pathbufsize >> 8
		add	ebx, edi
		int	0x80
quit1:		ret


;; lsfile produces output for a single file.
;;
;; input:
;; pathbuf = file's complete pathname
;; sts = file's stat information
;; esi = filename (minus the path)
;; edi = length of filename
;;
;; output:
;; all registers (except ebp) are altered

lsfile:

;; edi is initialized to point to outbuf, which is where the output
;; will be built up. The start of outbuf and the filename's length are
;; stored on the stack.

		mov	eax, outbuf
		push	eax
		push	edi
		xchg	eax, edi
		cdq

;; The inode number is displayed if -i was specified.

		cmp	dh, [DATAOFF(showinod)]
		jz	.noinode
		mov	eax, [DATAOFF(sts.st_ino)]
		mov	dl, 8
		call	ebp
		mov	al, ' '
		stosb
.noinode:

;; The size in blocks is displayed if -s was specified.

		cmp	dh, [DATAOFF(showblks)]
		jz	.noblocks
		mov	eax, [DATAOFF(sts.st_blocks)]
		shr	eax, 1
		mov	dl, 4
		call	ebp
		mov	al, ' '
		stosb
.noblocks:

;; A whole bunch of code is skipped over unless -l was specified.

		cmp	dh, [DATAOFF(longdisp)]
		jz	.briefrelay

;; The first section of long output displays the mode value as a
;; string of diverse characters. The mode is separated into the nine
;; basic permission bits (dl plus the carry flag), the three upper
;; permission bits (dh), and the top four bits which indicate the file
;; type (al). The latter is, after being saved on the stack,
;; translated directly into a lowercase letter and added to outbuf.
;; ecx is initialized to a pattern which is used to indicate which of
;; the nine basic permission bits are execute permissions, as well as
;; when all nine bits have been examined.

		mov	edx, [DATAOFF(sts.st_mode)]
		mov	al, dh
		shr	al, 4
		push	eax
		lea	ebx, [DATAOFF(filetypes)]
		xlatb
		stosb
		lea	ebx, [DATAOFF(permissions)]
		mov	ecx, 444q
		rcr	dh, 1

;; This loop executes once for each basic permission bit, moving each
;; bit into the carry flag in turn. al is set to 4, 2, or 1 if the bit
;; is set, or 7 if it is turned off. If the bit is an execute
;; permission, then the corresponding upper permission bit is also
;; examined. If it is set, then an "x" must be changed to an "s" or
;; "t", and a "-" to an "S" or "T".

.permloop:	mov	al, 7
		jnc	.permoff
		and	al, cl
.permoff:	test	cl, 1
		jz	.nosbit
		shl	dh, 1
		test	dh, 8
		jz	.nosbit
		add	al, cl
		and	al, 0x1F
		inc	eax
.nosbit:	xlatb
		stosb
		shr	ecx, 1
		jz	.permloopend
		rcl	dl, 1				; carry = next bit
		jmp	short .permloop

.briefrelay:	jmp	short .brief

.permloopend:

;; The number of hard links, the owner's user id, and the owner's
;; group id are each in turn added to outbuf.

		lea	ebx, [byte ecx + 3]
		lea	edx, [byte ecx + 4]
		push	esi
		lea	esi, [DATAOFF(sts.st_nlink)]
.numoutloop:	lodsw
		call	ebp
		mov	dl, 7
		dec	ebx
		jnz	.numoutloop
		pop	esi

;; The file type is retrieved from the stack, and the values
;; indicating a character or block device are checked for. If present,
;; the file's rdev numbers are added to outbuf; otherwise, the file's
;; size is used.

		pop	eax
		and	al, ~4
		cmp	al, 2
		mov	eax, [DATAOFF(sts.st_size)]
		mov	dl, 10
		jnz	.nordev
		mov	dl, 5
		movzx	eax, byte [DATAOFF(sts.st_rdev + 1)]
		call	ebp
		mov	al, [DATAOFF(sts.st_rdev)]
.nordev:	call	ebp

;; The programs gets the current time and subtracts the file's
;; modification time from it. This value is then subsequently divided
;; by 60, 60, and 24, stopping if the quotient ever drops to zero. The
;; remainder (or quotient, if it is non-zero after all three
;; divisions) is added to outbuf as the file's modification age,
;; followed by the appropriate letter to indicate the units. (The xlatb
;; at the end works because al is always zero.)

		mov	al, 78
		mov	ebx, edi
		xor	ecx, ecx
		int	0x80
		mov	eax, [edi]
		sub	eax, [DATAOFF(sts.st_mtime)]
		mov	ebx, ebp
.timingloop:	cdq
		dec	ebx
		mov	cl, [byte ebx + timeunits - itoa]
		or	ecx, ecx
		jz	.showtime
		div	ecx
		or	eax, eax
		jnz	.timingloop
		xchg	eax, edx
.showtime:	mov	dl, 5
		call	ebp
		xlatb
		stosb
		mov	al, ' '
		stosb

;; End of long output.

.brief:

;; The filename's length is retrieved, and the filename is copied into
;; outbuf.

		pop	ecx
		call	copyfilename

;; The mode bits are checked to see if the file is a symbolic link.

		mov	al, [DATAOFF(sts.st_mode) + 1]
		and	al, 0xF0
		sub	al, 0xA0
		jnz	.nosymlink
		cmp	al, [DATAOFF(longdisp)]
		jz	.nosymlink

;; If it is, and the output is verbose, then the link destination is
;; appended to outbuf, prefixed by an arrow. (The top of the directory
;; queue is used as a convenient temporary buffer.) Then the original
;; filename is given to stat, as opposed to lstat, in order to obtain
;; the file type of the link destination.

		mov	eax, ' -> '
		stosd
		lea	eax, [byte ecx + 85]
		lea	ebx, [DATAOFF(pathbuf)]
		mov	ecx, [DATAOFF(dirqueuetop)]
		cdq
		mov	dh, pathbufsize >> 8
		int	0x80
		xchg	eax, ecx
		xchg	eax, esi
		mov	[esi + ecx], dl
		call	copyfilename
		xchg	eax, ecx
		mov	al, 106
		lea	ecx, [DATAOFF(sts)]
		int	0x80
		or	eax, eax
		js	.endentry
.nosymlink:

;; The file's type is separated out from the other mode bits. If -F
;; was specified, and the file type has an associated suffix
;; character, this character is added to outbuf immediately following
;; the filename. If the file type has no suffix, but is executable, an
;; asterisk is used.

		mov	eax, [DATAOFF(sts.st_mode) - 2]
		shr	eax, 29
		cmp	ah, [DATAOFF(showtype)]
		jz	.endentry
		mov	al, [DATAOFF(suffixes) + eax]
		cmp	al, '*'
		jnz	.fmtchar
		test	byte [DATAOFF(sts.st_mode)], 111q
		jz	.endentry
.fmtchar:	stosb

;; The entry is complete but for the whitespace. If -1 was specified,
;; then a final newline is all that is needed.

.endentry:	mov	al, 10
		xor	ebx, ebx
		inc	ebx
		cmp	bh, [DATAOFF(oneperln)]
		jnz	.endline

;; Otherwise, the tabstop corresponding to the current cursor position
;; is retrieved from xline and is added to the length of the text in
;; outbuf (converted to tabstops). If it extends past the tenth
;; tabstop, a newline is output to stdout immediately. If it extends
;; past the eighth tabstop, a newline is added to outbuf. If the
;; tabstop is an odd number, two tabs are added to outbuf. Otherwise,
;; one tabstop is added to outbuf. In all cases, xline is updated to
;; reflect the position of the cursor after output.

		mov	ecx, edi
		sub	ecx, [esp]
		shr	ecx, 3
		inc	ecx
		mov	dl, [DATAOFF(xline)]
		add	dl, cl
		cmp	al, dl
		jae	.fitsinline
		push	ecx
		call	tobol
		pop	edx
.fitsinline:	mov	al, 9
		cmp	al, dl
		ja	.roomformore
		inc	eax
		mov	dl, 0
.roomformore:	test	dl, bl
		jz	.setxline
		stosb
		inc	edx
.setxline:	mov	[DATAOFF(xline)], dl
.endline:

;; The contents of outbuf are written to stdout, and lsfile returns.

		stosb
		mov	[DATAOFF(following)], al
		lea	eax, [byte ebx + 3]
		pop	ecx
		mov	edx, edi
		sub	edx, ecx
		int	0x80
		ret


;; errmsg displays a simple error message on stderr.
;;
;; input:
;; eax = error number (positive value, to be returned upon exit)
;; esi = filename to use as error message
;; edi = length of filename
;;
;; output:
;; retval = error number
;; carry flag is set (for the benefit of dolstat)
;; eax, ebx, ecx, and edx are altered

errmsg:

		mov	[DATAOFF(retval)], eax
		call	tobol
		mov	dword [edi + esi], ' ??' | (10 << 24)
		lea	eax, [byte ebx + 3]
		inc	ebx
		mov	ecx, esi
		lea	edx, [byte edi + 4]
		int	0x80
		stc
		ret


;; dolstat executues the lstat system call, and displays an error
;; message if the call was not successful.
;;
;; input:
;; ebx = pathname of file
;;
;; output:
;; sts = stat information for the file
;; eax = lstat return value
;; ecx = zero if the file is a directory
;; carry flag is set if lstat returned an error
;; zero flag is set if the file is a directory

dolstat:	push	byte 107
		pop	eax
		lea	ecx, [DATAOFF(sts)]
		int	0x80
		neg	eax
		jc	errmsg
		mov	ecx, [DATAOFF(sts.st_mode) - 2]
		shr	ecx, 28
		and	ecx, byte ~4
		ret


;; Here is the epilog code for lsdir. When the directory contains
;; nothing more, the program closes the file handle, and then examines
;; the last return value from getdents. If it is non-zero, errmsg is
;; called; otherwise the program falls through to tobol.

getdentsend:	push	eax
		push	byte 6
		pop	eax
		int	0x80
		pop	eax
badfile:	neg	eax
		jc	errmsg


;; tobol forces the output to the start of a line.
;;
;; input:
;; xline = current tabstop position of cursor
;;
;; output:
;; xline = zero
;; eax, ebx, ecx, and edx are altered

tobol:

		xor	ebx, ebx
		inc	ebx
		cmp	bh, [DATAOFF(xline)]
		jz	.quit
		mov	[DATAOFF(xline)], bh
		lea	eax, [byte ebx + 3]
		lea	ecx, [DATAOFF(newline)]
		mov	edx, ebx
		int	0x80
.quit:		ret



;; lsdir is the continuation of ls in the case when the supplied
;; filename is a directory needing to be expanded.

lsdir:

;; A read-only file handle for the directory is obtained and saved on
;; the stack.

		mov	al, 5
		int	0x80
		or	eax, eax
		js	badfile
		push	eax

;; If the directory headings flag is on, then the pathname is
;; displayed on stdout followed by a colon. If this is not the first
;; time the program has written to stdout, then a preceding newline is
;; added.

		lea	ebx, [byte ecx + 1]
		mov	eax, [DATAOFF(dirheads)]
		or	al, al
		jz	.nodirhead
		mov	[DATAOFF(following)], bl
		mov	ecx, esi
		lea	edx, [byte edi + 2]
		or	ah, ah
		mov	ah, 10
		mov	[esi + edi], eax
		jz	.firstout
		dec	ecx
		inc	edx
		mov	byte [ecx], 10
.firstout:	lea	eax, [byte ebx + 3]
		int	0x80
.nodirhead:

;; A slash is added to the pathname, and the file handle is stored in
;; ebx.

		mov	byte [esi + edi], '/'
		inc	edi
		pop	ebx

;; The program obtains the directory's contents in the outer loop,
;; using the getdents system call. The loop is exited when getdents
;; returns a non-positive value.

.getdentsloop:	xor	eax, eax
		mov	al, 141
		mov	ecx, direntsbuf
		cdq
		mov	dh, direntsbufsize >> 8
		int	0x80
		or	eax, eax
		jle	getdentsend

;; The program examines each directory entry in the inner loop, during
;; which the current state is saved on the stack. (ebx contains the
;; file handle, ecx contains the current pointer into the dirents
;; buffer, eax contains the size of the unread dirents, esi points to
;; pathbuf, and edi contains the length of the pathname.)

.direntloop:	pusha

;; edi is set to the position after the final slash in the pathname,
;; and esi is set to the filename of the current directory entry. Then
;; the filename is appended to the directory, and the filename's
;; length (plus one) is stashed in edx.

		cdq
		add	edi, esi
		mov	ebx, esi
		lea	esi, [byte ecx + dirent.d_name]
		push	esi
.appendloop:	lodsb
		stosb
		inc	edx
		or	al, al
		jnz	.appendloop
		pop	esi

;; If the filename begins with a dot and -a was not specified, this
;; file is passed over. Likewise if the -B option was specified and
;; the filename ends in a tilde.

		cmp	byte [esi], '.'
		jnz	.visible
		cmp	al, [DATAOFF(showdots)]
		jz	.resume
.visible:	cmp	byte [byte edi - 2], '~'
		jnz	.notabackup
		cmp	al, [DATAOFF(hidebkup)]
		jnz	.resume
.notabackup:

;; lstat is called on the complete path. If it is also a directory,
;; and if -R was specified, and if the entry is not named "." or "..",
;; then the path is added to the directory queue.

		lea	edi, [byte edx - 1]
		call	dolstat
		jc	.resume
		jnz	.dofile
		cmp	al, [DATAOFF(recursed)]
		jz	.dofile
		push	esi
		lodsb
		cmp	al, '.'
		jnz	.dirokay
		lodsb
		cmp	al, '.'
		jnz	.notdotdot
		lodsb
.notdotdot:	or	al, al
		jz	.dirloops
.dirokay:	mov	esi, ebx
		push	edi
		call	dirpush
		pop	edi
.dirloops:	pop	esi

;; lsfile displays the entry.

.dofile:	call	lsfile

;; State is restored, and ecx is advanced to the next entry. If there
;; are no entries left in the buffer, the program returns to the outer
;; loop in order to get the next batch of entries.

.resume:
		popa
		movzx	edx, word [byte ecx + dirent.d_reclen]
		add	ecx, edx
		sub	eax, edx
		jbe	.getdentsloop
		jmp	short .direntloop


;; copyfilename copies a string, translating non-graphic characters
;; according to the flags rawchars and octchars.
;;
;; input:
;; esi = source string
;; edi = destination buffer
;; ecx = length of string
;;
;; output:
;; esi = end of source string
;; edi = end of copied string
;; ecx = zero
;; eax and edx are altered

copyfilename:

;; dl is non-zero if -N was specified, and dh is non-zero if -b was
;; specified.

		mov	edx, [DATAOFF(rawchars)]

;; The program retrieves the next character in the filename. If -N was
;; specified, the character is copied as-is.

.charloop:	lodsb
		or	dl, dl
		jnz	.graphic

;; The backslash and double-quote characters are checked for
;; separately, since they need to be escaped when -b is specified. All
;; other characters between 32 and 126 inclusive are copied as-is.

		mov	ah, '\'
		cmp	al, ah
		jz	.backslash
		cmp	al, '"'
		jz	.backslash
		cmp	al, '~'
		ja	.octal
		cmp	al, ' '
		jae	.graphic

;; If -b was not specified, non-graphic characters are replaced with a
;; question mark. Otherwise, non-graphic characters are replaced by a
;; backslash followed by three octal digits. In the case of a
;; backslash or a double-quote, a backslash is inserted before the
;; actual character.

.octal:		or	dh, dh
		jz	.usehook
		ror	eax, 8
		stosb
		mov	al, '0' >> 2
		rol	eax, 2
		stosb
		shr	eax, 26
		aam	8
		add	ax, '00'
.backslash:	or	dh, dh
		jz	.graphic
		mov	[edi], ah
		inc	edi
		jmp	short .graphic
.usehook:	mov	al, '?'
.graphic:	stosb

;; The program loops until all characters have been copied.

		dec	ecx
		jnz	.charloop
quit2:		ret


;; lsrecurse calls ls repeatedly for each directory listed in
;; the queue, recursing through any subdirectories depth-first.
;;
;; input:
;; esi = where in the queue to begin
;; dirqueuetop = where in the queue to end
;;
;; output:
;; edx, esi = dirqueuetop
;; all other registers (except ebp) are altered

lsrecurse:

;; The program ensures that directory listings always begin and end on
;; their own lines, and then sets edx to the current end of the queue
;; (which is initially set to the start of the queue).

		call	tobol
		mov	edx, dirqueue

dirqueuetop	equ	$ - 4

;; The program begins at the requested position in the queue, and
;; calls ls for each element. The program calls lsrecurse again
;; directly, using the original end of the queue as the starting
;; point, so that if the call to ls caused more items to be added to
;; the queue, they will be displayed before the next item in the
;; original queue. This recursion ensures a depth-first traversal of
;; subdirectories (and causes the queue to act more like a stack of
;; queues). After the recursive call has bottomed out and returns
;; here, the end of the queue is reset. This prevents the new items
;; from being displayed again later, and also keeps the queue from
;; continually growing in size.

.dirloop:	cmp	esi, edx
		jnc	quit2
		push	edx
		xor	edx, edx
		call	ls
		mov	esi, [byte esp + 4]
		call	lsrecurse
		pop	esi
		pop	edx
		mov	[DATAOFF(dirqueuetop)], edx
		jmp	short .dirloop


;; The read-only data.

suffixes:	db	'|*/**@='			; file suffix array

		db	0, 24, 60, 60			; size of time units
timeunits:

permissions	equ	$ - 1
filetypes	equ	$ + 3
options		equ	$ + 12
optioncount	equ	13

		db	'xwtrpc-dTbs-1lFsSiRbNBadhms'
; permission bits:	 xwtr  - T s     S
; file type chars:	     pc d b - l s
; cmdline options:	             1lFs iRbNBad
; time unit chars:	                        dhms


_ebp_pos_	equ	origin + ($ - $$)

;; itoa converts a number to its ASCII decimal representation.
;;
;; input:
;; eax = number to display
;; edi = buffer to write to
;; edx = number of bytes to write (padding with spaces on the left)
;; 
;; output:
;; eax = zero
;; edi = the position immediately following the string
;; ecx is altered

itoa:

;; edx is preserved on the stack, for the caller's convenience, and
;; the complete buffer is filled with spaces.

		push	edx
		push	eax
		mov	ecx, edx
		mov	al, ' '
		rep stosb
		pop	eax

;; The position after the number is preserved on the stack, and then
;; the program moves backward through the buffer, dividing the number
;; by ten and replacing a space with the remainder changed to an ASCII
;; digit on each iteration through the loop.

newline		equ	$ + 2

		push	edi
		mov	cl, 10
.decloop:	cdq
		div	ecx
		add	dl, '0'
		dec	edi
		mov	[edi], dl
		or	eax, eax
		jnz	.decloop

;; edi and edx are restored and the program returns to the caller.

		pop	edi
		pop	edx
		ret


filesize	equ	$ - $$


;; The zero-initialized data.

ABSOLUTE origin + filesize

ALIGNB 16

;; buffer containing the stat structure

sts:
.st_dev:	resw	1
		resw	1
.st_ino:	resd	1				; inode number
.st_mode:	resw	1				; mode bits
.st_nlink:	resw	1				; number of hard links
.st_uid:	resw	1				; owner user id
.st_gid:	resw	1				; owner group id
.st_rdev:	resw	1				; rdev numbers
		resw	1
.st_size:	resd	1				; file size in bytes
.st_blksize:	resd	1
.st_blocks:	resd	1				; file size in blocks
.st_atime:	resd	1
		resd	1
.st_mtime:	resd	1				; modification time
		resd	1
.st_ctime:	resd	1
		resd	3

;; The program's return value buffer.

retval:		resd	1

;; The flags set by the command-line options.

optionflags:
		resb	1				; (invalid option)
hidedirs:	resb	1				; -d
showdots:	resb	1				; -a
hidebkup:	resb	1				; -B
rawchars:	resb	1				; -N
octchars:	resb	1				; -b
recursed:	resb	1				; -R
showinod:	resb	1				; -i
		resb	1				; (-S, invalid option)
showblks:	resb	1				; -s
showtype:	resb	1				; -F
longdisp:	resb	1				; -l
oneperln:	resb	1				; -1

;; The directory headings flag.

dirheads:	resb	1

;; following is set to non-zero after writing to stdout.

following:	resb	1

;; The current cursor position, as measured in tabstops.

xline:		resb	1

;; The buffer for holding the current pathname (with an extra byte
;; prepended for holding the occasional newline character).

		resd	1
pathbuf:	resb	pathbufsize + 16

;; The output buffer. (It needs to be large enough to hold two
;; complete pathnames, both of which could be quadrupled in size due
;; to substituting octal escape sequences for non-graphic characters.)

outbuf:		resb	pathbufsize * 8 + 64

;; The buffer used to hold the data returned from getdents.

direntsbuf:	resb	direntsbufsize

;; The initial size of the directory queue. This item needs to be last
;; of all, since it is expanded by calling brk.

dirqueue:	resb	pathbufsize


memsize		equ	$ - origin
