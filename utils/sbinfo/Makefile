TGT = sbinfo

all: $(TGT)

$(TGT): sbinfo.c crc.c crypto.h aes128.c sha1.c elf.c
	$(CC) -g -std=c99 -o $(TGT) -W -Wall sbinfo.c aes128.c crc.c sha1.c elf.c

clean:
	rm -fr $(TGT)
