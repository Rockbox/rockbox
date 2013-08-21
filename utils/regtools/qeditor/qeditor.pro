QT += widgets

HEADERS += mainwindow.h backend.h regtab.h analyser.h settings.h std_analysers.h
SOURCES += main.cpp mainwindow.cpp regtab.cpp backend.cpp analyser.cpp std_analysers.cpp settings.cpp 
LIBS += -L../lib/ -lsocdesc -lxml2
INCLUDEPATH += ../lib/

CONFIG += debug