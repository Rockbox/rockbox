CC=gcc
CXX=g++
LD=g++
SBTOOLS_DIR=../imxtools/sbtools
CFLAGS=-Wall -O3 -std=c99 -g -I$(SBTOOLS_DIR) `pkg-config --cflags lua5.2`
LDFLAGS=`pkg-config --libs lua5.2` -L$(REGTOOLS_LIB_DIR) `xml2-config --libs` -lreadline
EXEC=hwpatcher
SBTOOLS_SRC=elf.c crypto.c sb.c sb1.c aes128.c crc.c misc.c sha1.c xorcrypt.c
SBTOOLS_OBJ=$(SBTOOLS_SRC:.c=.o)
SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

all: $(EXEC)

%.o: $(SBTOOLS_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

hwpatcher: hwpatcher.o md5.o $(SBTOOLS_OBJ)
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(SBTOOLS_OBJ) $(OBJ) $(EXEC)


