# Enabling profiling
QMAKE_CXXFLAGS_DEBUG += -pg
QMAKE_LFLAGS_DEBUG += -pg

# build in a separate folder.
MYBUILDDIR = $$OUT_PWD/build/
OBJECTS_DIR = $$MYBUILDDIR/o
UI_DIR = $$MYBUILDDIR/ui
MOC_DIR = $$MYBUILDDIR/moc
RCC_DIR = $$MYBUILDDIR/rcc
RBBASE_DIR = $$_PRO_FILE_PWD_
RBBASE_DIR = $$replace(RBBASE_DIR,/utils/themeeditor,)

# Include directories
INCLUDEPATH += gui
INCLUDEPATH += models
INCLUDEPATH += graphics

# Stuff for the parse lib
libskin_parser.commands = @$(MAKE) \
    TARGET_DIR=$$MYBUILDDIR \
    CC=\"$$QMAKE_CC\" \
    BUILDDIR=$$OBJECTS_DIR \
    -C \
    $$RBBASE_DIR/lib/skin_parser \
    libskin_parser.a
QMAKE_EXTRA_TARGETS += libskin_parser
PRE_TARGETDEPS += libskin_parser
INCLUDEPATH += $$RBBASE_DIR/lib/skin_parser
LIBS += -L$$MYBUILDDIR \
    -lskin_parser
DEPENDPATH = $$INCLUDEPATH
HEADERS += models/parsetreemodel.h \
    models/parsetreenode.h \
    gui/editorwindow.h \
    gui/skinhighlighter.h \
    gui/skindocument.h \
    gui/preferencesdialog.h \
    gui/codeeditor.h \
    models/projectmodel.h \
    gui/tabcontent.h \
    gui/configdocument.h \
    gui/skinviewer.h \
    graphics/rbscreen.h \
    graphics/rbviewport.h \
    graphics/rbrenderinfo.h \
    graphics/rbimage.h \
    graphics/rbfont.h \
    gui/devicestate.h \
    graphics/rbalbumart.h \
    graphics/rbprogressbar.h \
    gui/findreplacedialog.h \
    graphics/rbtext.h \
    graphics/rbfontcache.h \
    graphics/rbtextcache.h \
    gui/skintimer.h \
    graphics/rbtoucharea.h \
    gui/newprojectdialog.h
SOURCES += main.cpp \
    models/parsetreemodel.cpp \
    models/parsetreenode.cpp \
    gui/editorwindow.cpp \
    gui/skinhighlighter.cpp \
    gui/skindocument.cpp \
    gui/preferencesdialog.cpp \
    gui/codeeditor.cpp \
    models/projectmodel.cpp \
    gui/configdocument.cpp \
    gui/skinviewer.cpp \
    graphics/rbscreen.cpp \
    graphics/rbviewport.cpp \
    graphics/rbrenderinfo.cpp \
    graphics/rbimage.cpp \
    graphics/rbfont.cpp \
    gui/devicestate.cpp \
    graphics/rbalbumart.cpp \
    graphics/rbprogressbar.cpp \
    gui/findreplacedialog.cpp \
    graphics/rbtext.cpp \
    graphics/rbfontcache.cpp \
    graphics/rbtextcache.cpp \
    gui/skintimer.cpp \
    graphics/rbtoucharea.cpp \
    gui/newprojectdialog.cpp
OTHER_FILES += README \
    resources/windowicon.png \
    resources/appicon.xcf \
    resources/COPYING \
    resources/document-save.png \
    resources/document-open.png \
    resources/document-new.png \
    resources/deviceoptions \
    resources/render/statusbar.png \
    resources/render/scenebg.png \
    resources/play.xcf \
    resources/play.png \
    resources/rwnd.png \
    resources/pause.xcf \
    resources/pause.png \
    resources/ffwd.xcf \
    resources/ffwd.png \
    resources/lines.xcf \
    resources/lines.png \
    resources/cursor.xcf \
    resources/cursor.png
FORMS += gui/editorwindow.ui \
    gui/preferencesdialog.ui \
    gui/configdocument.ui \
    gui/skinviewer.ui \
    gui/findreplacedialog.ui \
    gui/skintimer.ui \
    gui/newprojectdialog.ui
RESOURCES += resources.qrc
win32:RC_FILE = themeeditor.rc
macx { 
    QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.4u.sdk
    QMAKE_LFLAGS_PPC = -mmacosx-version-min=10.4 \
        -arch \
        ppc
    QMAKE_LFLAGS_X86 = -mmacosx-version-min=10.4 \
        -arch \
        i386
    CONFIG += x86 \
        ppc
    QMAKE_INFO_PLIST = Info.plist
    RC_FILE = resources/windowicon.icns
}
