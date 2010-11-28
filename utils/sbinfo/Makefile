TGT = sbinfo

all: $(TGT)

$(TGT): sbinfo.c
	$(CC) -std=c99 -o $(TGT) -W -Wall sbinfo.c aes128_impl.c

clean:
	rm -fr $(TGT)
