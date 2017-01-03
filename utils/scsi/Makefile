PREFIX?=
CC=$(PREFIX)gcc
AR=$(PREFIX)ar
CFLAGS=-g -std=c99 -Wall
BINS=librbscsi.a
ARFLAGS=rcs

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

librbscsi.a: rbscsi.o
	$(AR) $(ARFLAGS) -o $@ $^

clean:
	rm -fr *.o $(BINS)
