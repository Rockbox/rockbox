#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

SETJMP_DIR = $(ROOTDIR)/lib/libsetjmp
SETJMP_SRC = $(call preprocess, $(SETJMP_DIR)/SOURCES)
SETJMP_OBJ := $(call c2obj, $(SETJMP_SRC))

OTHER_SRC += $(SETJMP_SRC)

LIBSETJMP = $(BUILDDIR)/lib/libsetjmp.a

INCLUDES += -I$(SETJMP_DIR)

$(LIBSETJMP): $(SETJMP_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
