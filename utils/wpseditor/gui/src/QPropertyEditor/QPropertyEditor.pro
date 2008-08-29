TEMPLATE = lib
CONFIG += staticlib debug_and_release
SOURCES = ColorCombo.cpp \
 Property.cpp \
 QPropertyEditorWidget.cpp \
 QPropertyModel.cpp \
 QVariantDelegate.cpp
HEADERS = ColorCombo.h \
 Property.h \
 QPropertyEditorWidget.h \
 QPropertyModel.h \
 QVariantDelegate.h
INCLUDEPATH += .
DESTDIR = ../../lib
UI_DIR = .
CONFIG(debug, debug|release) {
 TARGET =  QPropertyEditord
 OBJECTS_DIR =  ../../build/QPropertyEditor/debug
 MOC_DIR =  ../../build/QPropertyEditor/debug
}
CONFIG(release, debug|release) {
 TARGET =  QPropertyEditor
 OBJECTS_DIR =  ../../build/QPropertyEditor/release
 MOC_DIR =  ../../build/QPropertyEditor/release
 DEFINES +=  QT_NO_DEBUG
}
