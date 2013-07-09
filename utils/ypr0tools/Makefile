DEFINES=
CC=gcc
LD=gcc
CFLAGS=-g -std=c99 -W -Wall $(DEFINES) `pkg-config --cflags openssl`
LDFLAGS=`pkg-config --libs openssl`
BINS=fwcrypt fwdecrypt

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

fwdecrypt: fwdecrypt.o common.o
	$(LD) -o $@ $^ $(LDFLAGS)

fwcrypt: fwcrypt.o common.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o $(BINS)
