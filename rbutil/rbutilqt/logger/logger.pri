
SOURCES += \
 $$PWD/AbstractAppender.cpp \
 $$PWD/AbstractStringAppender.cpp \
 $$PWD/ConsoleAppender.cpp \
 $$PWD/FileAppender.cpp \
 $$PWD/Logger.cpp \

INCLUDES += \
 $$PWD/AbstractAppender.h \
 $$PWD/ConsoleAppender.h \
 $$PWD/FileAppender.h \
 $$PWD/OutputDebugAppender.h \
 $$PWD/AbstractStringAppender.h \
 $$PWD/CuteLogger_global.h \
 $$PWD/Logger.h \

DEFINES += \
 CUTELOGGER_STATIC
