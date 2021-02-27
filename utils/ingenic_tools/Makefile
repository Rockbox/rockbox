DEFINES=
CC?=gcc
CFLAGS=-g -std=c99 -Wall $(DEFINES) `pkg-config --cflags libusb-1.0`
LDFLAGS=`pkg-config --libs libusb-1.0`
SRC=$(wildcard *.c)
EXEC=$(SRC:.c=)

all: $(EXEC)

%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -fr $(EXEC)
