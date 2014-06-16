#  The top-level makefile simply invokes all the other makefiles

prefix = /usr/local

PROGRAMS = elfls objres rebind sstrip elftoc ebfc infect

all: $(PROGRAMS)

bin/%:
	mkdir -p bin
	$(MAKE) -C$* $*
	cp $*/$* $@

doc/%.1:
	mkdir -p doc
	cp $*/$*.1 $@

elfls: bin/elfls doc/elfls.1
objres: bin/objres doc/objres.1
rebind: bin/rebind doc/rebind.1
sstrip: bin/sstrip doc/sstrip.1
elftoc: bin/elftoc doc/elftoc.1
ebfc: bin/ebfc doc/ebfc.1
infect: bin/infect doc/infect.1

install: $(PROGRAMS:%=bin/%)
	cp bin/* $(prefix)/bin/.
	cp doc/* $(prefix)/share/man/man1/.

clean:
	for dir in elfrw $(PROGRAMS) ; do $(MAKE) -C$$dir clean ; done
	rm -f $(PROGRAMS:%=bin/%) $(PROGRAMS:%=doc/%.1)
