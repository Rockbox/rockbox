#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: libwmavoice.make 27586 2010-07-27 06:48:15Z nls $
#

# libwmavoice
WMAVOICELIB := $(CODECDIR)/libwmavoice.a
WMAVOICELIB_SRC := $(call preprocess, $(RBCODECLIB_DIR)/codecs/libwmavoice/SOURCES)
WMAVOICELIB_OBJ := $(call c2obj, $(WMAVOICELIB_SRC))
OTHER_SRC += $(WMAVOICELIB_SRC)

$(WMAVOICELIB): $(WMAVOICELIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

WMAVOICEFLAGS = -I$(RBCODECLIB_DIR)/codecs/libwmavoice $(filter-out -O%,$(CODECFLAGS))

ifeq ($(ARCH),arch_m68k)
	WMAVOICEFLAGS += -O2
else
	WMAVOICEFLAGS += -O1
endif

ifeq ($(APP_TYPE),sdl-sim)
# wmavoice needs libm in the simulator
$(CODECDIR)/wmavoice.codec: $(CODECDIR)/wmavoice.o
	$(call PRINTS,LD $(@F))$(CC) $(CODECFLAGS) -o $(CODECDIR)/wmavoice.elf \
	$(filter %.o, $^) \
	$(filter %.a, $+) \
	-lgcc -lm $(CODECLDFLAGS)
	$(SILENT)cp $(CODECDIR)/wmavoice.elf $@
endif

