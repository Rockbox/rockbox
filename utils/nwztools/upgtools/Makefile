DEFINES=
CC=gcc
CXX=g++
LD=g++
PROFILE=
CFLAGS=-g $(PROFILE) -std=c99 -W -Wall $(DEFINES) `pkg-config --cflags openssl` `pkg-config --cflags libcrypto++`
CXXFLAGS=-g $(PROFILE) -W -Wall $(DEFINES) `pkg-config --cflags openssl` `pkg-config --cflags libcrypto++`
LDFLAGS=$(PROFILE) `pkg-config --libs openssl` `pkg-config --libs libcrypto++` -lcrypt
BINS=upgtool

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

upgtool: upgtool.o misc.o fwp.o mg.o keysig_search.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o

veryclean:
	rm -rf $(BINS)
