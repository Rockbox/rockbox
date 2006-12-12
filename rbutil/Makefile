#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# Unix-style Makefile for rbutil 


# type "make WIN=1" for a Windows build using the Debian mingw cross-compiler
ifdef WIN
  CROSS=i586-mingw32msvc-
  WINDRES=i586-mingw32msvc-windres
  EXT=.exe
  PREFIX=/usr/i586-mingw32msvc/bin/
endif


CC=$(CROSS)gcc
CXX=$(shell $(PREFIX)wx-config --cxx)
CFLAGS=-Wall $(INCLUDES)
LIBS=-lm
OBJS=main.o rbutil.o rbutilApp.o rbutilFrm.o wizard_pages.o rbutil-rc.o

.PHONY: all
all: rbutil$(EXT)

.cpp.o :
	$(CXX) -c `$(PREFIX)wx-config --cxxflags` -o $@ $<

rbutil-rc.o: rbutil-rc.rc
	$(WINDRES) `$(PREFIX)wx-config --cxxflags` -O coff -F pe-i386 -o $@ $<

rbutil$(EXT): $(OBJS)
	$(CXX) -o $@ $(OBJS) `$(PREFIX)wx-config --libs`

.PHONY: clean
clean:
	rm -f rbutil rbutil.exe $(OBJS) *~
