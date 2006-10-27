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
PRINTS=$(info $(1))$(SILENT)

LDS := link.lds
LINKFILE = $(OBJDIR)/linkage.lds
OBJS := $(OBJDIR)/decompressor.o $(OBJDIR)/uclimage.o $(OBJDIR)/startup.o
CFLAGS = $(GCCOPTS)

all: $(OBJDIR)/compressed.bin

$(OBJDIR)/compressed.bin : $(OBJDIR)/compressed.elf
	$(call PRINTS,OBJCOPY $(@F))$(OC) -O binary $< $@

$(OBJDIR)/compressed.elf : $(OBJS) $(LINKFILE)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -Os -nostdlib -o $@ $(OBJS) -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/compressed.map

$(LDS): $(OBJS)
	

$(LINKFILE): $(LDS)
	$(call PRINTS,Build LDS file)cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) -E -P $(ROMBUILD) - >$@

$(OBJDIR)/decompressor.o : decompressor.c $(OBJDIR)/uclimage.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $<)$(CC) $(CFLAGS) -I$(OBJDIR) -c $< -o $@

$(OBJDIR)/startup.o : startup.S
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,AS $<)$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/uclimage.o : $(OBJDIR)/uclimage.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(<F))$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/uclimage.c : $(FLASHFILE)  $(TOOLSDIR)/ucl2src.pl
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,UCL2SRC)perl -s $(TOOLSDIR)/ucl2src.pl -p=$(OBJDIR)/uclimage $<

