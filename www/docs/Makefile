ACTION=@echo preprocessing $@; rm -f $@; $(HOME)/bin/fcpp -WWW -I.. -Uunix -H -C -V -P -LL >$@

SRC := $(wildcard *.t)
OBJS := $(SRC:%.t=%.html)
TXT2PLAIN = ../txt2plain.pl

all: $(OBJS)

faq.html: faq.t faq.raw $(TXT2PLAIN)
	$(ACTION) $<

faq.raw: FAQ $(TXT2PLAIN)
	$(TXT2PLAIN) < $< > $@


%.html : %.t
	$(ACTION) $<
