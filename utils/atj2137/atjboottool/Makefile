DEFINES=
CC=gcc
LD=gcc
CFLAGS=-g -std=c99 -W -Wall $(DEFINES)
LDFLAGS=
BINS=atjboottool

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

atjboottool: atjboottool.o fwu.o fw.o afi.o misc.o atj_tables.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o

veryclean:
	rm -rf $(BINS)
