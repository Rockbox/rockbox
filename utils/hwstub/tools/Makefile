CC=gcc
AR=ar
HWEMUL_LIB_DIR=../lib
CFLAGS=-W -Wall -O2 `pkg-config --cflags libusb-1.0` -std=c99 -g -I$(HWEMUL_LIB_DIR)
LDFLAGS=`pkg-config --libs libusb-1.0` -lreadline
EXEC=hwemul_tool
HWEMUL_LIB=$(HWEMUL_LIB_DIR)/libhwemul.a
SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

all: $(EXEC)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

hwemul_tool: hwemul_tool.o $(HWEMUL_LIB)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(OBJ) $(LIB)


