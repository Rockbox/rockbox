DEFINES=
CC=gcc
CXX=g++
LD=g++
CFLAGS=-g -std=c99 -W -Wall `xml2-config --cflags` $(DEFINES)
CXXFLAGS=-g -W -Wall `xml2-config --cflags` $(DEFINES)
LDFLAGS=`xml2-config --libs`
BINS= tester headergen hwemulgen

all: $(BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

headergen: headergen.o desc_parser.o
	$(LD) -o $@ $^ $(LDFLAGS)

hwemulgen: hwemulgen.o desc_parser.o
	$(LD) -o $@ $^ $(LDFLAGS)

tester: tester.o desc_parser.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -fr *.o $(BINS)
