ACTION=@echo preprocessing $@; rm -f $@; $(HOME)/bin/fcpp -WWW -I.. -Uunix -H -C -V -P -LL >$@


SRC := $(wildcard *.t)
OBJS := $(SRC:%.t=%.html)
TXT2PLAIN = ../txt2plain.pl

# This is correct.  It does point to ../../docs but we need a different
# name so that the directories don't collide.
DOCS = ../docs_

all: $(OBJS)

faq.html: faq.t faq.raw $(TXT2PLAIN)
	$(ACTION) $<

faq.raw: $(DOCS)/FAQ $(TXT2PLAIN)
	$(TXT2PLAIN) < $< > $@

battery.raw: $(DOCS)/BATTERY $(TXT2PLAIN)
	$(TXT2PLAIN) < $< > $@

battery.html: battery.t battery.raw $(TXT2PLAIN)
	$(ACTION) $<

%.html : %.t
	$(ACTION) $<
