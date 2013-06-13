CC=gcc
AR=ar
CFLAGS=-W -Wall -O2 `pkg-config --cflags libusb-1.0` -std=c99 -g -fPIC
LDFLAGS=`pkg-config --libs libusb-1.0` -fPIC
LIB=libhwstub.a
SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

all: $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

clean:
	rm -rf $(OBJ) $(LIB)


