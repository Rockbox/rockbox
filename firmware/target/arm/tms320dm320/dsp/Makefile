#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 12058 2007-01-18 00:46:52Z dave $
#

# http://daniel.haxx.se/blog/2007/11/18/free-to-use-compiler-from-ti/
CC = cl500
LD = lnk500
CFLAGS = 
# There's more in linker.cmd.
LDFLAGS = -w

OBJDIR=./build

OBJS = arm.obj main.obj vectors.obj dma.obj

ifeq ($(findstring -DCREATIVE_ZV,$(TARGET)), -DCREATIVE_ZV)
OBJS += aic23.obj
else
OBJS += tsc2100.obj
endif

OBJS := $(patsubst %.obj, $(OBJDIR)/%.obj, $(OBJS))

all: dsp-image.h
	
clean:
	$(call PRINTS,cleaning DSP firmware)rm -f $(OBJS) $(OBJDIR)/dsp-image.out $(OBJDIR)/dsp-image.xml
	rmdir $(OBJDIR)

dsp-image.h: $(OBJS) linker.cmd
	$(call PRINTS,LNK500 dsp-image.out)lnk500 $(LDFLAGS) -o $(OBJDIR)/dsp-image.out $^
	$(call PRINTS,OFD500+XML2H $(@F))ofd500 -x -o /dev/stdout $(OBJDIR)/dsp-image.out | python xml2h.py $(OBJDIR)/dsp-image.xml > $@

$(OBJDIR)/%.obj: %.asm
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CL500 $<)$(CC) $(CFLAGS) -fr $(dir $@) $<

$(OBJDIR)/%.obj: %.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CL500 $<)$(CC) $(CFLAGS) -fr $(dir $@) $<

$(OBJDIR)/arm.obj: arm.c arm.h registers.h ipc.h

$(OBJDIR)/main.obj: main.c arm.h registers.h ipc.h dma.h audio.h

$(OBJDIR)/aic23.obj: aic23.c audio.h registers.h

$(OBJDIR)/tsc2100.obj: tsc2100.c audio.h registers.h

$(OBJDIR)/dma.obj: dma.c dma.h registers.h ipc.h

