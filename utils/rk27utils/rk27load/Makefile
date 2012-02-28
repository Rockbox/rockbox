all: rk27load

rk27load: main.c scramble.c checksum.c common.c stage1_upload.c stage2_upload.c stage3_upload.c
	gcc -g -std=c99 -o $@ -W -Wall -I/usr/include/libusb-1.0/ $^ -lusb-1.0

clean:
	rm -fr *.o rk27load
