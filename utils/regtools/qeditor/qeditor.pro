QT += widgets

HEADERS += mainwindow.h backend.h regtab.h analyser.h settings.h std_analysers.h
SOURCES += main.cpp mainwindow.cpp regtab.cpp backend.cpp analyser.cpp std_analysers.cpp settings.cpp 
LIBS += -L../lib/ -lsocdesc -lxml2
INCLUDEPATH += ../lib/ ../../hwstub/lib

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
