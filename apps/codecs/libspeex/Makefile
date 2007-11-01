#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile,v 1.16 2006-09-02 22:34:13 bagder Exp $
#

INCLUDES=-I$(APPSDIR) -I.. -I. -I$(FIRMDIR)/include -I$(FIRMDIR)/export	\
 -I$(FIRMDIR)/common -I$(FIRMDIR)/drivers -I$(BUILDDIR)

ifdef APPEXTRA
   INCLUDES += $(patsubst %,-I$(APPSDIR)/%,$(subst :, ,$(APPEXTRA)))
endif

SPEEXOPTS = -DHAVE_CONFIG_H -DSPEEX_DISABLE_ENCODER

# We're faster on ARM-targets with -O1 instead of -O2
ifeq ($(CPU),arm)
   SPEEXOPTS += -O
else
   SPEEXOPTS += -O2
endif

CFLAGS = $(INCLUDES) $(GCCOPTS) $(TARGET_INC) $(SPEEXOPTS) $(TARGET)	\
$(EXTRA_DEFINES) -DMEM=${MEMORYSIZE} ${PROFILE_OPTS} -Wno-unused-parameter

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc

SOURCES = $(SRC)
OBJS2 := $(SRC:%.c=$(OBJDIR)/%.o)
OBJS = $(patsubst %.S, $(OBJDIR)/%.o, $(OBJS2))
DEPFILE = $(OBJDIR)/dep-Speex
DIRS = 

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	@echo "AR+RANLIB $(notdir $@)"
	@$(AR) ruv $@ $+ >/dev/null 2>&1

include $(TOOLSDIR)/make.inc

clean:
	@echo "cleaning Speex"
	@rm -f $(OBJS) $(OUTPUT) $(DEPFILE)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPFILE)
endif
