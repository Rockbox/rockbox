DEFINES=
CC=gcc
LD=gcc
CFLAGS=-g -std=c99 -W -Wall $(DEFINES)
LDFLAGS=
BINS=fwcrypt fwdecrypt

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

fwdecrypt: fwdecrypt.o common.o ../../tools/fwpatcher/md5.o
	$(LD) -o $@ $^ $(LDFLAGS)

fwcrypt: fwcrypt.o common.o ../../tools/fwpatcher/md5.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o $(BINS)
