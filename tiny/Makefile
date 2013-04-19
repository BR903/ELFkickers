all: true false hello fgconsole keepalive bf hexdump factor ls date snake

% : %.asm
	nasm -f bin -w+orphan-labels -o $@ $< && chmod +x $@

true      : true.asm
hello     : hello.asm
fgconsole : fgconsole.asm
keepalive : keepalive.asm
bf        : bf.asm
hexdump   : hexdump.asm
ls        : ls.asm
factor    : factor.asm
date      : date.asm
snake     : snake.asm

false: true
	ln -s true false

clean:
	rm -f true false hello fgconsole keepalive
	rm -f bf hexdump factor ls date snake
