#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

MIPSUNWINDERLIB_DIR = $(ROOTDIR)/lib/mipsunwinder
MIPSUNWINDERLIB_SRC = $(call preprocess, $(MIPSUNWINDERLIB_DIR)/SOURCES)
MIPSUNWINDERLIB_OBJ := $(call c2obj, $(MIPSUNWINDERLIB_SRC))

OTHER_SRC += $(MIPSUNWINDERLIB_SRC)

MIPSUNWINDERLIB = $(BUILDDIR)/lib/libmipsunwinder.a
CORE_LIBS += $(MIPSUNWINDERLIB)

INCLUDES += -I$(MIPSUNWINDERLIB_DIR)
DEFINES += -DBACKTRACE_MIPSUNWINDER

$(MIPSUNWINDERLIB): $(MIPSUNWINDERLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
