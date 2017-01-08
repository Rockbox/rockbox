DEFINES=
CC=gcc
LD=gcc
CFLAGS=-g -std=c99 -Wall $(DEFINES)
LDFLAGS=
BINS=emmctool

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

emmctool: emmctool.o misc.o nvp.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o

veryclean:
	rm -rf $(BINS)
