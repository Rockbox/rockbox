#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

ifndef V
SILENT=@
endif

LDS := link.lds
LINKFILE = $(OBJDIR)/linkage.lds
OBJS := $(OBJDIR)/decompressor.o $(OBJDIR)/uclimage.o $(OBJDIR)/startup.o
CFLAGS = $(GCCOPTS)

all: $(OBJDIR)/compressed.bin

$(OBJDIR)/compressed.bin : $(OBJDIR)/compressed.elf
	@echo "OBJCOPY "`basename $@`
	$(SILENT)$(OC) -O binary $< $@

$(OBJDIR)/compressed.elf : $(OBJS) $(LINKFILE)
	@echo "LD `basename $@`"
	$(SILENT)$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJS) -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/compressed.map

$(LDS): $(OBJS)
	

$(LINKFILE): $(LDS)
	@echo "Build LDS file"
	$(SILENT)cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - >$@

$(OBJDIR)/decompressor.o : decompressor.c $(OBJDIR)/uclimage.c
	$(SILENT)mkdir -p `dirname $@`
	@echo "CC $<"
	$(SILENT)$(CC) $(CFLAGS) -I$(OBJDIR) -c $< -o $@

$(OBJDIR)/startup.o : startup.S
	$(SILENT)mkdir -p `dirname $@`
	@echo "AS $<"
	$(SILENT)$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/uclimage.o : $(OBJDIR)/uclimage.c
	$(SILENT)mkdir -p `dirname $@`
	@echo "CC `basename $<`"
	$(SILENT)$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/uclimage.c : $(FLASHFILE)  $(TOOLSDIR)/ucl2src.pl
	$(SILENT)mkdir -p `dirname $@`
	@echo "UCL2SRC"
	$(SILENT)perl -s $(TOOLSDIR)/ucl2src.pl -p=$(OBJDIR)/uclimage $<

