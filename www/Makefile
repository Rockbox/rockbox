ACTION=@echo preprocessing $@; rm -f $@; fcpp -WWW -Uunix -H -C -V -LL >$@

SRC := $(wildcard *.t)
OBJS := $(SRC:%.t=%.html)

.SUFFIXES: .t .html

%.html : %.t
	$(ACTION) $<

all: $(OBJS) descramble descramble.static.bz2 sh2d sh2d.static.bz2 \
	scramble scramble.static.bz2
	@(cd schematics; $(MAKE))
	@(cd docs; $(MAKE))
	@(cd mods; $(MAKE))

main.html: main.t activity.html

descramble: descramble.c
	cc -Wall -ansi -O2 -s -o $@ $<
	chmod a+r descramble

descramble.static.bz2: descramble.c
	cc -static -O2 -s -o descramble.static $<
	bzip2 -f descramble.static
	chmod a+r descramble.static.bz2

scramble: scramble.c
	cc -Wall -ansi -O2 -s -o $@ $<
	chmod a+r scramble

scramble.static.bz2: scramble.c
	cc -static -O2 -s -o scramble.static $<
	bzip2 -f scramble.static
	chmod a+r scramble.static.bz2

sh2d: sh2d.c
	cc -O2 -s -o $@ $<
	chmod a+r sh2d

sh2d.static.bz2: sh2d.c
	cc -static -O2 -s -o sh2d.static $<
	bzip2 -f sh2d.static
	chmod a+r sh2d.static.bz2
