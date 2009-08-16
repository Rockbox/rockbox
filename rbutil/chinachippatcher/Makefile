CFLAGS=-g -Wall -DSTANDALONE
CC=gcc

all: chinachip

chinachip: chinachip.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f chinachip
