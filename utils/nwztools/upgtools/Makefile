DEFINES=
CC=gcc
CXX=g++
LD=g++
PROFILE=
CFLAGS=-g $(PROFILE) -std=c99 -Wall $(DEFINES)
CXXFLAGS=-g $(PROFILE) -Wall $(DEFINES)
LDFLAGS=$(PROFILE)
BINS=upgtool

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

upgtool: upgtool.o upg.o misc.o fwp.o mg.o keysig_search.o md5.o des.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o

veryclean:
	rm -rf $(BINS)
