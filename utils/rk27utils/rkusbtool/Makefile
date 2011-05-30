all: rkusbtool

rkusbtool: rkusbtool.c
	gcc -g -std=c99 -o $@ -W -Wall -lusb-1.0 -I/usr/include/libusb-1.0/ $^

clean:
	rm -fr rkusbtool
