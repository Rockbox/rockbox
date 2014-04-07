DEFINES=
CC=gcc
LD=gcc
CFLAGS=-O2 -g -std=c99 -W -Wall `pkg-config --cflags libusb-1.0` $(DEFINES)
LDFLAGS=`pkg-config --libs libusb-1.0`
BINS=e200load

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

e200load: e200load.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o

veryclean:
	rm -rf $(BINS)
 
