# $Id$

FILTERS = libdemac/filter_16_11.o libdemac/filter_64_11.o libdemac/filter_256_13.o libdemac/filter_32_10.o libdemac/filter_1280_15.o
LIBOBJS = libdemac/parser.o libdemac/decoder.o libdemac/entropy.o libdemac/predictor.o libdemac/crc.o $(FILTERS)
OBJS = demac.o wavwrite.o $(LIBOBJS)

CFLAGS = -Wall -O3 -Ilibdemac

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
EXT = .exe
CROSS = 
CFLAGS += -mno-cygwin
else
  ifdef WIN
     EXT = .exe
     CROSS = i586-mingw32msvc-
  else
     EXT =
     CROSS =
  endif
endif

CC = $(CROSS)gcc
STRIP = $(CROSS)strip
OUTPUT = demac$(EXT)

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	$(CC) -o $(OUTPUT) $(OBJS)

.c.o :
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

libdemac/filter_16_11.o: libdemac/filter.c
libdemac/filter_64_11.o: libdemac/filter.c
libdemac/filter_256_13.o: libdemac/filter.c
libdemac/filter_1280_15.o: libdemac/filter.c
libdemac/filter_32_10.o: libdemac/filter.c

clean:
	rm -f $(OUTPUT) $(OBJS) *~ */*~
