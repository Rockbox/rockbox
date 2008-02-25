#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES= $(TARGET_INC) -I$(FIRMDIR)/include -I$(FIRMDIR)/export	\
-I. -I$(BUILDDIR) -I$(OBJDIR)

# the header we generate
HEADER = $(BUILDDIR)/max_language_size.h

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc
SOURCES=$(SRC)

# OUTP is the list of files to depend upon
OUTP = $(patsubst %.lang,$(OBJDIR)/%.lng, $(SOURCES))

# the generated file with features specified genlang-style
FEATS=$(BUILDDIR)/apps/genlang-features

ifndef V
SILENT=@
endif
PRINTS=$(SILENT)$(call info,$(1))

all: $(HEADER)

# generic rule for creating .lng from .lang
$(OBJDIR)/%.lng : %.lang $(FEATS)
	$(call PRINTS,GENLANG $<)
	$(SILENT)$(TOOLSDIR)/genlang -e=$(APPSDIR)/lang/english.lang -t=$(MODELNAME)`cat $(FEATS)` -i=$(TARGET_ID) -b=$@ $<

$(HEADER): $(OUTP)
	$(call PRINTS,Make $(HEADER))
	$(SILENT)echo "#define MAX_LANGUAGE_SIZE `ls -ln $(OBJDIR)/* | awk '{print $$5}' | sort -n | tail -1`" > $(HEADER)
