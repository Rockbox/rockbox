DEFINES=-DCRYPTO_LIBUSB
CC=gcc
LD=gcc
CFLAGS=-g -std=c99 -W -Wall `pkg-config --cflags libusb-1.0` $(DEFINES)
LDFLAGS=`pkg-config --libs libusb-1.0`

all: elftosb sbtoelf

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

sbtoelf: sbtoelf.o crc.o crypto.o aes128.o sha1.o elf.o misc.o sb.o
	$(LD) $(LDFLAGS) -o $@ $^

elftosb: elftosb.o crc.o crypto.o aes128.o sha1.o elf.o dbparser.o misc.o sb.o
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	rm -fr *.o

veryclean:
	rm -rf sbtoelf elftosb
