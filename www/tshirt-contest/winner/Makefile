ACTION=@echo preprocessing $@; rm -f $@; $(HOME)/bin/fcpp -WWW -I../.. -Uunix -H -C -V -LL >$@

SRC := $(wildcard *.t)
OBJS := $(SRC:%.t=%.html)

all: $(OBJS)

%.html : %.t
	$(ACTION) $<
