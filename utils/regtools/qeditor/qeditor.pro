QT += widgets

HEADERS += mainwindow.h backend.h regtab.h analyser.h settings.h \
    std_analysers.h utils.h regdisplaypanel.h regedit.h
SOURCES += main.cpp mainwindow.cpp regtab.cpp backend.cpp analyser.cpp \
    std_analysers.cpp settings.cpp utils.cpp regdisplaypanel.cpp regedit.cpp
LIBS += -L../lib/ -lsocdesc -lxml2
INCLUDEPATH += ../lib/ ../../hwstub/lib
DEPENDPATH += ../


libsocdesc.target = ../lib/libsocdesc.a
libsocdesc.commands = cd ../lib && make clean && make
QMAKE_EXTRA_TARGETS += libsocdesc
QMAKE_CLEAN += ../lib/libsocdesc.a
PRE_TARGETDEPS += ../lib/libsocdesc.a

VERSION = 2.0.3

DEFINES += APP_VERSION=\\\"$$VERSION\\\"

unix {
    !nohwstub {
        message("Use 'qmake -config nohwstub' if you want to disable hwstub support")
        libhwstub.target = ../../hwstub/lib/libhwstub.a
        libhwstub.commands = cd ../../hwstub/lib && make clean && make
        QMAKE_EXTRA_TARGETS += libhwstub
        QMAKE_CLEAN += ../../hwstub/lib/libhwstub.a
        PRE_TARGETDEPS += ../../hwstub/lib/libhwstub.a
        LIBS += -L../../hwstub/lib -lhwstub
        DEPENDPATH += ../../hwstub
        DEFINES += HAVE_HWSTUB
        CONFIG += link_pkgconfig
        PKGCONFIG += libusb-1.0
    }
}

CONFIG += debug
