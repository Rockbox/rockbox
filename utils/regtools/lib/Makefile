CC?=gcc
CXX?=g++
AR?=ar
CFLAGS=-Wall -O2 `xml2-config --cflags` -std=c99 -g -fPIC
CXXFLAGS=-Wall -O2 `xml2-config --cflags` -g -fPIC
LIB=libsocdesc.a
SRC=$(wildcard *.c)
SRCXX=$(wildcard *.cpp)
OBJ=$(SRC:.c=.o) $(SRCXX:.cpp=.o)

all: $(LIB) $(EXEC)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

clean:
	rm -rf $(OBJ) $(LIB)
