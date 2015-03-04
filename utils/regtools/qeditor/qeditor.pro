QT += widgets

HEADERS += mainwindow.h backend.h regtab.h analyser.h settings.h \
    std_analysers.h utils.h regdisplaypanel.h regedit.h
SOURCES += main.cpp mainwindow.cpp regtab.cpp backend.cpp analyser.cpp \
    std_analysers.cpp settings.cpp utils.cpp regdisplaypanel.cpp regedit.cpp
LIBS += -L../lib/ -lsocdesc -lxml2
INCLUDEPATH += ../lib/ ../../hwstub/lib
DEPENDPATH += ../

libsocdesc.commands = cd ../lib && make
QMAKE_EXTRA_TARGETS += libsocdesc
PRE_TARGETDEPS += libsocdesc

VERSION = 2.0.3

DEFINES += APP_VERSION=\\\"$$VERSION\\\"

unix {
    !nohwstub {
        message("Use 'qmake -config nohwstub' if you want to disable hwstub support")
        libhwstub.commands = cd ../../hwstub/lib && make
        QMAKE_EXTRA_TARGETS += libhwstub
        PRE_TARGETDEPS += libhwstub

        LIBS += -L../../hwstub/lib -lhwstub
        DEPENDPATH += ../../hwstub
        DEFINES += HAVE_HWSTUB
        CONFIG += link_pkgconfig
        PKGCONFIG += libusb-1.0
    }
}

CONFIG += debug
