QT += widgets

HEADERS += mainwindow.h backend.h regtab.h analyser.h settings.h \
    std_analysers.h utils.h regdisplaypanel.h regedit.h
SOURCES += main.cpp mainwindow.cpp regtab.cpp backend.cpp analyser.cpp \
    std_analysers.cpp settings.cpp utils.cpp regdisplaypanel.cpp regedit.cpp
LIBS += -L../lib/ -lsocdesc -lxml2
INCLUDEPATH += ../include/ ../../hwstub/include
DEPENDPATH += ../

greaterThan(QT_MAJOR_VERSION, 4) {
    # qt5 knows c++11
    CONFIG += c++11
}
else {
    message("Qt4 is deprecated, you should use Qt5")
    # qt4 does not, use gcc specific code
    QMAKE_CXXFLAGS += -std=c++11
}

libsocdesc.commands = cd ../lib && make
QMAKE_EXTRA_TARGETS += libsocdesc
PRE_TARGETDEPS += libsocdesc

VERSION = 3.0.0

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
