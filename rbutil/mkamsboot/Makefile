# We use the UCL code available in the Rockbox tools/ directory
CFLAGS=-I../../tools/ucl/include -Wall
LIBUCL=../../tools/ucl/src/libucl.a
CC = gcc

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


ifdef RBARCH
CFLAGS += -arch $(RBARCH)
endif

OUT = $(TARGET_DIR)build$(RBARCH)

all: $(OUTPUT)

$(LIBUCL):
	make -C ../../tools/ucl/src $(TARGET_DIR)libucl.a

# This file can be generated in the dualboot/ directory
$(OUT)/dualboot.o: dualboot.c
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -c -o $(OUT)/dualboot.o dualboot.c

$(OUT)/md5.o: md5.c md5.h
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -c -o $(OUT)/md5.o -W -Wall md5.c

$(OUT)/mkamsboot.o: mkamsboot.c dualboot.h md5.h
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -c -o $(OUT)/mkamsboot.o -W -Wall mkamsboot.c -DVERSION=\"$(APPVERSION)\"

$(OUTPUT):  $(OUT) $(OUT)/mkamsboot.o $(OUT)/md5.o $(OUT)/dualboot.o $(LIBUCL)
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -o $(OUTPUT) $(OUT)/mkamsboot.o $(OUT)/md5.o $(OUT)/dualboot.o $(LIBUCL)

$(OUT)/libmkamsboot.o: mkamsboot.c dualboot.h md5.h
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -DLIB -c -o $(OUT)/libmkamsboot.o -W -Wall mkamsboot.c

libmkamsboot$(RBARCH).a: $(OUT) $(OUT)/libmkamsboot.o $(OUT)/md5.o $(OUT)/dualboot.o
	@echo AR $@
	$(SILENT)$(AR) ru $(TARGET_DIR)libmkamsboot$(RBARCH).a $(OUT)/libmkamsboot.o $(OUT)/md5.o $(OUT)/dualboot.o

# some trickery to build ppc and i386 from a single call
ifeq ($(RBARCH),)
libmkamsbooti386.a:
	make RBARCH=i386 TARGET_DIR=$(TARGET_DIR) libmkamsbooti386.a

libmkamsbootppc.a:
	make RBARCH=ppc TARGET_DIR=$(TARGET_DIR) libmkamsbootppc.a
endif    
    
libmkamsboot-universal: libmkamsbooti386.a libmkamsbootppc.a
	@echo lipo $(TARGET_DIR)libmkamsboot.a
	$(SILENT) rm -f $(TARGET_DIR)libmkamsboot.a
	lipo -create $(TARGET_DIR)libmkamsbootppc.a $(TARGET_DIR)libmkamsbooti386.a -output $(TARGET_DIR)libmkamsboot.a
    
clean:
	rm -f $(OUTPUT) libmkamsboot.o $(TARGET_DIR)libmkamsboot*.a
	rm -rf build*
    
$(OUT):
	@echo MKDIR $(OUT)
	$(SILENT)mkdir $(OUT)

