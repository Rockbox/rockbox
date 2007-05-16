#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# Unix-style Makefile for rbutil

CXX=$(shell $(PREFIX)wx-config --version=2.8 --cxx)
INC =
CFLAGS = -Wall -Wundef -DRBUTIL
OBJS=rbutil.o rbutilApp.o rbutilFrm.o rbutilCtrls.o install_dialogs.o bootloaders.o installlog.o ipodpatcher/ipodpatcher.o ipodpatcher/ipodio-posix.o sansapatcher/sansapatcher.o sansapatcher/sansaio-posix.o irivertools.o md5sum.o

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
	$(CXX) $(CFLAGS) $(INC) -c `$(PREFIX)wx-config --version=2.8 --cxxflags` -o $@ $<

.c.o :
	$(CC) $(CFLAGS) $(INC) -c `$(PREFIX)wx-config --version=2.8 --cxxflags` -o $@ $<

rbutil-rc.o: rbutil-rc.rc
	$(WINDRES) `$(PREFIX)wx-config --version=2.8 --cxxflags` -O coff -F pe-i386 -o $@ $<

rbutil$(EXT): $(OBJS)
	$(CXX) -o $@ $(OBJS) `$(PREFIX)wx-config --version=2.8 --libs`

.PHONY: clean
clean:
	rm -f rbutil rbutil.exe $(OBJS) *~

.PHONY: install
	install -D rbutil$(EXT) $(DESTDIR)/bin/rbutil$(EXT)
