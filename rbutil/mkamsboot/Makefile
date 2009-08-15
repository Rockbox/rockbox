# We use the UCL code available in the Rockbox tools/ directory
CFLAGS=-I../../tools/ucl/include -Wall
LIBUCL=../../tools/ucl/src/libucl.a

#change for releases
APPVERSION=`../../tools/version.sh`

ifndef V
SILENT = @
endif

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

ifdef RBARCH
CFLAGS += -arch $(RBARCH)
endif

OUT = build$(RBARCH)

all: $(OUTPUT)

$(LIBUCL):
	make -C ../../tools/ucl/src libucl.a

# This file can be generated in the dualboot/ directory
$(OUT)/dualboot.o: dualboot.c
	$(CC) $(CFLAGS) -c -o $(OUT)/dualboot.o dualboot.c

$(OUT)/md5.o: md5.c md5.h
	$(CC) $(CFLAGS) -c -o $(OUT)/md5.o -W -Wall md5.c

$(OUT)/mkamsboot.o: mkamsboot.c dualboot.h md5.h
	$(CC) $(CFLAGS) -c -o $(OUT)/mkamsboot.o -W -Wall mkamsboot.c -DVERSION=\"$(APPVERSION)\"

$(OUTPUT):  $(OUT) $(OUT)/mkamsboot.o $(OUT)/md5.o $(OUT)/dualboot.o $(LIBUCL)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(OUT)/mkamsboot.o $(OUT)/md5.o $(OUT)/dualboot.o $(LIBUCL)

$(OUT)/libmkamsboot.o: mkamsboot.c dualboot.h md5.h
	$(CC) $(CFLAGS) -DLIB -c -o $(OUT)/libmkamsboot.o -W -Wall mkamsboot.c

libmkamsboot$(RBARCH).a: $(OUT) $(OUT)/libmkamsboot.o $(OUT)/md5.o $(OUT)/dualboot.o
	$(AR) ru libmkamsboot$(RBARCH).a $(OUT)/libmkamsboot.o $(OUT)/md5.o $(OUT)/dualboot.o

# some trickery to build ppc and i386 from a single call
ifeq ($(RBARCH),)
libmkamsbooti386.a:
	make RBARCH=i386 libmkamsbooti386.a

libmkamsbootppc.a:
	make RBARCH=ppc libmkamsbootppc.a
endif    
    
libmkamsboot-universal: libmkamsbooti386.a libmkamsbootppc.a
	@echo lipo libmkamsboot.a
	$(SILENT) rm -f libmkamsboot.a
	lipo -create libmkamsbootppc.a libmkamsbooti386.a -output libmkamsboot.a    
    
clean:
	rm -f $(OUTPUT) libmkamsboot.o libmkamsboot*.a
	rm -rf build*
    
build$(RBARCH):
	@echo MKDIR build$(RBARCH)
	$(SILENT)mkdir build$(RBARCH)

