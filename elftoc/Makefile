#  Makefile for elftoc

CC = gcc
CFLAGS = -ggdb -Wall
LDFLAGS = $(CFLAGS)

OBJS = pieces.o addr.o shdrtab.o dynamic.o outbasic.o outtools.o out.o elftoc.o

elftoc: $(OBJS)

pieces.o: pieces.c pieces.h gen.h elf.h
addr.o: addr.c addr.h gen.h
shdrtab.o: shdrtab.c shdrtab.h gen.h elf.h outbasic.h
outbasic.o: outbasic.c outbasic.h gen.h elftoc.h
outtools.o: outtools.c outtools.h gen.h elf.h elftoc.h pieces.h addr.h	\
            outbasic.h
out.o: out.c out.h gen.h elf.h elftoc.h addr.h pieces.h shdrtab.h	\
       outbasic.h outtools.h names.h
elftoc.o: elftoc.c elftoc.h gen.h elf.h pieces.h addr.h shdrtab.h out.h

clean:
	rm -f elftoc $(OBJS)
