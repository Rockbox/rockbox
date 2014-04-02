TOOLS_DIR=../../../tools
CFLAGS=-g -std=c99 -W -Wall
BINS=librbscsi.a
CFLAGS+=-Wall -std=c99 -g -O2
ARFLAGS=rcs

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

librbscsi.a: rbscsi.o
	$(AR) $(ARFLAGS) -o $@ $^

clean:
	rm -fr *.o

veryclean:
	rm -rf $(BINS)
