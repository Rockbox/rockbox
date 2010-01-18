#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

IMGVSRCDIR := $(APPSDIR)/plugins/imageviewer
IMGVBUILDDIR := $(BUILDDIR)/apps/plugins/imageviewer

# include actual viewer's make file
IMGVSUBDIRS := $(call preprocess, $(IMGVSRCDIR)/SUBDIRS)
$(foreach dir,$(IMGVSUBDIRS),$(eval include $(dir)/$(notdir $(dir)).make))

