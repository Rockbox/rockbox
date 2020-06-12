SCSI_DIR=../../scsi
DEFINES=
PREFIX?=
EXE_EXT?=
CC=$(PREFIX)gcc
LD=$(PREFIX)gcc
NWZ_DB_DIR=../../../firmware/target/hosted/sonynwz
INCLUDES=-I$(NWZ_DB_DIR) -I$(SCSI_DIR)
CFLAGS=-std=c99 -g -Wall -O2 $(DEFINES) $(INCLUDES)
LDFLAGS=-L$(SCSI_DIR) -lrbscsi
BINS=scsitool$(EXE_EXT)

all: $(BINS)

scsitool$(EXE_EXT): scsitool.c misc.c para_noise.c $(NWZ_DB_DIR)/nwz-db.c
	$(MAKE) -C $(SCSI_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(BINS)
