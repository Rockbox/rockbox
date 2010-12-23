#
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#

# ccache
unix:!mac:!noccache {
    CCACHE = $$system(which ccache)
    !isEmpty(CCACHE) {
        message("using ccache")
        QMAKE_CXX = ccache g++
        QMAKE_CC = ccache gcc
    }
}

MYBUILDDIR = $$OUT_PWD/build/
OBJECTS_DIR = $$MYBUILDDIR/o
UI_DIR = $$MYBUILDDIR/ui
MOC_DIR = $$MYBUILDDIR/moc
RCC_DIR = $$MYBUILDDIR/rcc


# check version of Qt installation
VER = $$find(QT_VERSION, ^4\\.[5-9]+.*)
isEmpty(VER) {
    message("Qt >= 4.5 required!")
    !isEmpty(QT_VERSION) error("Qt found:" $$[QT_VERSION])
}
message("Qt version used:" $$VER)

RBBASE_DIR = $$_PRO_FILE_PWD_
RBBASE_DIR = $$replace(RBBASE_DIR,/rbutil/rbutilqt,)

message("Rockbox Base dir: "$$RBBASE_DIR)

# custom rules for rockbox-specific libs
!mac {
    RBLIBPOSTFIX = .a
}
mac {
    RBLIBPOSTFIX = -universal
}
# check for system speex. Add a custom rule for pre-building librbspeex if not
# found. Newer versions of speex are split up into libspeex and libspeexdsp,
# and some distributions package them separately. Check for both and fall back
# to librbspeex if not found.
# NOTE: keep adding the linker option after -lrbspeex, otherwise linker errors
# occur if the linker defaults to --as-needed
# (see http://www.gentoo.org/proj/en/qa/asneeded.xml)
#
# Always use our own copy when building statically. Don't search for libspeex
# on Mac, since we don't deploy statically there.
!static:unix:!mac {
    LIBSPEEX = $$system(pkg-config --silence-errors --libs speex speexdsp)
}

rbspeex.commands = @$(MAKE) \
        TARGET_DIR=$$MYBUILDDIR -C $$RBBASE_DIR/tools/rbspeex \
        librbspeex$$RBLIBPOSTFIX \
        CC=\"$$QMAKE_CC\" \
        SYS_SPEEX=\"$$LIBSPEEX\"
libucl.commands = @$(MAKE) \
        TARGET_DIR=$$MYBUILDDIR -C $$RBBASE_DIR/tools/ucl/src \
        libucl$$RBLIBPOSTFIX \
        CC=\"$$QMAKE_CC\"
libmkamsboot.commands = @$(MAKE) \
        TARGET_DIR=$$MYBUILDDIR -C $$RBBASE_DIR/rbutil/mkamsboot \
        APPVERSION=\"rbutil\" \
        libmkamsboot$$RBLIBPOSTFIX \
        CC=\"$$QMAKE_CC\"
libmktccboot.commands = @$(MAKE) \
        TARGET_DIR=$$MYBUILDDIR -C $$RBBASE_DIR/rbutil/mktccboot \
        libmktccboot$$RBLIBPOSTFIX \
        CC=\"$$QMAKE_CC\"
libmkmpioboot.commands = @$(MAKE) \
        TARGET_DIR=$$MYBUILDDIR -C $$RBBASE_DIR/rbutil/mkmpioboot \
        APPVERSION=\"rbutil\" \
        libmkmpioboot$$RBLIBPOSTFIX \
        CC=\"$$QMAKE_CC\"
QMAKE_EXTRA_TARGETS += rbspeex libucl libmkamsboot libmktccboot libmkmpioboot
PRE_TARGETDEPS += rbspeex libucl libmkamsboot libmktccboot libmkmpioboot

# rule for creating ctags file
tags.commands = ctags -R --c++-kinds=+p --fields=+iaS --extra=+q $(SOURCES)
tags.depends = $(SOURCES)
QMAKE_EXTRA_TARGETS += tags

