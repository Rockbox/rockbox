#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# We use the Telechips code available in the Rockbox tools/ directory
TOOLSDIR=../../tools
CFLAGS := -O -g -W -Wall -Wshadow -pedantic -I$(TOOLSDIR)

CC = gcc

# Change for releases
ifndef APPVERSION
APPVERSION=`$(TOOLSDIR)/version.sh`
endif

ifndef V
SILENT = @
endif

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
OUTPUT=mktccboot.exe
CFLAGS+=-mno-cygwin
else
ifeq ($(findstring MINGW,$(shell uname)),MINGW)
OUTPUT=mktccboot.exe
else
ifeq ($(findstring mingw,$(CC)),mingw)
OUTPUT=mktccboot.exe
else
OUTPUT=mktccboot
endif
endif
endif

ifdef RBARCH
CFLAGS += -arch $(RBARCH)
endif

OUT = $(TARGET_DIR)build$(RBARCH)

all: $(OUTPUT)

$(OUT)/telechips.o: $(TOOLSDIR)/telechips.[ch]
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -c -o $(OUT)/telechips.o $(TOOLSDIR)/telechips.c

$(OUT)/mktccboot.o: mktccboot.[ch] $(OUT)/telechips.o
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -c -o $(OUT)/mktccboot.o -W -Wall mktccboot.c -DVERSION=\"$(APPVERSION)\"

$(OUTPUT):  $(OUT) $(OUT)/mktccboot.o
	@echo LD $@
	$(SILENT)$(CC) $(CFLAGS) -o $(OUTPUT) $(OUT)/mktccboot.o $(OUT)/telechips.o

$(OUT)/libmktccboot.o: $(OUT)/mktccboot.o
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -DLIB -c -o $(OUT)/libmktccboot.o -W -Wall mktccboot.c

libmktccboot$(RBARCH).a: $(OUT) $(OUT)/libmktccboot.o
	@echo AR $@
	$(SILENT)$(AR) ruc $(TARGET_DIR)libmktccboot$(RBARCH).a $(OUT)/libmktccboot.o

# some trickery to build ppc and i386 from a single call
ifeq ($(RBARCH),)
libmktccbooti386.a:
	make RBARCH=i386 TARGET_DIR=$(TARGET_DIR) libmktccbooti386.a

libmktccbootppc.a:
	make RBARCH=ppc TARGET_DIR=$(TARGET_DIR) libmktccbootppc.a
endif    
    
libmktccboot-universal: libmktccbooti386.a libmktccbootppc.a
	@echo lipo $(TARGET_DIR)libmktccboot.a
	$(SILENT) rm -f $(TARGET_DIR)libmktccboot.a
	lipo -create $(TARGET_DIR)libmktccbootppc.a $(TARGET_DIR)libmktccbooti386.a -output $(TARGET_DIR)libmktccboot.a
    
clean:
	rm -f $(OUTPUT) libmktccboot.o $(TARGET_DIR)libmktccboot*.a mktccboot.dmg
	rm -f $(TOOLSDIR)/telechips.o
	rm -rf build* mktccboot-*

mktccboot-i386:
	$(MAKE) RBARCH=i386
	mv mktccboot mktccboot-i386

mktccboot-ppc:
	make RBARCH=ppc
	mv mktccboot mktccboot-ppc

mktccboot-mac: mktccboot-i386 mktccboot-ppc
	lipo -create mktccboot-ppc mktccboot-i386 -output mktccboot-mac

mktccboot.dmg: mktccboot-mac
	mkdir -p mktccboot-dmg
	cp -p mktccboot-mac mktccboot-dmg
	hdiutil create -srcfolder mktccboot-dmg mktccboot.dmg
    
$(OUT):
	@echo MKDIR $(OUT)
	$(SILENT)mkdir $(OUT)

