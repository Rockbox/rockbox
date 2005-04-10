ACTION=@echo preprocessing $@; rm -f $@; fcpp -WWW -I.. -Uunix -H -C -V -LL >$@

SRC := $(wildcard *.t)
OBJS := $(SRC:%.t=%.html)

all: $(OBJS) digest.rss digest.mail

%.html : %.t news.t digesthead.t log.t
	$(ACTION) $<

digest.mail: mail.t log.t mailify.pl
	echo mailifying $@;
	rm -f $@;
	fcpp -WWW -DMAKE_MAIL -Uunix -P -H -C -V -LL >digest.temp $<
	./mailify.pl < digest.temp > digest.mail

digest.rss: digest.t digesthead.t log.raw
	@echo rssing $@;
	@rm -f $@;
	@fcpp -WWW -DMAKE_RSS -Uunix -P -H -C -V -LL >$@ $<

log.raw: log.t rssify.pl digesthead.t
	./rssify.pl < $< >$@



