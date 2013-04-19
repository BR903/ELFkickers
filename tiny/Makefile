all: true false hello fgconsole bf hexdump cat factor

% : %.asm
	nasm -f bin -w+orphan-labels -o $@ $< && chmod +x $@

true: true.asm
hello: hello.asm
fgconsole: fgconsole.asm
bf: bf.asm
hexdump: hexdump.asm
cat: cat.asm
factor: factor.asm

false: true.asm true
	ln -s true false

clean:
