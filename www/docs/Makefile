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

battery-faq.raw: $(DOCS)/BATTERY-FAQ $(TXT2PLAIN)
	$(TXT2PLAIN) < $< > $@

battery-faq.html: battery-faq.t battery-faq.raw $(TXT2PLAIN)
	$(ACTION) $<

contributing.raw: $(DOCS)/CONTRIBUTING $(TXT2PLAIN)
	$(TXT2PLAIN) < $< > $@

contributing.html: contributing.t contributing.raw $(TXT2PLAIN)
	$(ACTION) $<

%.html : %.t
	$(ACTION) $<
