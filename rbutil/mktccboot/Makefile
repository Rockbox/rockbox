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

# inputs
LIBSOURCES := mktccboot.c $(TOOLSDIR)/telechips.o
SOURCES := $(LIBSOURCES) main.c
OBJS := $(patsubst %.c,%.o,$(addprefix $(OUT)/,$(notdir $(SOURCES))))
LIBOBJS := $(patsubst %.c,%.o,$(addprefix $(OUT)/,$(notdir $(LIBSOURCES))))
EXTRADEPS :=

# rule for sources from tools dir
$(OUT)/%.o: $(TOOLSDIR)/%.c $(OUT)
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -c -o $@ $<

$(OUT)/%.o: %.c $(OUT)
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -c -o $@ $<

# building the library archive
$(OUT)/libmktccboot.o: $(OUT)/mktccboot.o
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -DLIB -c -o $(OUT)/libmktccboot.o -W -Wall mktccboot.c

libmktccboot$(RBARCH).a: $(LIBOBJS)
	@echo AR $@
	$(SILENT)$(AR) ruc $(TARGET_DIR)$@ $^

# building the standalone executable
$(OUTPUT): $(OBJS) $(EXTRADEPS)
	@echo LD $@
	$(SILENT)$(CC) $(CFLAGS) -o$(OUTPUT) $(OBJS) $(EXTRADEPS)

# some trickery to build ppc and i386 from a single call
ifeq ($(RBARCH),)
$(TARGET_DIR)libmktccbooti386.a:
	make RBARCH=i386 TARGET_DIR=$(TARGET_DIR) libmktccbooti386.a

$(TARGET_DIR)libmktccbootppc.a:
	make RBARCH=ppc TARGET_DIR=$(TARGET_DIR) libmktccbootppc.a
endif    
    
libmktccboot-universal: $(TARGET_DIR)libmktccbooti386.a $(TARGET_DIR)libmktccbootppc.a
	@echo lipo $(TARGET_DIR)libmktccboot.a
	$(SILENT) rm -f $(TARGET_DIR)libmktccboot.a
	lipo -create $(TARGET_DIR)libmktccbootppc.a $(TARGET_DIR)libmktccbooti386.a -output $(TARGET_DIR)libmktccboot.a
    
clean:
	rm -f $(OUTPUT) $(TARGET_DIR)libmktccboot*.a mktccboot.dmg
	rm -rf $(OUT)

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

