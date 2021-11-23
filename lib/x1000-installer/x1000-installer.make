#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

X1000INSTALLER_DIR = $(ROOTDIR)/lib/x1000-installer
X1000INSTALLER_SRC = $(call preprocess, $(X1000INSTALLER_DIR)/SOURCES)
X1000INSTALLER_OBJ := $(call c2obj, $(X1000INSTALLER_SRC))

X1000INSTALLERLIB = $(BUILDDIR)/lib/libx1000-installer.a

INCLUDES += -I$(X1000INSTALLER_DIR)/include
OTHER_SRC += $(X1000INSTALLER_SRC)
CORE_LIBS += $(X1000INSTALLERLIB)

$(X1000INSTALLERLIB): $(X1000INSTALLER_OBJ)
	$(SILENT)$(shell rm -f $@)
	$(call PRINTS,AR $(@F))$(AR) rcs $@ $^ >/dev/null