# add a custom rule for making the translations
lrelease.commands = $$[QT_INSTALL_BINS]/lrelease -silent $$_PRO_FILE_
QMAKE_EXTRA_TARGETS += lrelease
!dbg {
    PRE_TARGETDEPS += lrelease
}

# Needed by QT on Win
INCLUDEPATH = $$_PRO_FILE_PWD_ $$_PRO_FILE_PWD_/irivertools \
            $$_PRO_FILE_PWD_/zip $$_PRO_FILE_PWD_/zlib $$_PRO_FILE_PWD_/base
INCLUDEPATH += $$RBBASE_DIR/rbutil/ipodpatcher $$RBBASE_DIR/rbutil/sansapatcher \
            $$RBBASE_DIR/tools/rbspeex $$RBBASE_DIR/tools

DEPENDPATH = $$INCLUDEPATH

LIBS += -L$$OUT_PWD -L$$MYBUILDDIR -lrbspeex -lmkamsboot -lmktccboot -lmkmpioboot -lucl

# Add a (possibly found) libspeex now, don't do this before -lrbspeex!
!static:!isEmpty(LIBSPEEX) {
    LIBS += $$LIBSPEEX
}

TEMPLATE = app
TARGET = RockboxUtility
QT += network
dbg {
    CONFIG += debug thread qt warn_on
    DEFINES -= QT_NO_DEBUG_OUTPUT
    message("debug")
}
!dbg {
    CONFIG += release thread qt
    DEFINES -= QT_NO_DEBUG_OUTPUT
    DEFINES += NODEBUG
    message("release")
}

DEFINES += RBUTIL _LARGEFILE64_SOURCE

# platform specific
win32 {
    # use MinGW's implementation of stdio functions for extended format string
    # support.
    DEFINES += __USE_MINGW_ANSI_STDIO=1
    LIBS += -lsetupapi -lnetapi32
}
win32:static {
    QMAKE_LFLAGS += -static-libgcc -static-libstdc++
}
unix:!static:!libusb1:!macx {
    LIBS += -lusb
}
unix:!static:libusb1:!macx {
    DEFINES += LIBUSB1
    LIBS += -lusb-1.0
}
unix {
    # explicitly link zlib, we do need it. Don't rely on implicit linking via Qt.
    LIBS += -lz
}

unix:!macx:static {
    # force statically linking of libusb. Libraries that are appended
    # later will get linked dynamically again.
    LIBS += -Wl,-Bstatic -lusb -Wl,-Bdynamic
}

# if -config intel is specified use 10.5 SDK and don't build for PPC
macx:!intel {
    CONFIG += ppc
    QMAKE_LFLAGS_PPC=-mmacosx-version-min=10.4 -arch ppc
    QMAKE_LFLAGS_X86=-mmacosx-version-min=10.4 -arch i386
    QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
}
macx:intel {
    QMAKE_LFLAGS_X86=-mmacosx-version-min=10.5 -arch i386
    QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.5.sdk
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5
}
macx {
    CONFIG += x86
    LIBS += -L/usr/local/lib -lz \
            -framework IOKit -framework CoreFoundation -framework Carbon \
            -framework SystemConfiguration -framework CoreServices
    INCLUDEPATH += /usr/local/include
    
    # rule for creating a dmg file
    dmg.commands = hdiutil create -ov -srcfolder rbutilqt.app/ rbutil.dmg
    QMAKE_EXTRA_TARGETS += dmg
}

static {
    QTPLUGIN += qtaccessiblewidgets
    LIBS += -L$$(QT_BUILD_TREE)/plugins/accessible -lqtaccessiblewidgets
    LIBS += -L.
    DEFINES += STATIC
    message("using static plugin")
}

unix {
    target.path = /usr/local/bin
    INSTALLS += target
}

# source files are separate.
include(rbutilqt.pri)

