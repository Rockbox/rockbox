<<<<<<< Updated upstream
DEFINES=
CC=gcc
LD=gcc
CFLAGS=-g -std=c99 -W -Wall $(DEFINES)
LDFLAGS=-lsgutils2
=======
TOOLS_DIR=../../scsi
DEFINES+=
PREFIX?=
CC=$(PREFIX)gcc
LD=$(PREFIX)gcc
CFLAGS=-g -std=c99 -Wall $(DEFINES) -I$(TOOLS_DIR)
LDFLAGS=-L$(TOOLS_DIR) -lrbscsi
>>>>>>> Stashed changes
BINS=scsitool

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

scsitool: scsitool.o misc.o
	$(MAKE) -C $(TOOLS_DIR)
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o

veryclean:
	rm -rf $(BINS)
