# We use the UCL code available in the Rockbox tools/ directory
CFLAGS=-I../../tools/ucl/include -Wall
LIBUCL=../../tools/ucl/src/libucl.a

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
OUTPUT=mkamsboot.exe
CFLAGS+=-mno-cygwin
else
ifeq ($(findstring MINGW,$(shell uname)),MINGW)
OUTPUT=mkamsboot.exe
else
ifeq ($(findstring mingw,$(CC)),mingw)
OUTPUT=mkamsboot.exe
else
OUTPUT=mkamsboot
endif
endif
endif

CC?= gcc

all: $(OUTPUT)

$(LIBUCL):
	make -C ../../tools/ucl/src libucl.a

# This file can be generated in the dualboot/ directory
dualboot.o: dualboot.c
	$(CC) $(CFLAGS) -c -o dualboot.o dualboot.c

md5.o: md5.c md5.h
	$(CC) $(CFLAGS) -c -o md5.o -W -Wall md5.c

mkamsboot.o: mkamsboot.c dualboot.h md5.h
	$(CC) $(CFLAGS) -c -o mkamsboot.o -W -Wall mkamsboot.c

$(OUTPUT): mkamsboot.o md5.o dualboot.o $(LIBUCL)
	$(CC) $(CFLAGS) -o $(OUTPUT) mkamsboot.o md5.o dualboot.o $(LIBUCL)

libmkamsboot.o: mkamsboot.c dualboot.h md5.h
	$(CC) $(CFLAGS) -DLIB -c -o libmkamsboot.o -W -Wall mkamsboot.c

libmkamsboot.a: libmkamsboot.o md5.o dualboot.o
	$(AR) ruv libmkamsboot.a libmkamsboot.o md5.o dualboot.o

clean:
	rm -f $(OUTPUT) mkamsboot.o *~ md5.o dualboot.o \
	libmkamsboot.o libmkamsboot.a
