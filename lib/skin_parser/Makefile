#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

ifndef V
SILENT = @
endif

CC ?= gcc
AR ?= ar
OUTPUT=libskin_parser
TARGET_DIR ?= ./

ifeq ($(OS),Windows_NT)
mkdir = if not exist $(subst /,\,$(1)) mkdir $(subst /,\,$(1))
rm = if exist $(subst /,\,$(1)) del /q /s $(subst /,\,$(1))
else
mkdir = mkdir -p $(1)
rm = rm -rf $(1)
endif

CPPDEFINES := $(shell echo foo | $(CC) -dM -E -)

# construct build directory if BUILDDIR is not set.
BUILDDIR ?= $(TARGET_DIR)build

CFLAGS += -D__PCTOOL__
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
	$(info CC $<)
	$(SILENT)$(call mkdir,$(dir $@))
	$(SILENT)$(CC) $(CFLAGS) -c -o $@ $<

ifeq ($(findstring Darwin,$(TARGETPLATFORM)),Darwin)
# some trickery to build ppc and i386 from a single call
$(OUTPUT).a: $(TARGET_DIR)$(OUTPUT)i386.a $(TARGET_DIR)$(OUTPUT)ppc.a
	$(info lipo $(TARGET_DIR)$@)
	$(SILENT)$(call rm,$(TARGET_DIR)$(OUTPUT))
	$(SILENT)lipo -create $(TARGET_DIR)$(OUTPUT)ppc.a $(TARGET_DIR)$(OUTPUT)i386.a -output $(TARGET_DIR)$(OUTPUT).a

ifneq ($(RBARCH),)
$(OUTPUT)$(RBARCH).a: $(OBJS)
	$(info AR $@)
	$(SILENT)$(AR) ruc $(TARGET_DIR)$@ $^
else
$(TARGET_DIR)$(OUTPUT)i386.a:
	make RBARCH=i386 TARGET_DIR=$(TARGET_DIR) $(OUTPUT)i386.a

$(TARGET_DIR)$(OUTPUT)ppc.a:
	make RBARCH=ppc TARGET_DIR=$(TARGET_DIR) $(OUTPUT)ppc.a
endif
else
$(OUTPUT).a: $(OBJS)
	$(info AR $@)
	$(SILENT)$(AR) ruc $(TARGET_DIR)$@ $^
endif

clean:
	$(call rm,$(OBJS) $(OUTPUT) $(TARGET_DIR)$(OUTPUT)*.a)

