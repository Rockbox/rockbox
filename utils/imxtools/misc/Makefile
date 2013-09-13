DEFINES=
CC=gcc
LD=gcc
CFLAGS=-O3 -g -std=c99 -W -Wall $(DEFINES)
LDFLAGS=
BINS=io_pins wiki_gen

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

io_pins: io_pins.o
	$(LD) -o $@ $^ $(LDFLAGS)

wiki_gen: wiki_gen.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o

veryclean:
	rm -rf $(BINS)
