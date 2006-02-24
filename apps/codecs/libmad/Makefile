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

# NOTE: FPM_ define has been moved to global.h
MADOPTS = -UDEBUG -DNDEBUG -O2
CFLAGS = $(INCLUDES) $(GCCOPTS) $(MADOPTS) $(TARGET) $(EXTRA_DEFINES) \
 -DMEM=${MEMORYSIZE} $(PROFILE_OPTS)

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc

SOURCES = $(SRC)
OBJS2 := $(SRC:%.c=$(OBJDIR)/%.o)
OBJS = $(patsubst %.S, $(OBJDIR)/%.o, $(OBJS2))
DEPFILE = $(OBJDIR)/dep-libmad
DIRS = 

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	@echo "AR $@"
	@$(AR) ruv $@ $+ >/dev/null 2>&1

include $(TOOLSDIR)/make.inc

clean:
	@echo "cleaning libmad"
	@rm -f $(OBJS) $(OUTPUT) $(DEPFILE)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPFILE)
endif
