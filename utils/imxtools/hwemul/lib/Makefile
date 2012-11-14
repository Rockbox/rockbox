CC=gcc
AR=ar
CFLAGS=-W -Wall -O2 `pkg-config --cflags libusb-1.0` -std=c99 -g -fPIC
LDFLAGS=`pkg-config --libs libusb-1.0` -fPIC
LIB=libhwemul.a
REGTOOLS=../../regtools
DESC=$(REGTOOLS)/desc
HWEMULGEN=$(REGTOOLS)/hwemulgen
HWEMULSOC_PREFIX=hwemul_soc
SRC=$(wildcard *.c) $(HWEMULSOC_PREFIX).c
OBJ=$(SRC:.c=.o)

all: $(LIB) $(EXEC)

$(HWEMULSOC_PREFIX).c $(HWEMULSOC_PREFIX).h:
	$(HWEMULGEN) $(DESC)/*.xml $(HWEMULSOC_PREFIX)

%.o: %.c $(HWEMULSOC_PREFIX).h
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

clean:
	rm -rf $(OBJ) $(LIB) $(HWEMULSOC_PREFIX).c $(HWEMULSOC_PREFIX).h


