ACTION=@echo preprocessing $@; rm -f $@; $(HOME)/bin/fcpp -WWW -I.. -Uunix -H -C -V -LL >$@

SRC := $(wildcard *.t)
OBJS := $(SRC:%.t=%.shtml)

all: $(OBJS)

%.html : %.t
	$(ACTION) $<

%.shtml : %.t
	$(ACTION) $<

index.shtml: index.t nicks.txt
