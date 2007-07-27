SOURCES += rbutilqt.cpp \
           main.cpp \
 install.cpp \
 httpget.cpp \
 configure.cpp \
 zip/zip.cpp \
 zip/unzip.cpp \
 installzip.cpp
 
HEADERS += rbutilqt.h \
 settings.h \
 install.h \
 httpget.h \
 configure.h \
 zip/zip.h \
 zip/unzip.h \
 zip/zipentry_p.h \
 zip/unzip_p.h \
 zip/zip_p.h \
 version.h \
 installzip.h

TEMPLATE = app
CONFIG += release \
          warn_on \
	  thread \
          qt
TARGET = rbutilqt
FORMS += rbutilqtfrm.ui \
 aboutbox.ui \
 installfrm.ui \
 installprogressfrm.ui \
 configurefrm.ui
RESOURCES += rbutilqt.qrc

TRANSLATIONS += rbutil_de.ts
QT += network
