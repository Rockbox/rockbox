#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
CC = gcc
CXX = $(shell $(PREFIX)wx-config --version=2.8 --cxx)
INC = `$(PREFIX)wx-config --version=2.8 --cxxflags`
LIBS = `$(PREFIX)wx-config --version=2.8 --libs`

CFLAGS = -Wall -Wundef -DRBUTIL -D_LARGEFILE64_SOURCE
ICONS := $(wildcard *.png)
HEADERS := $(subst .png,.h,$(ICONS))
OBJS := $(subst .png,.o,$(ICONS))
CXXSOURCES = $(subst .png,.cpp,$(ICONS))
SILENT = @
BIN2C = ./bin2c

all: $(OBJS)
	

bin2c: bin2c.c
	$(SILENT) echo CC $<
	$(SILENT) $(CC) -o $@ $<

%.cpp: %.png bin2c
	$(SILENT) echo BIN2C $<
	$(SILENT) $(BIN2C) $<

%.o: %.cpp
	$(SILENT) echo CXX $<
	$(SILENT) $(CXX) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	$(SILENT) echo cleaning icons. 
	$(SILENT) rm -f bin2c $(OBJS) $(HEADERS) $(CXXSOURCES)
