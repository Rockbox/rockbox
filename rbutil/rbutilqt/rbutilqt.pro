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
 httpget.cpp \
 configure.cpp \
 zip/zip.cpp \
 zip/unzip.cpp \
 installzip.cpp \
 installbootloader.cpp \
 progressloggergui.cpp \
 installtalkwindow.cpp \
 talkfile.cpp \
 autodetection.cpp \
 ../ipodpatcher/ipodpatcher.c \
 ../sansapatcher/sansapatcher.c \
 irivertools/irivertools.cpp \
 irivertools/md5sum.cpp  \
 browsedirtree.cpp \
 installthemes.cpp \
 uninstall.cpp \
 uninstallwindow.cpp \
 utils.cpp \
 browseof.cpp \
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
 rbunzip.cpp \
 rbzip.cpp \
 sysinfo.cpp \
 detect.cpp
 
HEADERS += rbutilqt.h \
 install.h \
 httpget.h \
 configure.h \
 zip/zip.h \
 zip/unzip.h \
 zip/zipentry_p.h \
 zip/unzip_p.h \
 zip/zip_p.h \
 version.h \
 installzip.h \
 installbootloader.h \
 installtalkwindow.h \
 talkfile.h \
 autodetection.h \
 progressloggerinterface.h \
 progressloggergui.h \
 ../ipodpatcher/ipodpatcher.h \
 ../ipodpatcher/ipodio.h \
 ../ipodpatcher/parttypes.h \
 ../sansapatcher/sansapatcher.h \
 ../sansapatcher/sansaio.h \
 irivertools/irivertools.h \
 irivertools/md5sum.h \
 irivertools/h100sums.h \ 
 irivertools/h120sums.h \
 irivertools/h300sums.h \
 irivertools/checksums.h \
 browsedirtree.h \
 installthemes.h \
 uninstall.h \
 uninstallwindow.h \
 utils.h \
 browseof.h \
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
 rbunzip.h \
 rbzip.h \
 sysinfo.h \
 detect.h
 
# Needed by QT on Win
INCLUDEPATH = . irivertools zip zlib ../ipodpatcher ../sansapatcher ../../tools/rbspeex ../../tools
 
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
 browseoffrm.ui \
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

TRANSLATIONS += rbutil_de.ts \
 rbutil_fi.ts \
 rbutil_fr.ts \
 rbutil_nl.ts \
 rbutil_tr.ts \
 rbutil_zh_CN.ts \
 rbutil_zh_TW.ts
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

