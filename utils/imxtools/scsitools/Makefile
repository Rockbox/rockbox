DEFINES=
CC=$(PREFIX)gcc
LD=$(PREFIX)gcc
CFLAGS=-g -std=c99 -W -Wall $(DEFINES)
LDFLAGS=-lsgutils2
BINS=scsitool

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

scsitool: scsitool.o misc.o
	$(MAKE) -C $(TOOLS_DIR)
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o

veryclean:
	rm -rf $(BINS)
