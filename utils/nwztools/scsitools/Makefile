SCSI_DIR=../../scsi
DEFINES=
PREFIX?=
CC=$(PREFIX)gcc
LD=$(PREFIX)gcc
NWZ_DB_DIR=../database
INCLUDES=-I$(NWZ_DB_DIR) -I$(SCSI_DIR)
CFLAGS=-std=c99 -Wall $(DEFINES) $(INCLUDES)
LDFLAGS=-L$(SCSI_DIR) -lrbscsi
BINS=scsitool

all: $(BINS)

scsitool: scsitool.c misc.c para_noise.c $(NWZ_DB_DIR)/nwz_db.c
	$(MAKE) -C $(SCSI_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(BINS)
