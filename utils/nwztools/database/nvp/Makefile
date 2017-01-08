DEFINES=
CC=gcc
CXX=g++
LD=g++
PROFILE=
CFLAGS=-g $(PROFILE) -std=c99 -Wall $(DEFINES) `xml2-config --cflags`
CXXFLAGS=-g $(PROFILE) -Wall $(DEFINES) `xml2-config --cflags`
LDFLAGS=$(PROFILE) `xml2-config --libs`
BINS=nvptool

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

nvptool: nvptool.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o

veryclean:
	rm -rf $(BINS)
