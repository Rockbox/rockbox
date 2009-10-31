# We use the UCL code available in the Rockbox tools/ directory
CFLAGS=-I../../tools/ucl/include -Wall
CC = gcc

#change for releases
ifndef APPVERSION
APPVERSION=`../../tools/version.sh`
endif

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

# Dependant modules
LIBUCL=../../tools/ucl/src/libucl$(RBARCH).a

$(LIBUCL):
	make -C ../../tools/ucl/src $(TARGET_DIR)libucl$(RBARCH).a

# This file can be generated in the dualboot/ directory
$(OUT)/dualboot.o: dualboot.[ch]
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -c -o $(OUT)/dualboot.o dualboot.c

$(OUT)/md5.o: md5.[ch]
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -c -o $(OUT)/md5.o -W -Wall md5.c

DEPENDANT_OBJS=$(LIBUCL) $(OUT)/dualboot.o $(OUT)/md5.o

$(OUT)/mkamsboot.o: mkamsboot.[ch] $(DEPENDANT_OBJS)
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -c -o $(OUT)/mkamsboot.o -W -Wall mkamsboot.c -DVERSION=\"$(APPVERSION)\"

$(OUTPUT):  $(OUT) $(OUT)/mkamsboot.o
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -o $(OUTPUT) $(OUT)/mkamsboot.o $(DEPENDANT_OBJS)

$(OUT)/libmkamsboot.o: $(OUT)/mkamsboot.o
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -DLIB -c -o $(OUT)/libmkamsboot.o -W -Wall mkamsboot.c

libmkamsboot$(RBARCH).a: $(OUT) $(OUT)/libmkamsboot.o
	@echo AR $@
	$(SILENT)$(AR) ruc $(TARGET_DIR)libmkamsboot$(RBARCH).a $(OUT)/libmkamsboot.o

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
	rm -f $(OUTPUT) libmkamsboot.o $(TARGET_DIR)libmkamsboot*.a mkamsboot.dmg $(DEPENDANT_OBJS)
	rm -rf build* mkamsboot-*

mkamsboot-i386:
	$(MAKE) RBARCH=i386
	mv mkamsboot mkamsboot-i386

mkamsboot-ppc:
	make RBARCH=ppc
	mv mkamsboot mkamsboot-ppc

mkamsboot-mac: mkamsboot-i386 mkamsboot-ppc
	lipo -create mkamsboot-ppc mkamsboot-i386 -output mkamsboot-mac

mkamsboot.dmg: mkamsboot-mac
	mkdir -p mkamsboot-dmg
	cp -p mkamsboot-mac mkamsboot-dmg
	hdiutil create -srcfolder mkamsboot-dmg mkamsboot.dmg
    
$(OUT):
	@echo MKDIR $(OUT)
	$(SILENT)mkdir $(OUT)

