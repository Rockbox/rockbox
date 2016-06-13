#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

PASSMGRSRCDIR := $(APPSDIR)/plugins/passmgr
PASSMGRBUILDDIR := $(BUILDDIR)/apps/plugins/passmgr

ROCKS += $(PASSMGRBUILDDIR)/passmgr.rock

PASSMGR_SRC := $(call preprocess, $(PASSMGRSRCDIR)/SOURCES)
PASSMGR_OBJ := $(call c2obj, $(PASSMGR_SRC))

# add source files to OTHER_SRC to get automatic dependencies
OTHER_SRC += $(PASSMGR_SRC)

PASSMGRFLAGS = $(filter-out -O%,$(PLUGINFLAGS)) -O2

$(PASSMGRBUILDDIR)/passmgr.rock: $(PASSMGR_OBJ)

$(PASSMGRBUILDDIR)/%.o: $(PASSMGRSRCDIR)/%.c $(PASSMGRSRCDIR)/passmgr.make
	$(SILENT)mkdir -p $(dir $@)
	$(call PRINTS,CC $(subst $(ROOTDIR)/,,$<))$(CC) -I$(dir $<) $(PASSMGRFLAGS) -c $< -o $@
