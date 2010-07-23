# -------------------------------------------------
# Project created by QtCreator 2009-11-07T11:39:43
# -------------------------------------------------
TARGET = qtfindreplacedialog
TEMPLATE = lib
DEFINES += FINDREPLACE_LIBRARY
SOURCES += findform.cpp \
    finddialog.cpp \
    findreplaceform.cpp \
    findreplacedialog.cpp \
    varianteditor.cpp
HEADERS += findreplaceform.h \
    findreplacedialog.h \
    findform.h \
    finddialog.h \
    findreplace_global.h \
    varianteditor.h
FORMS += findreplaceform.ui \
    findreplacedialog.ui
DESTDIR = ../lib
DEPENDPATH += .
target.path = /lib
headers.files = $$HEADERS
headers.path = /include
INSTALLS += target \
    headers
