#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
CFLAGS += -I../include

ifndef V
SILENT = @
endif

ifdef RBARCH
CFLAGS += -arch $(RBARCH)
endif

# OS X specifics. Needs to consider cross compiling for Windows.
ifeq ($(findstring Darwin,$(shell uname)),Darwin)
ifneq ($(findstring mingw,$(CROSS)$(CC)),mingw)
# when building libs for OS X build for both i386 and ppc at the same time.
# This creates fat objects, and ar can only create the archive but not operate
# on it. As a result the ar call must NOT use the u (update) flag.
CFLAGS += -arch ppc -arch i386
# building against SDK 10.4 is not compatible with gcc-4.2 (default on newer Xcode)
# might need adjustment for older Xcode.
CC ?= gcc-4.0
CFLAGS += -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
NATIVECC ?= gcc-4.0
endif
endif

TARGET_DIR ?= $(shell pwd)/
OBJDIR = $(TARGET_DIR)build$(RBARCH)

SOURCES = alloc.c io.c n2b_99.c n2b_d.c n2b_ds.c n2b_to.c n2d_99.c	\
	n2d_d.c n2d_ds.c n2d_to.c n2e_99.c n2e_d.c n2e_ds.c n2e_to.c ucl_crc.c	\
	ucl_init.c ucl_ptr.c ucl_str.c ucl_util.c #ucl_dll.c

OBJS =  $(addprefix $(OBJDIR)/,$(SOURCES:%.c=%.o))

# This Makefile is _not_ used to build uclpack for Rockbox builds anymore (they
# use tools.make), so we can use $(CC) and $(AR) here.

libucl$(RBARCH).a: $(TARGET_DIR)libucl$(RBARCH).a

dll: ucl.dll
ucl.dll: $(TARGET_DIR)ucl.dll
$(TARGET_DIR)ucl.dll: $(OBJS)
	@echo DLL $(notdir $@)
	$(SILENT)$(CROSS)$(CC) $(CFLAGS) -shared -o $@ $^ \
		    -Wl,--output-def,$(TARGET_DIR)ucl.def

$(TARGET_DIR)libucl$(RBARCH).a: $(OBJS)
	@echo AR $(notdir $@)
	$(SILENT)rm -f $@
	$(SILENT)$(CROSS)$(AR) rcs $@ $(OBJS) >/dev/null 2>&1

$(OBJDIR)/%.o: %.c
	@echo CC $<
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(CROSS)$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET_DIR)libucl*.a
	rm -rf build*

