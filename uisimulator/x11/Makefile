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

DEPFILE = $(OBJDIR)/dep-sim

RM = rm -f
DEBUG = -g

# Use this for simulator-only files
INCLUDES = -I. -I$(SIMCOMMON) -I$(OBJDIR) -I$(FIRMDIR)/export -I$(APPSDIR)

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc

OBJS := $(SRC:%.c=$(OBJDIR)/%.o)

DEFINES := -DHAVE_CONFIG_H -DGETTIMEOFDAY_TWO_ARGS -DSIMULATOR \
$(TARGET) -DAPPSVERSION=\"$(VERSION)\" -DMEM=${MEMORYSIZE} $(EXTRA_DEFINES) 

SOURCES = $(SRC)

DIRS = .

CFLAGS = $(DEBUG) $(DEFINES) $(INCLUDES) $(GCCOPTS)

OUTFILE = $(OBJDIR)/libsim.a

all: $(OUTFILE)
	@$(MAKE) -C $(SIMCOMMON)

include $(TOOLSDIR)/make.inc

clean:
	@echo "cleaning sim"
	@$(RM) $(OBJS) *~ core $(OUTFILE) $(DEPFILE)
	@$(MAKE) -C $(SIMCOMMON) clean

$(OUTFILE): $(OBJS)
	@echo "AR $@"
	@$(AR) ruv $@ $(OBJS) >/dev/null 2>&1

-include $(DEPFILE)
