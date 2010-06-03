# build in a separate folder.
MYBUILDDIR = $$OUT_PWD/build/
OBJECTS_DIR = $$MYBUILDDIR/o
UI_DIR = $$MYBUILDDIR/ui
MOC_DIR = $$MYBUILDDIR/moc
RCC_DIR = $$MYBUILDDIR/rcc
HEADERS += tag_table.h \
    symbols.h \
    skin_parser.h \
    skin_scan.h \
    skin_debug.h \
    parsetreemodel.h \
    parsetreenode.h \
    editorwindow.h \
    skinhighlighter.h
SOURCES += tag_table.c \
    skin_parser.c \
    skin_scan.c \
    skin_debug.c \
    main.cpp \
    parsetreemodel.cpp \
    parsetreenode.cpp \
    editorwindow.cpp \
    skinhighlighter.cpp
OTHER_FILES += README \
    resources/windowicon.png \
    resources/appicon.xcf
FORMS += editorwindow.ui
RESOURCES += resources.qrc
