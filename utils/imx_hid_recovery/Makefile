all: imx_hid_recovery

imx_hid_recovery: imx_hid_recovery.c
	gcc -Wall -O2 -std=c99 `pkg-config --libs --cflags libusb-1.0` -o $@ $<
	
clean:
	rm -rf imx_hid_recovery


