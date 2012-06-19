DEFINES=
CC=gcc
LD=gcc
CFLAGS=-g -std=c99 -W -Wall $(DEFINES)
LDFLAGS=
BINS=rkboottool

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

rkboottool: rkboottool.o misc.o elf.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o

veryclean:
	rm -rf $(BINS)
