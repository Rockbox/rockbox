#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

#change for releases
TARGET_DIR ?= $(shell pwd)/
CFLAGS=-Wall -g

ifndef V
SILENT = @
endif

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
OUTPUT=chinachip.exe
CFLAGS+=-mno-cygwin
else
ifeq ($(findstring MINGW,$(shell uname)),MINGW)
OUTPUT=chinachip.exe
else
ifeq ($(findstring mingw,$(CC)),mingw)
OUTPUT=chinachip.exe
else
OUTPUT=chinachip
endif
endif
endif

ifdef RBARCH
CFLAGS += -arch $(RBARCH)
OBJDIR = $(TARGET_DIR)build/$(RBARCH)/
else
OBJDIR = $(TARGET_DIR)build/
endif


all: $(OUTPUT)

# inputs
LIBSOURCES := chinachip.c
SOURCES := $(LIBSOURCES) main.c
OBJS := $(patsubst %.c,%.o,$(addprefix $(OBJDIR),$(SOURCES)))
LIBOBJS := $(patsubst %.c,%.o,$(addprefix $(OBJDIR),$(LIBSOURCES)))
EXTRADEPS :=

$(OBJDIR)%.o: %.c
	@echo CC $<
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(CC) $(CFLAGS) -c -o $@ $<

libchinachip$(RBARCH).a: $(TARGET_DIR)libchinachip$(RBARCH).a

$(TARGET_DIR)libchinachip$(RBARCH).a: $(LIBOBJS)
	@echo AR $(notdir $@)
	$(SILENT)$(AR) rucs $@ $^

# building the standalone executable
$(OUTPUT): $(OBJS) $(EXTRADEPS)
	@echo LD $@
	$(SILENT)$(CC) $(CFLAGS) -o $(OUTPUT) $(OBJS) $(EXTRADEPS)

# some trickery to build ppc and i386 from a single call
ifeq ($(RBARCH),)
$(TARGET_DIR)libchinachipi386.a:
	make RBARCH=i386 TARGET_DIR=$(TARGET_DIR) libchinachipi386.a

$(TARGET_DIR)libchinachipppc.a:
	make RBARCH=ppc TARGET_DIR=$(TARGET_DIR) libchinachipppc.a
endif

libchinachip-universal: $(TARGET_DIR)libchinachipi386.a $(TARGET_DIR)libchinachipppc.a
	@echo lipo $(TARGET_DIR)libchinachip.a
	$(SILENT) rm -f $(TARGET_DIR)libchinachip.a
	$(SILENT)lipo -create $(TARGET_DIR)libchinachipppc.a $(TARGET_DIR)libchinachipi386.a -output $(TARGET_DIR)libchinachip.a

clean:
	rm -f $(OBJS) $(OUTPUT) libchinachip.o $(TARGET_DIR)libchinachip*.a chinachip.dmg
	rm -rf chinachip-* i386 ppc $(OBJDIR)

chinachip-i386:
	$(MAKE) RBARCH=i386
	mv chinachip chinachip-i386

chinachip-ppc:
	$(MAKE) RBARCH=ppc
	mv chinachip chinachip-ppc

chinachip-mac: chinachip-i386 chinachip-ppc
	$(SILENT)lipo -create chinachip-ppc chinachip-i386 -output chinachip-mac

chinachip.dmg: chinachip-mac
	mkdir -p chinachip-dmg
	cp -p chinachip-mac chinachip-dmg
	hdiutil create -srcfolder chinachip-dmg chinachip.dmg
