ACTION=@echo preprocessing $@; rm -f $@; $(HOME)/bin/fcpp -WWW -I.. -Uunix -H -C -V -LL >$@

SRC := $(wildcard *.t)
OBJS := $(SRC:%.t=%.shtml)

FSRC := $(wildcard *.bdf)
FOBJS := $(FSRC:%.bdf=%.fnt) $(FSRC:%.bdf=%.png)

all: bdf2bmp $(OBJS) $(FOBJS)

%.shtml : %.t
	$(ACTION) $<

%.fnt : %.bdf
	../tools/convbdf -s 32 -l 255 -f "$<"

%.png : %.bdf
	./bdf2bmp "$<" "$<.bmp"
	convert "$<.bmp" "$@"
	rm $<.bmp

convbdf: convbdf.c
	$(CC) -o "$@" "$<"

clean:
	rm $(OBJS) $(FOBJS)