#
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#

# The external Makefiles use ar to create libs. To allow cross-compiling pass
# the ar that matches the current gcc. Since qmake doesn't provide a variable
# holding the correct ar without any additions we need to figure it ourselves
# here.
# Only do this if CC is gcc. Also, do this before ccache support is enabled.
contains(QMAKE_CC,($$find(QMAKE_CC,.*gcc.*))) {
    EXTRALIBS_OPTS = "EXTRALIBS_AR=\""$$replace(QMAKE_CC,gcc.*,ar)\"
}
# ccache
unix:!mac:!noccache {
    CCACHE = $$system(which ccache)
    !isEmpty(CCACHE) {
        message("using ccache at $$CCACHE")
        QMAKE_CXX = ccache $$QMAKE_CXX
        QMAKE_CC = ccache $$QMAKE_CC
    }
}
MACHINEFLAGS = $$find(QMAKE_CFLAGS, -m[63][42])
EXTRALIBS_OPTS += EXTRALIBS_CC=\"$$QMAKE_CC\"
EXTRALIBS_OPTS += EXTRALIBS_CXX=\"$$QMAKE_CXX\"
EXTRALIBS_OPTS += EXTRALIB_CFLAGS=\"$$MACHINEFLAGS\"
EXTRALIBS_OPTS += EXTRALIB_CXXFLAGS=\"$$MACHINEFLAGS\"

MYBUILDDIR = $$OUT_PWD/build/
MYLIBBUILDDIR = $$MYBUILDDIR/libs/
OBJECTS_DIR = $$MYBUILDDIR/o
UI_DIR = $$MYBUILDDIR/ui
MOC_DIR = $$MYBUILDDIR/moc
RCC_DIR = $$MYBUILDDIR/rcc

!silent {
    VERBOSE = "V=1"
} else {
    VERBOSE =
}

# check version of Qt installation
contains(QT_MAJOR_VERSION, 4) {
    error("Qt 4 is not supported anymore.")
}

RBBASE_DIR = $$_PRO_FILE_PWD_
RBBASE_DIR = $$replace(RBBASE_DIR,/rbutil/rbutilqt,)

message("using Rockbox basedir $$RBBASE_DIR")

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

extralibs.commands = $$SILENT \
        $(MAKE) -f $$RBBASE_DIR/rbutil/rbutilqt/Makefile.libs \
        $$VERBOSE \
        SYS_SPEEX=\"$$LIBSPEEX\" \
        BUILD_DIR=$$MYLIBBUILDDIR/ \
        TARGET_DIR=$$MYLIBBUILDDIR \
        RBBASE_DIR=$$RBBASE_DIR \
        $$EXTRALIBS_OPTS \
        libs
# Note: order is important for RBLIBS! The libs are appended to the linker
# flags in this order, put libucl at the end.
RBLIBS = rbspeex ipodpatcher sansapatcher mkamsboot mktccboot \
         mkmpioboot chinachippatcher mkimxboot mks5lboot bspatch ucl \
         rbtomcrypt
# NOTE: Linking bzip2 causes problems on Windows (Qt seems to export those
# symbols as well, similar to what we have with zlib.) Only link that on
# non-Windows for now.
!win32 {
    RBLIBS += bzip2
}
!win32-msvc* {
    QMAKE_EXTRA_TARGETS += extralibs
    PRE_TARGETDEPS += extralibs
}
win32-msvc* {
    INCLUDEPATH += msvc
    LIBS += -L$$_PRO_FILE_PWD_/msvc
    LIBS += -ladvapi32 # required for MSVC / Qt Creator combination
}

# rule for creating ctags file
tags.commands = ctags -R --c++-kinds=+p --fields=+iaS --extra=+q $(SOURCES)
tags.depends = $(SOURCES)
QMAKE_EXTRA_TARGETS += tags

# add a custom rule for making the translations
LRELEASE = $$[QT_INSTALL_BINS]/lrelease

