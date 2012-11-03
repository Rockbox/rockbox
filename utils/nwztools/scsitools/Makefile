DEFINES=
CC=gcc
LD=gcc
CFLAGS=-g -std=c99 -W -Wall $(DEFINES)
LDFLAGS=-lsgutils2
BINS=scsitool

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

scsitool: scsitool.o misc.o para_noise.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o

veryclean:
	rm -rf $(BINS)
