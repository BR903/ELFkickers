#  Makefile for elftoc

CC = gcc
CFLAGS = -ggdb -Wall -W
LDFLAGS = $(CFLAGS)

elftoc: pieces.o addr.o shdrtab.o outbasic.o outtools.o out.o elftoc.o

pieces.o: pieces.c pieces.h gen.h
addr.o: addr.c addr.h gen.h
shdrtab.o: shdrtab.c shdrtab.h gen.h outbasic.h
outbasic.o: outbasic.c outbasic.h gen.h elftoc.h
outtools.o: outtools.c outtools.h gen.h elftoc.h pieces.h addr.h	\
            outbasic.h
out.o: out.c out.h gen.h elftoc.h addr.h pieces.h shdrtab.h outbasic.h	\
       outtools.h names.h
elftoc.o: elftoc.c elftoc.h gen.h pieces.h addr.h shdrtab.h out.h

clean:
	rm -f pieces.o addr.o shdrtab.o outbasic.o outtools.o out.o elftoc.o
	rm -f elftoc
