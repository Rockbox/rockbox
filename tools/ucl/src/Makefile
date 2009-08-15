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

OUT = build$(RBARCH)

SOURCES = alloc.c io.c n2b_99.c n2b_d.c n2b_ds.c n2b_to.c n2d_99.c	\
	n2d_d.c n2d_ds.c n2d_to.c n2e_99.c n2e_d.c n2e_ds.c n2e_to.c ucl_crc.c	\
	ucl_dll.c ucl_init.c ucl_ptr.c ucl_str.c ucl_util.c

OBJS = $(OUT)/alloc.o $(OUT)/io.o $(OUT)/n2b_99.o $(OUT)/n2b_d.o $(OUT)/n2b_ds.o $(OUT)/n2b_to.o $(OUT)/n2d_99.o	\
	$(OUT)/n2d_d.o $(OUT)/n2d_ds.o $(OUT)/n2d_to.o $(OUT)/n2e_99.o $(OUT)/n2e_d.o $(OUT)/n2e_ds.o $(OUT)/n2e_to.o $(OUT)/ucl_crc.o	\
	$(OUT)/ucl_dll.o $(OUT)/ucl_init.o $(OUT)/ucl_ptr.o $(OUT)/ucl_str.o $(OUT)/ucl_util.o

# we don't use $(AR) and $(RANLIB) below since we want the _native_ tools
# not the cross-compiler tools
libucl$(RBARCH).a: $(OUT) $(OBJS)
	$(SILENT)ar ruv $@ $(OBJS) >/dev/null 2>&1
	$(SILENT)ranlib $@

$(OUT)/%.o: %.c
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -c $< -o $@
    
# some trickery to build ppc and i386 from a single call
ifeq ($(RBARCH),)
libucli386.a:
	make RBARCH=i386 libucli386.a

libuclppc.a:
	make RBARCH=ppc libuclppc.a
endif    
    
libucl-universal: libucli386.a libuclppc.a
	@echo lipo libucl.a
	$(SILENT) rm -f libucl.a
	lipo -create libuclppc.a libucli386.a -output libucl.a    
    
clean:
	rm -f libucl*.a
	rm -rf build*

build$(RBARCH):
	@echo MKDIR build$(RBARCH)
	$(SILENT)mkdir build$(RBARCH)

