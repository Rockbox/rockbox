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

all:
ifneq ($(strip $(BMP2RB_NATIVE)),)
	@echo "MAKE in plugins/bitmaps/native"
	$(SILENT)mkdir -p $(OBJDIR)/native
	@$(MAKE) -C native OBJDIR=$(OBJDIR)/native
endif
ifneq ($(strip $(BMP2RB_MONO)),)
	@echo "MAKE in plugins/bitmaps/mono"
	$(SILENT)mkdir -p $(OBJDIR)/mono
	@$(MAKE) -C mono OBJDIR=$(OBJDIR)/mono
endif
ifneq ($(strip $(BMP2RB_REMOTENATIVE)),)
	@echo "MAKE in plugins/bitmaps/remote_native"
	$(SILENT)mkdir -p $(OBJDIR)/remote_native
	@$(MAKE) -C remote_native OBJDIR=$(OBJDIR)/remote_native
endif
ifneq ($(strip $(BMP2RB_REMOTEMONO)),)
	@echo "MAKE in plugins/bitmaps/remote_mono"
	$(SILENT)mkdir -p $(OBJDIR)/remote_mono
	@$(MAKE) -C remote_mono OBJDIR=$(OBJDIR)/remote_mono
endif
