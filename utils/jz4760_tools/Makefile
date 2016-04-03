DEFINES=
CC?=gcc
CXX?=g++
LD?=g++
CFLAGS=-g -std=c99 -Wall $(DEFINES) `pkg-config --cflags libusb-1.0`
CXXFLAGS=-g -Wall $(DEFINES)
LDFLAGS=`pkg-config --libs libusb-1.0`
SRC=$(wildcard *.c)
SRCXX=$(wildcard *.cpp)
EXEC=$(SRC:.c=) $(SRCXX:.cpp=)

all: $(EXEC)

%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

%: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -fr $(EXEC)
