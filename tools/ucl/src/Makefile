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
	ucl_dll.c ucl_init.c ucl_ptr.c ucl_str.c ucl_util.c

OBJS = $(OUT)/alloc.o $(OUT)/io.o $(OUT)/n2b_99.o $(OUT)/n2b_d.o $(OUT)/n2b_ds.o $(OUT)/n2b_to.o $(OUT)/n2d_99.o	\
	$(OUT)/n2d_d.o $(OUT)/n2d_ds.o $(OUT)/n2d_to.o $(OUT)/n2e_99.o $(OUT)/n2e_d.o $(OUT)/n2e_ds.o $(OUT)/n2e_to.o $(OUT)/ucl_crc.o	\
	$(OUT)/ucl_dll.o $(OUT)/ucl_init.o $(OUT)/ucl_ptr.o $(OUT)/ucl_str.o $(OUT)/ucl_util.o

# we don't use $(AR) and $(RANLIB) below since we want the _native_ tools
# not the cross-compiler tools
libucl$(RBARCH).a: $(OUT) $(OBJS)
	$(SILENT)ar ruv $(TARGET_DIR)$@ $(OBJS) >/dev/null 2>&1
	$(SILENT)ranlib $(TARGET_DIR)$@

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
	lipo -create $(TARGET_DIR)libuclppc.a $(TARGET_DIR)libucli386.a -output $(TARGET_DIR)libucl.a    
    
clean:
	rm -f $(TARGET_DIR)libucl*.a
	rm -rf build*

$(OUT):
	@echo MKDIR $(OUT)
	$(SILENT)mkdir $(OUT)

