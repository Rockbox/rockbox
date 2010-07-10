#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

TLSFLIB := $(CODECDIR)/libtlsf.a
TLSFLIB_SRC := $(call preprocess, $(APPSDIR)/codecs/lib/tlsf/SOURCES)
TLSFLIB_OBJ := $(call c2obj, $(TLSFLIB_SRC))
OTHER_SRC += $(TLSFLIB_SRC)

$(TLSFLIB): $(TLSFLIB_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null

TLSFLIBFLAGS = $(CODECFLAGS) -ffunction-sections

ifdef APP_TYPE
    TLSFLIBFLAGS += -DTLSF_STATISTIC=1
endif

$(CODECDIR)/lib/tlsf/src/%.o: $(APPSDIR)/codecs/lib/tlsf/src/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) \
		-I$(dir $<) $(TLSFLIBFLAGS) -c $< -o $@
