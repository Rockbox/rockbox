unix:!mac {
    CCACHE = $$system(which ccache)
    !isEmpty(CCACHE) {
        message("using ccache")
        QMAKE_CXX = ccache g++
        QMAKE_CC = ccache gcc
    }
}

OBJECTS_DIR = build/o
UI_DIR = build/ui
MOC_DIR = build/moc
RCC_DIR = build/rcc

# check version of Qt installation
VER = $$find(QT_VERSION, ^4\.[3-9]+.*)
isEmpty(VER) {
    !isEmpty(QT_VERSION) error("Qt found:" $$[QT_VERSION])
    error("Qt >= 4.3 required!")
}
message("Qt version used:" $$VER)

# add a custom rule for pre-building librbspeex
!mac {
rbspeex.commands = @$(MAKE) -C ../../tools/rbspeex librbspeex.a
}
mac {
rbspeex.commands = @$(MAKE) -C ../../tools/rbspeex librbspeex-universal
}
QMAKE_EXTRA_TARGETS += rbspeex
PRE_TARGETDEPS += rbspeex

# rule for creating ctags file
tags.commands = ctags -R --c++-kinds=+p --fields=+iaS --extra=+q $(SOURCES)
tags.depends = $(SOURCES)
QMAKE_EXTRA_TARGETS += tags

# add a custom rule for making the translations
lrelease.commands = $$[QT_INSTALL_BINS]/lrelease -silent rbutilqt.pro
QMAKE_EXTRA_TARGETS += lrelease
!dbg {
    PRE_TARGETDEPS += lrelease
}


SOURCES += rbutilqt.cpp \
           main.cpp \
 install.cpp \
 base/httpget.cpp \
 configure.cpp \
 zip/zip.cpp \
 zip/unzip.cpp \
 installzip.cpp \
 progressloggergui.cpp \
 installtalkwindow.cpp \
 talkfile.cpp \
 base/autodetection.cpp \
 ../ipodpatcher/ipodpatcher.c \
 ../sansapatcher/sansapatcher.c \
 browsedirtree.cpp \
 installthemes.cpp \
 base/uninstall.cpp \
 uninstallwindow.cpp \
 base/utils.cpp \
 preview.cpp \
 encoders.cpp \
 encodersgui.cpp \
 tts.cpp \
 ttsgui.cpp \
 ../../tools/wavtrim.c \
 ../../tools/voicefont.c \
 voicefile.cpp \
 createvoicewindow.cpp \
 rbsettings.cpp \
 base/rbunzip.cpp \
 base/rbzip.cpp \
 base/detect.cpp \
 sysinfo.cpp \
 base/bootloaderinstallbase.cpp \
 base/bootloaderinstallmi4.cpp \
 base/bootloaderinstallhex.cpp \
 base/bootloaderinstallipod.cpp \
 base/bootloaderinstallsansa.cpp \
 base/bootloaderinstallfile.cpp \
 ../../tools/mkboot.c \
 ../../tools/iriver.c

HEADERS += rbutilqt.h \
 install.h \
 base/httpget.h \
 configure.h \
 zip/zip.h \
 zip/unzip.h \
 zip/zipentry_p.h \
 zip/unzip_p.h \
 zip/zip_p.h \
 version.h \
 installzip.h \
 installtalkwindow.h \
 talkfile.h \
 base/autodetection.h \
 progressloggerinterface.h \
 progressloggergui.h \
 ../ipodpatcher/ipodpatcher.h \
 ../ipodpatcher/ipodio.h \
 ../ipodpatcher/parttypes.h \
 ../sansapatcher/sansapatcher.h \
 ../sansapatcher/sansaio.h \
 irivertools/h100sums.h \ 
 irivertools/h120sums.h \
 irivertools/h300sums.h \
 browsedirtree.h \
 installthemes.h \
 base/uninstall.h \
 uninstallwindow.h \
 base/utils.h \
 preview.h \
 encoders.h \
 encodersgui.h \
 tts.h \
 ttsgui.h \
 ../../tools/wavtrim.h \
 ../../tools/voicefont.h \
 voicefile.h \
 createvoicewindow.h \
 rbsettings.h \
 base/rbunzip.h \
 base/rbzip.h \
 sysinfo.h \
 base/detect.h \
 base/bootloaderinstallbase.h \
 base/bootloaderinstallmi4.h \
 base/bootloaderinstallhex.h \
 base/bootloaderinstallipod.h \
 base/bootloaderinstallsansa.h \
 base/bootloaderinstallfile.h \
 ../../tools/mkboot.h \
 ../../tools/iriver.h

# Needed by QT on Win
INCLUDEPATH = . irivertools zip zlib ../ipodpatcher ../sansapatcher ../../tools/rbspeex ../../tools
INCLUDEPATH += base

LIBS += -L../../tools/rbspeex -lrbspeex

TEMPLATE = app
dbg {
    CONFIG += debug thread qt warn_on
    DEFINES -= QT_NO_DEBUG_OUTPUT
    message("debug")
}
!dbg {
    CONFIG += release thread qt
    DEFINES += QT_NO_DEBUG_OUTPUT
    message("release")
}

TARGET = rbutilqt

FORMS += rbutilqtfrm.ui \
 aboutbox.ui \
 installfrm.ui \
 progressloggerfrm.ui \
 configurefrm.ui \
 browsedirtreefrm.ui \
 installtalkfrm.ui \
 installthemesfrm.ui \
 uninstallfrm.ui \
 previewfrm.ui \
 rbspeexcfgfrm.ui \
 encexescfgfrm.ui \
 ttsexescfgfrm.ui \
 sapicfgfrm.ui \
 createvoicefrm.ui \
 sysinfofrm.ui

RESOURCES += rbutilqt.qrc
win32 {
    RESOURCES += rbutilqt-win.qrc
}
!dbg {
    RESOURCES += rbutilqt-lang.qrc
}

TRANSLATIONS += lang/rbutil_de.ts \
 lang/rbutil_fi.ts \
 lang/rbutil_fr.ts \
 lang/rbutil_gr.ts \
 lang/rbutil_he.ts \
 lang/rbutil_ja.ts \
 lang/rbutil_nl.ts \
 lang/rbutil_pt.ts \
 lang/rbutil_tr.ts \
 lang/rbutil_zh_CN.ts \
 lang/rbutil_zh_TW.ts \


QT += network
DEFINES += RBUTIL _LARGEFILE64_SOURCE

win32 {
    SOURCES +=  ../ipodpatcher/ipodio-win32.c
    SOURCES +=  ../sansapatcher/sansaio-win32.c
    RC_FILE = rbutilqt.rc
    LIBS += -lsetupapi -lnetapi32
}

unix {
    SOURCES +=  ../ipodpatcher/ipodio-posix.c
    SOURCES +=  ../sansapatcher/sansaio-posix.c
}
unix:!static {
    LIBS += -lusb
}
unix:static {
    # force statically linking of libusb. Libraries that are appended
    # later will get linked dynamically again.
    LIBS += -Wl,-Bstatic -lusb -Wl,-Bdynamic
}

macx {
    QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
    QMAKE_LFLAGS_PPC=-mmacosx-version-min=10.4 -arch ppc
    QMAKE_LFLAGS_X86=-mmacosx-version-min=10.4 -arch i386
    CONFIG+=x86 ppc
    LIBS += -L/usr/local/lib -framework IOKit
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



