ACTION=@echo preprocessing $@; rm -f $@; $(HOME)/bin/fcpp -WWW -Uunix -H -C -V -LL >$@

SRC := $(wildcard *.t)
OBJS := $(SRC:%.t=%.html)

.SUFFIXES: .t .html

%.html : %.t
	$(ACTION) $<

all: $(OBJS)
	@(cd schematics; $(MAKE))
	@(cd docs; $(MAKE))
	@(cd mods; $(MAKE))
	@(cd internals; $(MAKE))
	@(cd irc; $(MAKE))

main.html: main.t activity.html

