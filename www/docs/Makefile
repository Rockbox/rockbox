ACTION=@echo preprocessing $@; rm -f $@; $(HOME)/bin/fcpp -WWW -I.. -Uunix -H -C -V -P -LL >$@


SRC := $(wildcard *.t)
OBJS := $(SRC:%.t=%.html)

FAQ2HTML = ../faq2html.pl
TXT2HTML = ../txt2html.pl

# This is correct.  It does point to ../../docs but we need a different
# name so that the directories don't collide.
DOCS = ../docs_

all: $(OBJS)

faq.raw: $(DOCS)/FAQ $(FAQ2HTML)
	$(FAQ2HTML) < $< > $@

faq.html: faq.t faq.raw $(FAQ2HTML)
	$(ACTION) $<

nodo.raw: $(DOCS)/NODO $(FAQ2HTML)
	$(FAQ2HTML) < $< > $@

nodo.html: nodo.t nodo.raw $(FAQ2HTML)
	$(ACTION) $<

battery-faq.raw: $(DOCS)/BATTERY-FAQ $(FAQ2HTML)
	$(FAQ2HTML) < $< > $@

battery-faq.html: battery-faq.t battery-faq.raw $(FAQ2HTML)
	$(ACTION) $<

contributing.raw: $(DOCS)/CONTRIBUTING $(TXT2HTML)
	$(TXT2HTML) < $< > $@

contributing.html: contributing.t contributing.raw $(TXT2HTML)
	$(ACTION) $<

custom_wps_format.raw: $(DOCS)/CUSTOM_WPS_FORMAT $(TXT2HTML)
	$(TXT2HTML) < $< > $@

custom_wps_format.html: custom_wps_format.t custom_wps_format.raw $(TXT2HTML)
	$(ACTION) $<

custom_cfg_format.raw: $(DOCS)/CUSTOM_CFG_FORMAT $(TXT2HTML)
	$(TXT2HTML) < $< > $@

custom_cfg_format.html: custom_cfg_format.t custom_cfg_format.raw $(TXT2HTML)
	$(ACTION) $<

how_to_compile.raw: $(DOCS)/README $(TXT2HTML)
	$(TXT2HTML) < $< > $@

how_to_compile.html: how_to_compile.t how_to_compile.raw $(TXT2HTML)
	$(ACTION) $<

credits.raw: $(DOCS)/CREDITS $(TXT2HTML)
	$(TXT2HTML) < $< > $@

credits.html: credits.t credits.raw $(TXT2HTML)
	$(ACTION) $<

%.html : %.t
	$(ACTION) $<
