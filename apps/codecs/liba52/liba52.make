#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# liba52
A52LIB := $(CODECDIR)/liba52.a
A52LIB_SRC := $(call preprocess, $(APPSDIR)/codecs/liba52/SOURCES)
A52LIB_OBJ := $(call c2obj, $(A52LIB_SRC))
OTHER_SRC += $(A52LIB_SRC)

$(A52LIB): $(A52LIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

A52FLAGS = -I$(APPSDIR)/codecs/liba52 $(filter-out -O%,$(CODECFLAGS))

ifeq ($(CPU),coldfire)
    A52FLAGS += -O2
else
    A52FLAGS += -O1
endif

$(CODECDIR)/liba52/%.o: $(ROOTDIR)/apps/codecs/liba52/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(A52FLAGS) -c $< -o $@

