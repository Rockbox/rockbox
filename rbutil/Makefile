#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# Unix-style Makefile for rbutil

#detect cygwin 
ifneq ($(strip $(shell gcc -v 2>&1 | grep "cygwin")),)
   __CYGWIN__ := 1
endif

CXX=$(shell $(PREFIX)wx-config --version=2.8 --cxx)
INC = `$(PREFIX)wx-config --version=2.8 --cxxflags`
LIBS = `$(PREFIX)wx-config --version=2.8 --libs`
CFLAGS= -Wall -Wundef -DRBUTIL -D_LARGEFILE64_SOURCE
OBJS=rbutil.o rbutilApp.o rbutilFrm.o rbutilCtrls.o install_dialogs.o bootloaders.o installlog.o ipodpatcher/ipodpatcher.o sansapatcher/sansapatcher.o irivertools.o md5sum.o autodetection.o talkfile.o

ifdef __CYGWIN__
OBJS+=ipodpatcher/ipodio-win32.o sansapatcher/sansaio-win32.o
CFLAGS+= -mno-cygwin -mwindows
else
OBJS+=ipodpatcher/ipodio-posix.o sansapatcher/sansaio-posix.o
endif


EXTRAOBJS = $(wildcard icons/*.o)

SILENT = @

# Install with / as root by default
ifndef DESTDIR
  DESTDIR=""
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
all: icons rbutil$(EXT)

.cpp.o :
	$(SILENT) echo CXX $<
	$(SILENT) $(CXX) $(CFLAGS) $(INC) -c -o $@ $<

.c.o :
	$(SILENT) echo CC $<
	$(SILENT) $(CC) $(CFLAGS) $(INC) -c -o $@ $<

rbutil-rc.o: rbutil-rc.rc
	$(WINDRES) -O coff -F pe-i386 -o $@ $<

rbutil$(EXT): $(OBJS)
	$(SILENT) echo CXX $<
	$(SILENT) $(CXX) -o $@ $(OBJS) $(EXTRAOBJS) $(LIBS)

.PHONY: icons
icons:
	$(SILENT) $(MAKE) -C icons

.PHONY: clean
clean:
	rm -f rbutil rbutil.exe $(OBJS) *~
	make -C icons clean

.PHONY: install
install:
	install -D rbutil$(EXT) $(DESTDIR)/usr/bin/rbutil$(EXT)
