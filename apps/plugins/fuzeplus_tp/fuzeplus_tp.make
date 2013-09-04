#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

FPTPSRCDIR := $(APPSDIR)/plugins/fuzeplus_tp
FPTPBUILDDIR := $(BUILDDIR)/apps/plugins/fuzeplus_tp

ROCKS += $(FPTPBUILDDIR)/fuzeplus_tp.rock

FPTP_SRC := $(call preprocess, $(FPTPSRCDIR)/SOURCES)
FPTP_OBJ := $(call c2obj, $(FPTP_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(FPTP_SRC)

$(FPTPBUILDDIR)/fuzeplus_tp.rock: $(FPTP_OBJ)
