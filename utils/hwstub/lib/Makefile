AR=ar
INCLUDE=../include
CXXFLAGS=-W -Wall -O2 `pkg-config --cflags libusb-1.0` -std=c++11 -g -fPIC -D_XOPEN_SOURCE=700 -I$(INCLUDE)
LDFLAGS=`pkg-config --libs libusb-1.0` -fPIC -lpthread
LIB=libhwstub.a
SRC=$(wildcard *.cpp)
OBJ=$(SRC:.cpp=.o)

all: $(LIB)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

clean:
	rm -rf $(OBJ) $(LIB)
