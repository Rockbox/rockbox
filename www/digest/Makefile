ACTION=@echo preprocessing $@; \
  fcpp -WWW -I.. -Uunix -H -C -V -LL $< $@

SRC := $(wildcard *.t)
OBJS := $(SRC:%.t=%.html)

all: $(OBJS) digest.rss digest.mail

digest.html: digest.t news.t digesthead.t log.t
	$(ACTION)

index.html: index.t ../head.t news.t ../foot.t

%.html : %.t news.t digesthead.t log.t
	$(ACTION)

digest.mail: mail.t log.t mailify.pl
	@echo mailifying $@;
	fcpp -WWW -DMAKE_MAIL -Uunix -P -H -C -V -LL $< digest.temp
	./mailify.pl < digest.temp > digest.mail

digest.rss: digest.t digesthead.t log.raw
	@echo rssing $@;
	@rm -f $@;
	@fcpp -WWW -DMAKE_RSS -Uunix -P -H -C -V -LL >$@ $<

log.raw: log.t rssify.pl digesthead.t
	./rssify.pl < $< >$@