win32:!cross:!exists($$LRELEASE) {
    LRELEASE = $$[QT_INSTALL_BINS]/lrelease.exe
}
lrelease.commands = $$LRELEASE -silent $$_PRO_FILE_
QMAKE_EXTRA_TARGETS += lrelease
exists($$LRELEASE) {
    message("using lrelease at $$LRELEASE")
    PRE_TARGETDEPS += lrelease
}
!exists($$LRELEASE) {
    warning("could not find lrelease. Skipping translations.")
}

# Needed by QT on Win
INCLUDEPATH += $$_PRO_FILE_PWD_ $$_PRO_FILE_PWD_/irivertools \
            $$_PRO_FILE_PWD_/zlib $$_PRO_FILE_PWD_/base \
            $$_PRO_FILE_PWD_/zlib $$_PRO_FILE_PWD_/gui
INCLUDEPATH += $$RBBASE_DIR/rbutil/ipodpatcher $$RBBASE_DIR/rbutil/sansapatcher \
            $$RBBASE_DIR/tools/rbspeex $$RBBASE_DIR/tools
INCLUDEPATH += logger

DEPENDPATH = $$INCLUDEPATH

LIBS += -L$$OUT_PWD -L$$MYLIBBUILDDIR
# append all RBLIBS to LIBS
for(rblib, RBLIBS) {
    LIBS += -l$$rblib
}

# on win32 libz is linked implicitly.
!win32 {
    LIBS += -lz
}

# Add a (possibly found) libspeex now, don't do this before -lrbspeex!
!static:!isEmpty(LIBSPEEX) {
    LIBS += $$LIBSPEEX
}

TEMPLATE = app
TARGET = RockboxUtility
QT += network

message("Qt$$QT_MAJOR_VERSION found")
QT += widgets
if (lessThan(QT_MAJOR_VERSION, 6)) {
    QT += multimedia
}

CONFIG += c++11

dbg {
    CONFIG += debug thread qt warn_on
    DEFINES += DBG
    message("creating debug version")
}
!dbg {
    CONFIG += release thread qt
    DEFINES += NODEBUG
    message("creating release version")
}

DEFINES += RBUTIL _LARGEFILE64_SOURCE
DEFINES += QT_DEPRECATED_WARNINGS

# platform specific
win32 {
    # use MinGW's implementation of stdio functions for extended format string
    # support.
    DEFINES += __USE_MINGW_ANSI_STDIO=1
    DEFINES += _CRT_SECURE_NO_WARNINGS
    DEFINES += UNICODE
    LIBS += -lsetupapi -lnetapi32
}
win32:static {
    QMAKE_LFLAGS += -static-libgcc -static-libstdc++
}
unix:!static:!macx {
    LIBS += -lusb-1.0
}

unix:!macx:static {
    # force statically linking of libusb. Libraries that are appended
    # later will get linked dynamically again.
    LIBS += -Wl,-Bstatic -lusb-1.0 -Wl,-Bdynamic
}

macx {
    QMAKE_MAC_SDK=macosx
    contains(QT_MAJOR_VERSION, 5) {
        greaterThan(QT_MINOR_VERSION, 5) {
            QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
            message("Qt 5.6+ detected: setting deploy target to 10.7")
        }
        !greaterThan(QT_MINOR_VERSION, 5) {
            QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
            message("Qt up to 5.5 detected: setting deploy target to 10.6")
        }
    }

    CONFIG += x86
    LIBS += -L/usr/local/lib \
            -framework IOKit -framework CoreFoundation -framework Carbon \
            -framework SystemConfiguration -framework CoreServices
    INCLUDEPATH += /usr/local/include

    # rule for creating a dmg file
    dmg.commands = hdiutil create -ov -srcfolder RockboxUtility.app/ RockboxUtility.dmg
    QMAKE_EXTRA_TARGETS += dmg
}

static {
    if(equals(QT_MAJOR_VERSION, 5) : lessThan(QT_MINOR_VERSION, 4)) {
            QTPLUGIN += qtaccessiblewidgets
            LIBS += -L$$(QT_BUILD_TREE)/plugins/accessible -lqtaccessiblewidgets
    }
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
include(quazip/quazip.pri)
include(logger/logger.pri)
