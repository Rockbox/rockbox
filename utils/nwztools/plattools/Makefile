PREFIX?=arm-sony-linux-gnueabi-
CC=$(PREFIX)gcc
LD=$(PREFIX)gcc
CFLAGS=-std=gnu99 -Wall -O2
INCLUDES=-I.

LIB_FILES=nwz_lib.c
ALL_BUT_LIB=$(patsubst %.c,%.elf,$(filter-out $(LIB_FILES),$(wildcard *.c)))

all: $(ALL_BUT_LIB)

%.elf: %.c $(LIB_FILES)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

clean:
	rm -rf $(ALL_BUT_LIB)

