#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

ifndef V
SILENT = @
endif

CC ?= gcc
AR ?= ar
OUTPUT=libskin_parser
TARGET_DIR ?= ./

# construct build directory if BUILDDIR is not set.
BUILDDIR ?= $(TARGET_DIR)build
TARGETPLATFORM ?= $(shell uname)

ifdef RBARCH
CFLAGS += -arch $(RBARCH)
OBJDIR = $(BUILDDIR)/$(RBARCH)/
else
OBJDIR = $(BUILDDIR)/
endif

# input files
SOURCES := skin_buffer.c skin_debug.c skin_parser.c skin_scan.c tag_table.c
OBJS := $(patsubst %.c,%.o,$(addprefix $(OBJDIR),$(SOURCES)))

all: $(OUTPUT).a

$(OBJDIR)%.o: %.c
	@echo CC $<
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(CC) $(CFLAGS) -c -o $@ $<

ifeq ($(findstring Darwin,$(TARGETPLATFORM)),Darwin)
# some trickery to build ppc and i386 from a single call
$(OUTPUT).a: $(TARGET_DIR)$(OUTPUT)i386.a $(TARGET_DIR)$(OUTPUT)ppc.a
	@echo lipo $(TARGET_DIR)$@
	$(SILENT) rm -f $(TARGET_DIR)$(OUTPUT)
	$(SILENT)lipo -create $(TARGET_DIR)$(OUTPUT)ppc.a $(TARGET_DIR)$(OUTPUT)i386.a -output $(TARGET_DIR)$(OUTPUT).a

ifneq ($(RBARCH),)
$(OUTPUT)$(RBARCH).a: $(OBJS)
	@echo AR $@
	$(SILENT)$(AR) ruc $(TARGET_DIR)$@ $^
else
$(TARGET_DIR)$(OUTPUT)i386.a:
	make RBARCH=i386 TARGET_DIR=$(TARGET_DIR) $(OUTPUT)i386.a

$(TARGET_DIR)$(OUTPUT)ppc.a:
	make RBARCH=ppc TARGET_DIR=$(TARGET_DIR) $(OUTPUT)ppc.a
endif
else
$(OUTPUT).a: $(OBJS)
	@echo AR $@
	$(SILENT)$(AR) ruc $(TARGET_DIR)$@ $^
endif

clean:
	rm -f $(OBJS) $(OUTPUT) $(TARGET_DIR)$(OUTPUT)*.a
ifeq ($(findstring Darwin,$(shell uname)),Darwin)
endif

