#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

CRYPTOSRCDIR := $(APPSDIR)/plugins/crypto
CRYPTOBUILDDIR := $(BUILDDIR)/apps/plugins/crypto

ROCKS += $(CRYPTOBUILDDIR)/crypto.rock

CRYPTO_SRC := $(call preprocess, $(CRYPTOSRCDIR)/SOURCES)
CRYPTO_OBJ := $(call c2obj, $(CRYPTO_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(CRYPTO_SRC)

CRYPTOCFLAGS = $(PLUGINFLAGS) -I$(CRYPTOSRCDIR) -O2

$(CRYPTOBUILDDIR)/crypto.rock: $(CRYPTO_OBJ) $(TLSFLIB)

# new rule needed to use extra compile flags
$(CRYPTOBUILDDIR)/%.o: $(CRYPTOSRCDIR)/%.c
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) $(CRYPTOCFLAGS) -c $< -o $@


