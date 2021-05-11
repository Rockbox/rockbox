#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

MICROTARLIB_DIR = $(ROOTDIR)/lib/microtar
MICROTARLIB_SRC = $(call preprocess, $(MICROTARLIB_DIR)/SOURCES)
MICROTARLIB_OBJ := $(call c2obj, $(MICROTARLIB_SRC))

MICROTARLIB = $(BUILDDIR)/lib/libmicrotar.a

INCLUDES += -I$(MICROTARLIB_DIR)/src
OTHER_SRC += $(MICROTARLIB_SRC)
CORE_LIBS += $(MICROTARLIB)

$(MICROTARLIB): $(MICROTARLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
