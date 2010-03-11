
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
VER = $$find(QT_VERSION, ^4\.[3-9]+.*)
isEmpty(VER) {
    !isEmpty(QT_VERSION) error("Qt found:" $$[QT_VERSION])
    error("Qt >= 4.3 required!")
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
rbspeex.commands = @$(MAKE) TARGET_DIR=$$MYBUILDDIR -C $$RBBASE_DIR/tools/rbspeex librbspeex$$RBLIBPOSTFIX CC=\"$$QMAKE_CC\"
libucl.commands = @$(MAKE) TARGET_DIR=$$MYBUILDDIR -C $$RBBASE_DIR/tools/ucl/src libucl$$RBLIBPOSTFIX CC=\"$$QMAKE_CC\"
libmkamsboot.commands = @$(MAKE) TARGET_DIR=$$MYBUILDDIR -C $$RBBASE_DIR/rbutil/mkamsboot libmkamsboot$$RBLIBPOSTFIX CC=\"$$QMAKE_CC\"
libmktccboot.commands = @$(MAKE) TARGET_DIR=$$MYBUILDDIR -C $$RBBASE_DIR/rbutil/mktccboot libmktccboot$$RBLIBPOSTFIX CC=\"$$QMAKE_CC\"
QMAKE_EXTRA_TARGETS += rbspeex libucl libmkamsboot libmktccboot
PRE_TARGETDEPS += rbspeex libucl libmkamsboot libmktccboot

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

SOURCES += rbutilqt.cpp \
 main.cpp \
 installwindow.cpp \
 base/httpget.cpp \
 configure.cpp \
 zip/zip.cpp \
 zip/unzip.cpp \
 base/zipinstaller.cpp \
 progressloggergui.cpp \
 installtalkwindow.cpp \
 base/talkfile.cpp \
 base/talkgenerator.cpp \
 base/autodetection.cpp \
 ../ipodpatcher/ipodpatcher.c \
 ../sansapatcher/sansapatcher.c \
 ../chinachippatcher/chinachip.c \
 browsedirtree.cpp \
 themesinstallwindow.cpp \
 base/uninstall.cpp \
 uninstallwindow.cpp \
 base/utils.cpp \
 preview.cpp \
 base/encoders.cpp \
 encttscfggui.cpp \
 base/encttssettings.cpp \
 base/ttsbase.cpp \
 base/ttsexes.cpp \
 base/ttssapi.cpp \
 base/ttsfestival.cpp \
 ../../tools/wavtrim.c \
 ../../tools/voicefont.c \
 base/voicefile.cpp \
 createvoicewindow.cpp \
 base/rbsettings.cpp \
 base/serverinfo.cpp \
 base/systeminfo.cpp \
 base/rbunzip.cpp \
 base/rbzip.cpp \
 base/system.cpp \
 sysinfo.cpp \
 systrace.cpp \
 base/bootloaderinstallbase.cpp \
 base/bootloaderinstallmi4.cpp \
 base/bootloaderinstallhex.cpp \
 base/bootloaderinstallipod.cpp \
 base/bootloaderinstallsansa.cpp \
 base/bootloaderinstallfile.cpp \
 base/bootloaderinstallchinachip.cpp \
 base/bootloaderinstallams.cpp \
 base/bootloaderinstalltcc.cpp \
 ../../tools/mkboot.c \
 ../../tools/iriver.c \

HEADERS += rbutilqt.h \
 installwindow.h \
 base/httpget.h \
 configure.h \
 zip/zip.h \
 zip/unzip.h \
 zip/zipentry_p.h \
 zip/unzip_p.h \
 zip/zip_p.h \
 version.h \
 base/zipinstaller.h \
 installtalkwindow.h \
 base/talkfile.h \
 base/talkgenerator.h \
 base/autodetection.h \
 base/progressloggerinterface.h \
 progressloggergui.h \
 ../ipodpatcher/ipodpatcher.h \
 ../ipodpatcher/ipodio.h \
 ../ipodpatcher/parttypes.h \
 ../sansapatcher/sansapatcher.h \
 ../sansapatcher/sansaio.h \
 ../chinachippatcher/chinachip.h \
 irivertools/h100sums.h \ 
 irivertools/h120sums.h \
 irivertools/h300sums.h \
 browsedirtree.h \
 themesinstallwindow.h \
 base/uninstall.h \
 uninstallwindow.h \
 base/utils.h \
 preview.h \
 base/encoders.h \
 encttscfggui.h \
 base/encttssettings.h \
 base/ttsbase.h \
 base/ttsexes.h \
 base/ttsfestival.h \
 base/ttssapi.h \
 ../../tools/wavtrim.h \
 ../../tools/voicefont.h \
 base/voicefile.h \
 createvoicewindow.h \
 base/rbsettings.h \
 base/serverinfo.h \
 base/systeminfo.h \
 base/rbunzip.h \
 base/rbzip.h \
 sysinfo.h \
 base/system.h \
 systrace.h \
 base/bootloaderinstallbase.h \
 base/bootloaderinstallmi4.h \
 base/bootloaderinstallhex.h \
 base/bootloaderinstallipod.h \
 base/bootloaderinstallsansa.h \
 base/bootloaderinstallfile.h \
 base/bootloaderinstallchinachip.h \
 base/bootloaderinstallams.h \
 base/bootloaderinstalltcc.h \
 ../../tools/mkboot.h \
 ../../tools/iriver.h \
 
# Needed by QT on Win
INCLUDEPATH = $$_PRO_FILE_PWD_ $$_PRO_FILE_PWD_/irivertools $$_PRO_FILE_PWD_/zip $$_PRO_FILE_PWD_/zlib $$_PRO_FILE_PWD_/base
INCLUDEPATH += $$RBBASE_DIR/rbutil/ipodpatcher $$RBBASE_DIR/rbutil/sansapatcher $$RBBASE_DIR/tools/rbspeex $$RBBASE_DIR/tools

DEPENDPATH = $$INCLUDEPATH

LIBS += -L$$OUT_PWD -L$$MYBUILDDIR -lrbspeex -lmkamsboot -lmktccboot -lucl

# check for system speex. Add a custom rule for pre-building librbspeex if not
# found. Newer versions of speex are split up into libspeex and libspeexdsp,
# and some distributions package them separately. Check for both and fall back
# to librbspeex if not found.
# NOTE: keep this after -lrbspeex, otherwise linker errors occur if the linker
# defaults to --as-needed (see http://www.gentoo.org/proj/en/qa/asneeded.xml)
LIBSPEEX = $$system(pkg-config --silence-errors --libs speex speexdsp)
!static:!isEmpty(LIBSPEEX) {
    LIBS += $$LIBSPEEX
}

TEMPLATE = app
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

TARGET = RockboxUtility

FORMS += rbutilqtfrm.ui \
 aboutbox.ui \
 installwindowfrm.ui \
 progressloggerfrm.ui \
 configurefrm.ui \
 browsedirtreefrm.ui \
 installtalkfrm.ui \
 themesinstallfrm.ui \
 uninstallfrm.ui \
 previewfrm.ui \
 createvoicefrm.ui \
 sysinfofrm.ui \
 systracefrm.ui

RESOURCES += $$_PRO_FILE_PWD_/rbutilqt.qrc
win32 {
    RESOURCES += $$_PRO_FILE_PWD_/rbutilqt-win.qrc
}
!dbg {
    RESOURCES += $$_PRO_FILE_PWD_/rbutilqt-lang.qrc
}

TRANSLATIONS += lang/rbutil_cs.ts \
 lang/rbutil_de.ts \
 lang/rbutil_fi.ts \
 lang/rbutil_fr.ts \
 lang/rbutil_gr.ts \
 lang/rbutil_he.ts \
 lang/rbutil_it.ts \
 lang/rbutil_ja.ts \
 lang/rbutil_nl.ts \
 lang/rbutil_pl.ts \
 lang/rbutil_pt.ts \
 lang/rbutil_pt_BR.ts \
 lang/rbutil_ru.ts \
 lang/rbutil_tr.ts \
 lang/rbutil_zh_CN.ts \
 lang/rbutil_zh_TW.ts \


QT += network
DEFINES += RBUTIL _LARGEFILE64_SOURCE

win32 {
    SOURCES +=  ../ipodpatcher/ipodio-win32.c
    SOURCES +=  ../ipodpatcher/ipodio-win32-scsi.c
    SOURCES +=  ../sansapatcher/sansaio-win32.c
    RC_FILE = rbutilqt.rc
    LIBS += -lsetupapi -lnetapi32
}

unix {
    SOURCES +=  ../ipodpatcher/ipodio-posix.c
    SOURCES +=  ../sansapatcher/sansaio-posix.c
}
unix:!static:!libusb1 {
    LIBS += -lusb
}
unix:!static:libusb1 {
    DEFINES += LIBUSB1
    LIBS += -lusb-1.0
}

unix:static {
    # force statically linking of libusb. Libraries that are appended
    # later will get linked dynamically again.
    LIBS += -Wl,-Bstatic -lusb -Wl,-Bdynamic
}

macx {
    SOURCES += base/ttscarbon.cpp
    HEADERS += base/ttscarbon.h
    QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
    QMAKE_LFLAGS_PPC=-mmacosx-version-min=10.4 -arch ppc
    QMAKE_LFLAGS_X86=-mmacosx-version-min=10.4 -arch i386
    CONFIG+=x86 ppc
    LIBS += -L/usr/local/lib -framework IOKit -framework CoreFoundation -framework Carbon -lz
    INCLUDEPATH += /usr/local/include
    QMAKE_INFO_PLIST = Info.plist
    RC_FILE = icons/rbutilqt.icns
    
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



