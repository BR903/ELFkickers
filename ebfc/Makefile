#  Makefile for ebfc

CC = gcc
CFLAGS = -Wall -Wextra -Wno-missing-field-initializers

# The main program.

ebfc: ebfc.o brainfuck.o libelfparts.a

ebfc.o: ebfc.c elfparts.h ebfc.h
brainfuck.o: brainfuck.c elfparts.h ebfc.h

# The elfparts library.

libelfparts.a: elfparts.o \
	       ehdr.o phdrtab.o shdrtab.o progbits.o \
               strtab.o symtab.o hash.o rel.o got.o dynamic.o
	ar crs libelfparts.a $^

elfparts.o: elfparts.c elfparts.h elfpartsi.h
ehdr.o: ehdr.c elfparts.h elfpartsi.h
phdrtab.o: phdrtab.c elfparts.h elfpartsi.h
shdrtab.o: shdrtab.c elfparts.h elfpartsi.h
progbits.o: progbits.c elfparts.h elfpartsi.h
strtab.o: strtab.c elfparts.h elfpartsi.h
symtab.o: symtab.c elfparts.h elfpartsi.h
hash.o: hash.c elfparts.h elfpartsi.h
rel.o: rel.c elfparts.h elfpartsi.h
got.o: got.c elfparts.h elfpartsi.h
dynamic.o: dynamic.c elfparts.h elfpartsi.h

clean:
	rm -f ebfc.o brainfuck.o
	rm -f elfparts.o elfpseq.o
	rm -f ehdr.o phdrtab.o shdrtab.o progbits.o strtab.o
	rm -f symtab.o hash.o rel.o got.o dynamic.o
	rm -f ebfc libelfparts.a
