#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES=-I$(APPSDIR) -I.. -I. -I$(FIRMDIR)/include -I$(FIRMDIR)/export	\
 -I$(FIRMDIR)/common -I$(FIRMDIR)/drivers -I$(BUILDDIR)

ifdef APPEXTRA
   INCLUDES += $(patsubst %,-I$(APPSDIR)/%,$(subst :, ,$(APPEXTRA)))
endif

# Tremor is slightly faster on coldfire with -O3
ifeq ($(CPU),coldfire)
    TREMOROPTS = -O3
else
    TREMOROPTS = -O2
endif

CFLAGS = $(INCLUDES) $(GCCOPTS) $(TARGET_INC) $(TREMOROPTS) $(TARGET)	\
$(EXTRA_DEFINES) -DMEM=${MEMORYSIZE} ${PROFILE_OPTS}

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc

SOURCES = $(SRC)
OBJS2 := $(SRC:%.c=$(OBJDIR)/%.o)
OBJS = $(patsubst %.S, $(OBJDIR)/%.o, $(OBJS2))
DEPFILE = $(OBJDIR)/dep-libtremor
DIRS = 

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	$(call PRINTS,AR+RANLIB $(@F))$(AR) ruv $@ $+ >/dev/null 2>&1
	$(SILENT)$(RANLIB) $@

include $(TOOLSDIR)/make.inc

clean:
	$(call PRINTS,cleaning libtremor)rm -f $(OBJS) $(OUTPUT) $(DEPFILE)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPFILE)
endif
