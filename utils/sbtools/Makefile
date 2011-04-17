all: elftosb sbtoelf

sbtoelf: sbtoelf.c crc.c crypto.h aes128.c sha1.c elf.c sb.h
	gcc -g -std=c99 -o $@ -W -Wall $^

elftosb: elftosb.c crc.c crypto.h aes128.c sha1.c elf.c sb.h
	gcc -g -std=c99 -o $@ -W -Wall $^

clean:
	rm -fr elftosb sbtoelf
