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
 preview.cpp 

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
 preview.h 
 
# Needed by QT on Win
INCLUDEPATH = . irivertools zip zlib ../ipodpatcher ../sansapatcher
 
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
 installprogressfrm.ui \
 configurefrm.ui \
 browsedirtreefrm.ui \
 installtalkfrm.ui \
 installthemesfrm.ui \
 uninstallfrm.ui \
 browseoffrm.ui \
 previewfrm.ui

RESOURCES += rbutilqt.qrc

TRANSLATIONS += rbutil_de.ts \
 rbutil_fr.ts \
 rbutil_zh_CN.ts \
 rbutil_zh_TW.ts
QT += network
DEFINES += RBUTIL _LARGEFILE64_SOURCE

win32 {
    SOURCES +=  ../ipodpatcher/ipodio-win32.c
    SOURCES +=  ../sansapatcher/sansaio-win32.c
    RC_FILE = rbutilqt.rc
    LIBS += -lsetupapi
}

unix {
    SOURCES +=  ../ipodpatcher/ipodio-posix.c
    SOURCES +=  ../sansapatcher/sansaio-posix.c
    LIBS += -lusb
}

macx {
    QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
    CONFIG+=x86 ppc
    LIBS += -L/usr/local/lib -framework IOKit
    INCLUDEPATH += /usr/local/include
}

static {
    QTPLUGIN += qtaccessiblewidgets
    LIBS += -L$$(QT_BUILD_TREE)/plugins/accessible -lqtaccessiblewidgets
    DEFINES += STATIC
    message("using static plugin")
}

