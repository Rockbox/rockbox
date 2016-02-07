AR=ar
INCLUDE=../include
CFLAGS=-W -Wall -O2 `pkg-config --cflags libusb-1.0` -std=c99 -g -fPIC -D_XOPEN_SOURCE=700 -I$(INCLUDE)
CXXFLAGS=-W -Wall -O2 `pkg-config --cflags libusb-1.0` -std=c++11 -g -fPIC -D_XOPEN_SOURCE=700 -I$(INCLUDE)
LDFLAGS=`pkg-config --libs libusb-1.0` -fPIC -lpthread
LIB=libhwstub.a
SRC=$(wildcard *.c)
SRCXX=$(wildcard *.cpp)
OBJ=$(SRCXX:.cpp=.oxx) $(SRCXX:.cpp=.o)

all: $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.oxx: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

clean:
	rm -rf $(OBJ) $(LIB)


