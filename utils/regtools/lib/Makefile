CC?=gcc
CXX?=g++
AR?=ar
INCLUDE=../include/
CXXFLAGS=-Wall -O2 `xml2-config --cflags` -g -fPIC -I$(INCLUDE)
LIB=libsocdesc.a
SRC=$(wildcard *.c)
SRCXX=$(wildcard *.cpp)
OBJ=$(SRCXX:.cpp=.o)

all: $(LIB) $(EXEC)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

clean:
	rm -rf $(OBJ) $(LIB)
