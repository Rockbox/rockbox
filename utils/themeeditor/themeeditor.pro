# build in a separate folder.
MYBUILDDIR = $$OUT_PWD/build/
OBJECTS_DIR = $$MYBUILDDIR/o
UI_DIR = $$MYBUILDDIR/ui
MOC_DIR = $$MYBUILDDIR/moc
RCC_DIR = $$MYBUILDDIR/rcc

#Include directories
INCLUDEPATH += gui
INCLUDEPATH += parser
INCLUDEPATH += models

HEADERS += parser/tag_table.h \
    parser/symbols.h \
    parser/skin_parser.h \
    parser/skin_scan.h \
    parser/skin_debug.h \
    models/parsetreemodel.h \
    models/parsetreenode.h \
    gui/editorwindow.h \
    gui/skinhighlighter.h \
    gui/skindocument.h \
    gui/preferencesdialog.h \
    gui/codeeditor.h \
    models/projectmodel.h \
    gui/tabcontent.h \
    gui/configdocument.h \
    gui/skinviewer.h
SOURCES += parser/tag_table.c \
    parser/skin_parser.c \
    parser/skin_scan.c \
    parser/skin_debug.c \
    main.cpp \
    models/parsetreemodel.cpp \
    models/parsetreenode.cpp \
    gui/editorwindow.cpp \
    gui/skinhighlighter.cpp \
    gui/skindocument.cpp \
    gui/preferencesdialog.cpp \
    gui/codeeditor.cpp \
    models/projectmodel.cpp \
    gui/configdocument.cpp \
    gui/skinviewer.cpp
OTHER_FILES += README \
    resources/windowicon.png \
    resources/appicon.xcf \
    resources/COPYING \
    resources/document-save.png \
    resources/document-open.png \
    resources/document-new.png
FORMS += gui/editorwindow.ui \
    gui/preferencesdialog.ui \
    gui/configdocument.ui \
    gui/skinviewer.ui
RESOURCES += resources.qrc
