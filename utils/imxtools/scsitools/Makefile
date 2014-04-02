TOOLS_DIR=../../scsi
DEFINES+=
CC=gcc
LD=gcc
CFLAGS=-g -std=c99 -W -Wall $(DEFINES) -I$(TOOLS_DIR)
LDFLAGS=-L$(TOOLS_DIR) -lrbscsi
BINS=scsitool

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

scsitool: scsitool.o misc.o stmp_scsi.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o

veryclean:
	rm -rf $(BINS)
