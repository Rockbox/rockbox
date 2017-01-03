TOOLS_DIR=../../scsi
DEFINES+=
PREFIX?=
CC=$(PREFIX)gcc
LD=$(PREFIX)gcc
CFLAGS=-g -std=c99 -Wall $(DEFINES) -I$(TOOLS_DIR)
LDFLAGS=-L$(TOOLS_DIR) -lrbscsi
BINS=scsitool

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

scsitool: scsitool.o misc.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o

veryclean:
	rm -rf $(BINS)
