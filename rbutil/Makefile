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
INC = `$(PREFIX)wx-config --version=2.8 --cxxflags`
LIBS = `$(PREFIX)wx-config --version=2.8 --libs`
CFLAGS = -Wall -Wundef -DRBUTIL -D_LARGEFILE64_SOURCE
OBJS=rbutil.o rbutilApp.o rbutilFrm.o rbutilCtrls.o install_dialogs.o bootloaders.o installlog.o ipodpatcher/ipodpatcher.o ipodpatcher/ipodio-posix.o sansapatcher/sansapatcher.o sansapatcher/sansaio-posix.o irivertools.o md5sum.o autodetection.o 
EXTRAOBJS = $(wildcard icons/*.o)

SILENT = @

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
	install -D rbutil$(EXT) $(DESTDIR)/bin/rbutil$(EXT)
