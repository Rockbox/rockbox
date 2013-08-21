DEFINES=-DCRYPTO_LIBUSB
CC=gcc
LD=gcc
CFLAGS=-O3 -g -std=c99 -W -Wall `pkg-config --cflags libusb-1.0` $(DEFINES)
LDFLAGS=`pkg-config --libs libusb-1.0`
BINS=elftosb sbtoelf sbloader rsrctool elftosb1

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

sbtoelf: sbtoelf.o crc.o crypto.o aes128.o sha1.o xorcrypt.o dbparser.o elf.o misc.o sb.o sb1.o
	$(LD) -o $@ $^ $(LDFLAGS)

elftosb: elftosb.o crc.o crypto.o aes128.o sha1.o elf.o dbparser.o misc.o sb.o
	$(LD) -o $@ $^ $(LDFLAGS)

elftosb1: elftosb1.o xorcrypt.o elf.o misc.o sb1.o
	$(LD) -o $@ $^ $(LDFLAGS)

sbloader: sbloader.o
	$(LD) -o $@ $^ $(LDFLAGS)

rsrctool: rsrctool.o rsrc.o misc.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o

veryclean:
	rm -rf $(BINS)
