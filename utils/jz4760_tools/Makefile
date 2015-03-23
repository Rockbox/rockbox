DEFINES=
CC?=gcc
CXX?=g++
LD?=g++
CFLAGS=-g -std=c99 -Wall $(DEFINES) -Ilib
CXXFLAGS=-g -Wall $(DEFINES)
LDFLAGS=
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
