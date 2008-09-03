TEMPLATE = app
TARGET =
DEPENDPATH += . build src ui
INCLUDEPATH += . src/QPropertyEditor ../libwps/src
DESTDIR = bin
OBJECTS_DIR = build
MOC_DIR = build
UI_DIR = build
QMAKE_LIBDIR += lib
QT = gui core
CONFIG += qt warn_on debug
HEADERS += ../libwps/src/api.h \
 ../libwps/src/defs.h \
 src/slider.h \
 src/qtrackstate.h \
 src/qwpsstate.h \
 src/qwpseditorwindow.h \
 src/utils.h \
 src/qwpsdrawer.h \
 src/qsyntaxer.h
FORMS += ui/mainwindow.ui ui/slider.ui
SOURCES += src/main.cpp \
 src/slider.cpp \
 src/qtrackstate.cpp \
 src/qwpsstate.cpp \
 src/qwpseditorwindow.cpp \
 src/utils.cpp \
 src/qwpsdrawer.cpp \
 src/qwpsdrawer_static.cpp \
 src/qsyntaxer.cpp
LIBS += -Lbin
CONFIG(debug, debug|release) {
 LIBS +=  -lQPropertyEditord
 TARGET =  wpseditord
 CONFIG += console
}
CONFIG(release, debug|release) {
 LIBS +=  -lQPropertyEditor
 TARGET =  wpseditor
}
