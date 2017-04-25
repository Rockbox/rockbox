DEFINES=
CC=gcc
CXX=g++
LD=g++
PROFILE=
CFLAGS=-g $(PROFILE) -std=c99 -Wall $(DEFINES)
CXXFLAGS=-g $(PROFILE) -Wall $(DEFINES)
LDFLAGS=$(PROFILE)
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
