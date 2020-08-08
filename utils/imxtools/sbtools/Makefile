DEFINES=
CC := gcc
CXX := g++
LD := g++

TOMCRYPT_DIR := ../../tomcrypt/

CFLAGS := -O3 -g -std=c99 -Wall $(shell pkg-config --cflags libusb-1.0) $(DEFINES) -I$(TOMCRYPT_DIR)/src/headers
CXXFLAGS := -O3 -g -Wall $(DEFINES) -I$(TOMCRYPT_DIR)/src/headers
LDFLAGS := $(shell pkg-config --libs libusb-1.0)
BINS := elftosb sbtoelf sbloader rsrctool elftosb1

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

sbtoelf: sbtoelf.o crc.o crypto.o xorcrypt.o dbparser.o elf.o misc.o sb.o sb1.o $(TOMCRYPT_DIR)/librbtomcrypt.a
	$(LD) -o $@ $^ $(LDFLAGS)

elftosb: elftosb.o crc.o crypto.o elf.o dbparser.o misc.o sb.o $(TOMCRYPT_DIR)/librbtomcrypt.a
	$(LD) -o $@ $^ $(LDFLAGS)

elftosb1: elftosb1.o xorcrypt.o elf.o misc.o sb1.o
	$(LD) -o $@ $^ $(LDFLAGS)

sbloader: sbloader.o
	$(LD) -o $@ $^ $(LDFLAGS)

rsrctool: rsrctool.o rsrc.o misc.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o

veryclean:
	rm -rf $(BINS)

$(TOMCRYPT_DIR)/librbtomcrypt.a:
	$(MAKE) -C $(TOMCRYPT_DIR)

