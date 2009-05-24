
VERSION = 0.2
SOURCE = $(shell ls *.c)
TARGETS = $(SOURCE:.c=.pd_linux)

EXT= pd_linux

AFLAGS = -g -O2  -I./  -DFIXEDPOINT
EFLAGS = -shared -Wl,-export-dynamic
PREFIX = /usr


all: $(TARGETS)

clean:
	-rm $(TARGETS)
	-rm *.o *~

tar: clean
	cd ..;tar czvf PDa-externals-$(VERSION).tgz PDa-externals

upload: tar
	scp ../PDa-externals-$(VERSION).tgz gige@xdv.org:~/www/pda/release

install:
	install -d $(DESTDIR)/$(PREFIX)/lib/pd/extra
	cp $(TARGETS) $(DESTDIR)/$(PREFIX)/lib/pd/extra

%.$(EXT) : %.o
	$(CC) -o $@ $(EFLAGS)  $+

%.o : %.c
	$(CC) -c $(AFLAGS) $(CFLAGS) $+

