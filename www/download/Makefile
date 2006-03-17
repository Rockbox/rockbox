ACTION=echo preprocessing $@; rm -f $@; \
       $(HOME)/bin/fcpp -WWW -I.. -Uunix -H -C -V -LL $< $@

OBJS := index.shtml old.html

all: $(OBJS)

index.shtml: index.t
	$(ACTION)

old.html: old.t
	$(ACTION)
