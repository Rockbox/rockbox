DEFINES=
PREFIX?=
EXE_EXT?=
CC=$(PREFIX)gcc
CXX=$(PREFIX)g++
LD=$(PREFIX)g++
PKGCONFIG:=$(CROSS)pkg-config

TARGET_TRIPLE:=$(shell $(CC) -dumpmachine)
ifneq ($(findstring -w64-,$(TARGET_TRIPLE)),)
	TARGET_OS := Windows
endif

ifeq ($(TARGET_OS), Windows)
# -static allows to statically include libwinpthread-1.dll if possible
CFLAGS+=-gstabs
CXXFLAGS+=-gstabs
LDFLAGS+=-static-libgcc -static
else
# Distros could use different names for the crypto library. We try a list
# of candidate names, only one of them should be the valid one.
LIBCRYPTO_NAMES = libcryptopp libcrypto++ cryptopp crypto++

$(foreach l,$(LIBCRYPTO_NAMES),\
	$(eval LDLIBS += $(shell $(PKGCONFIG) --silence-errors --libs $(l))))
$(foreach l,$(LIBCRYPTO_NAMES),\
	$(eval CFLAGS += $(shell $(PKGCONFIG) --silence-errors --cflags $(l))))
$(foreach l,$(LIBCRYPTO_NAMES),\
	$(eval CXXFLAGS += $(shell $(PKGCONFIG) --silence-errors --cflags $(l))))
endif

CXXFLAGS+=-g -Wall $(DEFINES)
CFLAGS+=-g -Wall -std=c99 $(DEFINES)
LDLIBS+=-lpthread -lz

BINS=upgtool$(EXE_EXT)

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

upgtool$(EXE_EXT): upgtool.o upg.o misc.o mg.o keysig_search.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -rf *.o

veryclean:
	rm -rf $(BINS)
