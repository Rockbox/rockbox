DEFINES=
CC?=gcc
CXX?=g++
LD?=g++
CFLAGS=-g -std=c99 -Wall $(DEFINES) -Ilib
CXXFLAGS=-g -Wall $(DEFINES) -Ilib
LDFLAGS=-Llib -lsocdesc `xml2-config --libs`
SRC=$(wildcard *.c)
SRCXX=$(wildcard *.cpp)
EXEC=$(SRC:.c=) $(SRCXX:.cpp=)
LIB=lib/libsocdesc.a

all: $(EXEC)

$(LIB):
	make -C lib

%: %.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

%: %.cpp $(LIB)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -fr $(EXEC)
