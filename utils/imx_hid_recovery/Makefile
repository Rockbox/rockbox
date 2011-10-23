CC=gcc
LD=gcc

all: imx_hid_recovery

imx_hid_recovery.o: imx_hid_recovery.c
	$(CC) -c -Wall -O2 -std=c99 `pkg-config --cflags libusb-1.0` -o $@ $<

imx_hid_recovery: imx_hid_recovery.o
	$(LD) -o $@ $<  `pkg-config --libs libusb-1.0` 
	
clean:
	rm -rf imx_hid_recovery.o


