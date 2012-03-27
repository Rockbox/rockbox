#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

TLSFLIB_DIR := $(ROOTDIR)/lib/tlsf
TLSFLIB_SRC := $(call preprocess, $(TLSFLIB_DIR)/SOURCES)
TLSFLIB_OBJ := $(call c2obj, $(TLSFLIB_SRC))
TLSFLIB := $(BUILDDIR)/lib/libtlsf.a

OTHER_SRC += $(TLSFLIB_SRC)
INCLUDES += -I$(TLSFLIB_DIR)/src
EXTRA_LIBS += $(TLSFLIB)

TLSFLIBFLAGS = $(CFLAGS) -fstrict-aliasing -ffunction-sections $(SHARED_CFLAGS)

# Enable statistics in the sim
ifneq ($(findstring sdl-sim, $(APP_TYPE)), sdl-sim)
  TLSFLIBFLAGS += -DTLSF_STATISTIC=1
endif

# special rules for tlsf
$(BUILDDIR)/lib/tlsf/src/%.o: $(TLSFLIB_DIR)/src/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -c $< -o $@ \
	-I$(TLSFLIB_DIR)/src $(TLSFLIBFLAGS)

$(TLSFLIB): $(TLSFLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
