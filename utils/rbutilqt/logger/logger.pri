
SOURCES += \
 $$PWD/src/AbstractAppender.cpp \
 $$PWD/src/AbstractStringAppender.cpp \
 $$PWD/src/ConsoleAppender.cpp \
 $$PWD/src/FileAppender.cpp \
 $$PWD/src/Logger.cpp \

HEADERS += \
 $$PWD/include/AbstractAppender.h \
 $$PWD/include/ConsoleAppender.h \
 $$PWD/include/FileAppender.h \
 $$PWD/include/OutputDebugAppender.h \
 $$PWD/include/AbstractStringAppender.h \
 $$PWD/include/CuteLogger_global.h \
 $$PWD/include/Logger.h \

INCLUDEPATH += $$PWD/include

DEFINES += \
 CUTELOGGER_STATIC

