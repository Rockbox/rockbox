#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

TEXT_VIEWER_SRCDIR := $(APPSDIR)/plugins/text_viewer
TEXT_VIEWER_BUILDDIR := $(BUILDDIR)/apps/plugins/text_viewer

TEXT_VIEWER_SRC := $(call preprocess, $(TEXT_VIEWER_SRCDIR)/SOURCES)
TEXT_VIEWER_OBJ := $(call c2obj, $(TEXT_VIEWER_SRC))

OTHER_SRC += $(TEXT_VIEWER_SRC)

ROCKS += $(TEXT_VIEWER_BUILDDIR)/text_viewer.rock

$(TEXT_VIEWER_BUILDDIR)/text_viewer.rock: $(TEXT_VIEWER_OBJ)
