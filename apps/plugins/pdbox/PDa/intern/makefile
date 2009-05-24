

SOURCE = $(shell ls *.c)
TARGETS = $(SOURCE:.c=.pd_linux)

EXT= pd_linux

CFLAGS += -O2  -I../src -DFIXEDPOINT
EFLAGS = -shared -Wl,-export-dynamic



all: $(TARGETS)

clean:
	-rm $(TARGETS)
	-rm *.o *~

install:
	install -d $(DESTDIR)/$(PREFIX)/lib/pd/extra
	cp $(TARGETS) $(DESTDIR)/$(PREFIX)/lib/pd/extra

%.$(EXT) : %.o
	$(CC) -o $@ $(EFLAGS)  $+

