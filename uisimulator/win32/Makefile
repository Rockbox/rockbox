############################################################################
#             __________               __   ___.                  
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/ 
# $Id$
#
# Copyright (C) 2002 by Daniel Stenberg <daniel@haxx.se>
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
############################################################################

SIMCOMMON = ../common

DEPFILE = $(OBJDIR)/dep-simwin

RM = rm -f
DEBUG = -g

INCLUDES = -I. -I$(SIMCOMMON) -I$(OBJDIR) -I$(FIRMDIR)/export -I$(APPSDIR)

DEFINES = -DHAVE_CONFIG_H -DGETTIMEOFDAY_TWO_ARGS -DSIMULATOR \
$(TARGET) -DAPPSVERSION=\"$(VERSION)\" -DMEM=${MEMORYSIZE} $(EXTRA_DEFINES)

SRC := $(shell cat SOURCES | gcc -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) \
 $(TARGET) $(DEFINES) $(EXTRA_DEFINES) -E -P -include "config.h" - )
OBJS := $(SRC:%.c=$(OBJDIR)/%.o)

SOURCES = $(SRC)
DIRS = .

CFLAGS = $(DEBUG) $(DEFINES) $(INCLUDES) -W -Wall -mno-cygwin

OUTFILE = $(OBJDIR)/libsim.a

all: $(OUTFILE)
	@echo "MAKE in common sim"
	$(MAKE) -C $(SIMCOMMON)

$(OUTFILE): $(OBJS) $(OBJDIR)/UI256.bmp $(OBJDIR)/uisw32-res.o
	@echo "AR+RANLIB $@"
	@$(AR) ruv $@ $(OBJS) $(OBJDIR)/uisw32-res.o >/dev/null 2>&1
	@$(RANLIB) $@

clean:
	@echo "cleaning simwin"
	@$(RM) $(OBJS) *~ core $(OUTFILE) $(OBJDIR)/uisw32-res.o \
	$(OBJDIR)/UI256.bmp $(DEPFILE)
	@$(MAKE) -C $(SIMCOMMON) clean

################## Specific dependencies ##################
$(OBJDIR)/UI256.bmp: UI-$(ARCHOS).bmp
	@echo UI
	@cp $< $@

$(OBJDIR)/uisw32-res.o: uisw32.rc $(OBJDIR)/UI256.bmp
	$(WINDRES) --include-dir $(OBJDIR) -i $< -o $@

include $(TOOLSDIR)/make.inc

-include $(DEPFILE)
