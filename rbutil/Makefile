#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# Unix-style Makefile for rbutil

CXX=$(shell $(PREFIX)wx-config --cxx)
INC =
OBJS=rbutil.o rbutilApp.o rbutilFrm.o wizard_pages.o bootloaders.o installlog.o ipodpatcher/ipodpatcher.o ipodpatcher/ipodio-posix.o irivertools.o md5sum.o

# Install into /usr/local by default
ifndef DESTDIR
  DESTDIR=/usr/local
endif

# type "make WIN=1" for a Windows build using the Debian mingw cross-compiler
ifdef WIN
  CROSS=i586-mingw32msvc-
  WINDRES=i586-mingw32msvc-windres
  EXT=.exe
  PREFIX=/usr/i586-mingw32msvc/bin/
  OBJS+=rbutil-rc.o
endif

.PHONY: all
all: rbutil$(EXT)

.cpp.o :
	$(CXX) $(INC) -c `$(PREFIX)wx-config --cxxflags` -o $@ $<

.c.o :
	$(CC) $(INC) -c `$(PREFIX)wx-config --cxxflags` -o $@ $<

rbutil-rc.o: rbutil-rc.rc
	$(WINDRES) `$(PREFIX)wx-config --cxxflags` -O coff -F pe-i386 -o $@ $<

rbutil$(EXT): $(OBJS)
	$(CXX) -o $@ $(OBJS) `$(PREFIX)wx-config --libs`

.PHONY: clean
clean:
	rm -f rbutil rbutil.exe $(OBJS) *~

.PHONY: install
	install -D rbutil$(EXT) $(DESTDIR)/bin/rbutil$(EXT)
