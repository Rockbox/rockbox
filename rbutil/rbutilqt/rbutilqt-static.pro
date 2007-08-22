# project file for building static binary

include(rbutilqt.pro)

QTPLUGIN += qtaccessiblewidgets
LIBS += -L$$(QT_BUILD_TREE)/plugins/accessible -lqtaccessiblewidgets
DEFINES += STATIC

