DEFINES=
CC=gcc
LD=gcc
CFLAGS=-g -std=c99 -W -Wall $(DEFINES) `pkg-config --cflags openssl`
LDFLAGS=`pkg-config --libs openssl`
BINS=fwdecrypt fwcrypt

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

fwdecrypt: fwdecrypt.o samsung.o
	$(LD) -o $@ $^ $(LDFLAGS)

fwcrypt: fwcrypt.o samsung.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o $(BINS)
