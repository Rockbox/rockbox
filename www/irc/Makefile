ACTION=@echo preprocessing $@; rm -f $@; fcpp -WWW -I.. -Uunix -H -C -V -LL >$@

SRC := $(wildcard *.t)
OBJS := $(SRC:%.t=%.html)

all: $(OBJS)

%.html : %.t
	$(ACTION) $<

index.html: index.t nicks.txt
