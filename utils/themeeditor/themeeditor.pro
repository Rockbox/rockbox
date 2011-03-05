# Setting the binary name
TARGET = rbthemeeditor
VERSION = 0.5
CONFIG(debug) { 
    REVISION = $$system(svnversion)
    VERSION = $$join(VERSION,,,r)
    VERSION = $$join(VERSION,,,$$REVISION)
}

# Adding network support
QT += network

# Enabling profiling
QMAKE_CXXFLAGS_DEBUG += -pg
QMAKE_LFLAGS_DEBUG += -pg

# Adding zlib dependency for QuaZip
LIBS += -lz
INCLUDEPATH += zlib

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
INCLUDEPATH += quazip
INCLUDEPATH += qtfindreplacedialog
DEFINES += FINDREPLACE_NOLIB
cross { 
    message("Crossbuilding for W32 binary")
    
    # retrieve ar binary for w32 cross compile. This might be specific to
    # Fedora mingw32 packages of Qt. Using member() here is needed because at
    # least the F13 packages add ar options to the variable.
    CROSSOPTIONS += AR=$$member(QMAKE_LIB) TARGETPLATFORM=\"MinGW\"
    
    # make sure we use the correct subsystem to prevent a console window coming up.
    LIBS += -Wl,-subsystem,windows
}

# Stuff for the parse lib
libskin_parser.commands = @$(MAKE) \
    TARGET_DIR=$$MYBUILDDIR \
    CC=\"$$QMAKE_CC\" \
    $$CROSSOPTIONS \
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
    graphics/rbtext.h \
    graphics/rbfontcache.h \
    graphics/rbtextcache.h \
    gui/skintimer.h \
    graphics/rbtoucharea.h \
    gui/newprojectdialog.h \
    models/targetdata.h \
    quazip/zip.h \
    quazip/unzip.h \
    quazip/quazipnewinfo.h \
    quazip/quazipfileinfo.h \
    quazip/quazipfile.h \
    quazip/quazip.h \
    quazip/ioapi.h \
    quazip/crypt.h \
    zlib/zlib.h \
    zlib/zconf.h \
    gui/fontdownloader.h \
    qtfindreplacedialog/varianteditor.h \
    qtfindreplacedialog/findreplace_global.h \
    qtfindreplacedialog/findreplaceform.h \
    qtfindreplacedialog/findreplacedialog.h \
    qtfindreplacedialog/findform.h \
    qtfindreplacedialog/finddialog.h \
    gui/projectexporter.h \
    gui/targetdownloader.h \
    gui/syntaxcompleter.h \
    graphics/rbmovable.h \
    graphics/rbscene.h \
    gui/rbconsole.h
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
    graphics/rbtext.cpp \
    graphics/rbfontcache.cpp \
    graphics/rbtextcache.cpp \
    gui/skintimer.cpp \
    graphics/rbtoucharea.cpp \
    gui/newprojectdialog.cpp \
    models/targetdata.cpp \
    quazip/zip.c \
    quazip/unzip.c \
    quazip/quazipnewinfo.cpp \
    quazip/quazipfile.cpp \
    quazip/quazip.cpp \
    quazip/ioapi.c \
    gui/fontdownloader.cpp \
    qtfindreplacedialog/varianteditor.cpp \
    qtfindreplacedialog/findreplaceform.cpp \
    qtfindreplacedialog/findreplacedialog.cpp \
    qtfindreplacedialog/findform.cpp \
    qtfindreplacedialog/finddialog.cpp \
    gui/projectexporter.cpp \
    gui/targetdownloader.cpp \
    gui/syntaxcompleter.cpp \
    graphics/rbmovable.cpp \
    graphics/rbscene.cpp \
    gui/rbconsole.cpp
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
    resources/cursor.png \
    resources/targetdb \
    quazip/README.ROCKBOX \
    quazip/LICENSE.GPL \
    qtfindreplacedialog/dialogs.pro \
    resources/tagdb \
    resources/document-save-as.png \
    resources/edit-undo.png \
    resources/edit-redo.png \
    resources/edit-paste.png \
    resources/edit-cut.png \
    resources/edit-copy.png \
    resources/edit-find-replace.png \
    resources/applications-system.png
FORMS += gui/editorwindow.ui \
    gui/preferencesdialog.ui \
    gui/configdocument.ui \
    gui/skinviewer.ui \
    gui/skintimer.ui \
    gui/newprojectdialog.ui \
    gui/fontdownloader.ui \
    qtfindreplacedialog/findreplaceform.ui \
    qtfindreplacedialog/findreplacedialog.ui \
    gui/projectexporter.ui \
    gui/targetdownloader.ui \
    gui/rbconsole.ui
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
