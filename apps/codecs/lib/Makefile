#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# ../.. for the codec.h in the apps dir
# .. for stuff in the codecs dir
# . for stuff in the codeclib dir
INCLUDES=-I$(APPSDIR) -I.. -I. -I$(FIRMDIR)/include -I$(FIRMDIR)/export	\
 -I$(FIRMDIR)/common -I$(BUILDDIR)

ifdef APPEXTRA
   INCLUDES += $(patsubst %,-I$(APPSDIR)/%,$(subst :, ,$(APPEXTRA)))
endif

CFLAGS = $(INCLUDES) $(GCCOPTS) \
$(TARGET) $(EXTRA_DEFINES) -DMEM=${MEMORYSIZE} -DCODEC

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc

SOURCES = $(SRC)
OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
DEPFILE = $(OBJDIR)/dep-codeclib
DIRS = .

OUTPUT = $(BUILDDIR)/libcodec.a

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	@echo "AR+RANLIB $@"
	@$(AR) ruv $@ $+ >/dev/null 2>&1
	@$(RANLIB) $@

include $(TOOLSDIR)/make.inc

clean:
	@echo "cleaning codecs/lib"
	@rm -f $(OBJS) $(OUTPUT) $(DEPFILE)

-include $(DEPFILE)
