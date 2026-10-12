CC=gcc
CXX=g++
LD=g++
SBTOOLS_DIR=../imxtools/sbtools
TOMCRYPT_DIR=../tomcrypt
CFLAGS=-Wall -O3 -std=c99 -g -I$(SBTOOLS_DIR) -I$(TOMCRYPT_DIR)/src/headers `pkg-config --cflags lua5.2`
LDFLAGS=`pkg-config --libs lua5.2` -L$(REGTOOLS_LIB_DIR) `xml2-config --libs` -lreadline
EXEC=hwpatcher
SBTOOLS_SRC=elf.c crypto.c sb.c sb1.c crc.c misc.c xorcrypt.c
SBTOOLS_OBJ=$(SBTOOLS_SRC:.c=.o)
SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

all: $(EXEC)

%.o: $(SBTOOLS_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(TOMCRYPT_DIR)/librbtomcrypt.a:
	$(MAKE) -C $(TOMCRYPT_DIR)

hwpatcher: hwpatcher.o md5.o $(SBTOOLS_OBJ) $(TOMCRYPT_DIR)/librbtomcrypt.a
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(SBTOOLS_OBJ) $(OBJ) $(EXEC)

