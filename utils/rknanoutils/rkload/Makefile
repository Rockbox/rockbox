DEFINES=
CC=gcc
LD=gcc
CFLAGS=-O3 -g -std=c99 -W -Wall `pkg-config --cflags libusb-1.0` $(DEFINES)
LDFLAGS=`pkg-config --libs libusb-1.0`
BINS=rkloader

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

sbtoelf: rkloader.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o

veryclean:
	rm -rf $(BINS)
