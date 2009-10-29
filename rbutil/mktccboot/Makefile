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

PLAT=tcc

CC = gcc
MKPLATBOOT=mk$(PLAT)boot
LIB_MKPLATBOOT=lib$(MKPLATBOOT)

#change for releases
ifndef APPVERSION
APPVERSION=`$(TOOLSDIR)/version.sh`
endif

ifndef V
SILENT = @
endif

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
OUTPUT=$(MKPLATBOOT).exe
CFLAGS+=-mno-cygwin
else
ifeq ($(findstring MINGW,$(shell uname)),MINGW)
OUTPUT=$(MKPLATBOOT).exe
else
ifeq ($(findstring mingw,$(CC)),mingw)
OUTPUT=$(MKPLATBOOT).exe
else
OUTPUT=$(MKPLATBOOT)
endif
endif
endif

ifdef RBARCH
CFLAGS += -arch $(RBARCH)
endif

OUT = $(TARGET_DIR)build$(RBARCH)

MKPLATBOOT_OBJ=$(OUT)/$(MKPLATBOOT).o
LIBMKPLATBOOT_OBJ=$(OUT)/$(LIB_MKPLATBOOT).o

all: $(OUTPUT)

# Dependant modules
TELECHIPS=$(TOOLSDIR)/telechips
TELECHIPS_OBJ=$(TELECHIPS).o

$(TELECHIPS_OBJ): $(TELECHIPS).[ch]
	make -C $(TOOLSDIR) $(TARGET_DIR)telechips.o

DEPENDANT_OBJS=$(TELECHIPS_OBJ)

$(MKPLATBOOT_OBJ): $(MKPLATBOOT).[ch] $(DEPENDANT_OBJS)
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -c -o $(MKPLATBOOT_OBJ) -W -Wall $(MKPLATBOOT).c -DVERSION=\"$(APPVERSION)\"

$(OUTPUT):  $(OUT) $(MKPLATBOOT_OBJ)
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -o $(OUTPUT) $(MKPLATBOOT_OBJ) $(DEPENDANT_OBJS)

$(LIBMKPLATBOOT_OBJ): $(MKPLATBOOT_OBJ)
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -DLIB -c -o $(LIBMKPLATBOOT_OBJ) -W -Wall $(MKPLATBOOT).c

$(LIB_MKPLATBOOT)$(RBARCH).a: $(OUT) $(LIBMKPLATBOOT_OBJ)
	@echo AR $@
	$(SILENT)$(AR) ruc $(TARGET_DIR)$(LIB_MKPLATBOOT)$(RBARCH).a $(LIBMKPLATBOOT_OBJ)

# some trickery to build ppc and i386 from a single call
ifeq ($(RBARCH),)
$(LIB_MKPLATBOOT)i386.a:
	make RBARCH=i386 TARGET_DIR=$(TARGET_DIR) $(LIB_MKPLATBOOT)i386.a

$(LIB_MKPLATBOOT)ppc.a:
	make RBARCH=ppc TARGET_DIR=$(TARGET_DIR) $(LIB_MKPLATBOOT)ppc.a
endif    
    
$(LIB_MKPLATBOOT)-universal: $(LIB_MKPLATBOOT)i386.a $(LIB_MKPLATBOOT)ppc.a
	@echo lipo $(TARGET_DIR)$(LIB_MKPLATBOOT).a
	$(SILENT) rm -f $(TARGET_DIR)$(LIB_MKPLATBOOT).a
	lipo -create $(TARGET_DIR)$(LIB_MKPLATBOOT)ppc.a $(TARGET_DIR)$(LIB_MKPLATBOOT)i386.a -output $(TARGET_DIR)$(LIB_MKPLATBOOT).a
    
clean:
	rm -f $(OUTPUT) $(LIB_MKPLATBOOT).o $(TARGET_DIR)$(LIB_MKPLATBOOT)*.a $(MKPLATBOOT).dmg
	rm -f $(DEPENDANT_OBJS)
	rm -rf build* $(MKPLATBOOT)-*

$(MKPLATBOOT)-i386:
	$(MAKE) RBARCH=i386
	mv $(MKPLATBOOT) $(MKPLATBOOT)-i386

$(MKPLATBOOT)-ppc:
	make RBARCH=ppc
	mv $(MKPLATBOOT) $(MKPLATBOOT)-ppc

$(MKPLATBOOT)-mac: $(MKPLATBOOT)-i386 $(MKPLATBOOT)-ppc
	lipo -create $(MKPLATBOOT)-ppc $(MKPLATBOOT)-i386 -output $(MKPLATBOOT)-mac

$(MKPLATBOOT).dmg: $(MKPLATBOOT)-mac
	mkdir -p $(MKPLATBOOT)-dmg
	cp -p $(MKPLATBOOT)-mac $(MKPLATBOOT)-dmg
	hdiutil create -srcfolder $(MKPLATBOOT)-dmg $(MKPLATBOOT).dmg
    
$(OUT):
	@echo MKDIR $(OUT)
	$(SILENT)mkdir $(OUT)
