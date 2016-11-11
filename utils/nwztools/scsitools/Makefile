DEFINES=
CC=gcc
LD=gcc
NWZ_DB_DIR=../database
INCLUDES=-I$(NWZ_DB_DIR)
CFLAGS=-std=c99 -W -Wall $(DEFINES) $(INCLUDES)
LDFLAGS=-lsgutils2
BINS=scsitool

all: $(BINS)

scsitool: scsitool.c misc.c para_noise.c $(NWZ_DB_DIR)/nwz_db.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(BINS)
