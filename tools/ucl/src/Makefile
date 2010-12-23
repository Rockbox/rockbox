#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
CFLAGS = -I../include

ifndef V
SILENT = @
endif

ifdef RBARCH
CFLAGS += -arch $(RBARCH)
endif


OUT = $(TARGET_DIR)build$(RBARCH)

SOURCES = alloc.c io.c n2b_99.c n2b_d.c n2b_ds.c n2b_to.c n2d_99.c	\
	n2d_d.c n2d_ds.c n2d_to.c n2e_99.c n2e_d.c n2e_ds.c n2e_to.c ucl_crc.c	\
	ucl_init.c ucl_ptr.c ucl_str.c ucl_util.c #ucl_dll.c

OBJS =  $(addprefix $(OUT)/,$(SOURCES:%.c=%.o))

# This Makefile is _not_ used to build uclpack for Rockbox builds anymore (they
# use tools.make), so we can use $(CC) and $(AR) here.

libucl$(RBARCH).a: $(OUT) $(OBJS)
	@echo AR $@
	$(SILENT)$(AR) rucs $(TARGET_DIR)$@ $(OBJS) >/dev/null 2>&1

$(OUT)/%.o: %.c
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -c $< -o $@
    
# some trickery to build ppc and i386 from a single call
ifeq ($(RBARCH),)
$(TARGET_DIR)libucli386.a:
	make RBARCH=i386 TARGET_DIR=$(TARGET_DIR) libucli386.a

$(TARGET_DIR)libuclppc.a:
	make RBARCH=ppc TARGET_DIR=$(TARGET_DIR) libuclppc.a
endif    
    
libucl-universal: $(TARGET_DIR)libucli386.a $(TARGET_DIR)libuclppc.a
	@echo lipo $(TARGET_DIR)libucl.a
	$(SILENT) rm -f $(TARGET_DIR)libucl.a
	$(SILENT)lipo -create $(TARGET_DIR)libuclppc.a $(TARGET_DIR)libucli386.a -output $(TARGET_DIR)libucl.a
    
clean:
	rm -f $(TARGET_DIR)libucl*.a
	rm -rf build*

$(OUT):
	@echo MKDIR $(OUT)
	$(SILENT)mkdir $(OUT)

