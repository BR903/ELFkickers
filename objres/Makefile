#  Makefile for objres

CC = gcc
CFLAGS = -Wall -Wextra -I../elfrw

objres: objres.c ../elfrw/libelfrw.a

../elfrw/libelfrw.a:
	$(MAKE) -C ../elfrw libelfrw.a

clean:
	rm -f objres
