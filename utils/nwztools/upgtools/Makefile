DEFINES=
CC=gcc
CXX=g++
LD=g++
PROFILE=
PKGCONFIG := $(CROSS)pkg-config

# Distros could use different names for the crypto library. We try a list
# of candidate names, only one of them should be the valid one.
LIBCRYPTO_NAMES = libcryptopp libcrypto++ cryptopp crypto++

$(foreach l,$(LIBCRYPTO_NAMES),\
	$(eval LDOPTS += $(shell $(PKGCONFIG) --silence-errors --libs $(l))))
$(foreach l,$(LIBCRYPTO_NAMES),\
	$(eval CFLAGS += $(shell $(PKGCONFIG) --silence-errors --cflags $(l))))
$(foreach l,$(LIBCRYPTO_NAMES),\
	$(eval CXXFLAGS += $(shell $(PKGCONFIG) --silence-errors --cflags $(l))))

CXXFLAGS=-g $(PROFILE) -Wall $(DEFINES)
CFLAGS=-g $(PROFILE) -Wall -std=c99 $(DEFINES)
LDFLAGS=$(PROFILE) $(LDOPTS) -lpthread

BINS=upgtool

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

upgtool: upgtool.o upg.o misc.o fwp.o mg.o keysig_search.o md5.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o

veryclean:
	rm -rf $(BINS)
