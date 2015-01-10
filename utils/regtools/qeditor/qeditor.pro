QT += widgets

HEADERS += mainwindow.h backend.h regtab.h analyser.h settings.h \
    std_analysers.h utils.h regdisplaypanel.h
SOURCES += main.cpp mainwindow.cpp regtab.cpp backend.cpp analyser.cpp \
    std_analysers.cpp settings.cpp utils.cpp regdisplaypanel.cpp
LIBS += -L../lib/ -lsocdesc -lxml2
INCLUDEPATH += ../include/ ../../hwstub/lib

VERSION = 2.1.0

DEFINES += APP_VERSION=\\\"$$VERSION\\\"

unix {
    !nohwstub {
        message("Use 'qmake -config nohwstub' if you want to disable hwstub support")
        LIBS += -L../../hwstub/lib -lhwstub
        DEFINES += HAVE_HWSTUB
        CONFIG += link_pkgconfig
        PKGCONFIG += libusb-1.0
    }
}

CONFIG += debug
