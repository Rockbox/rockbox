
.PHONY: all clean cable_detect bmp2raw image_conversion
PROGS = extract_section bmp2raw image_conversion cable_detect
CC = gcc
CFLAGS = -O1 -g -W -Wall
R1_SAFEMODE_DIR = files/r1/etc/safemode/
R0_SAFEMODE_DIR = files/r0/etc/safemode/

all: $(PROGS)
	$(MAKE) -C cramfs-1.1

cable_detect:
	arm-ypr0-linux-gnueabi-gcc $(CFLAGS) -o $(R1_SAFEMODE_DIR)cable_detect cable_detect.c

bmp2raw:
	$(CC) $(CFLAGS) -o bmp2raw bmp2raw.c

image_conversion:
	echo "asdasd"
	./convert_images.sh

clean:
	$(MAKE) -C cramfs-1.1 clean
	rm -f $(R0_SAFEMODE_DIR)*.raw
	rm -f $(R1_SAFEMODE_DIR)*.raw
	rm -f $(R1_SAFEMODE_DIR)cable_detect
	rm -f bmp2raw
	rm -f extract_section
