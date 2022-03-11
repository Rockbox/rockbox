#
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#

add_executable(RockboxThemeEditor WIN32 MACOSX_BUNDLE
    graphics/rbalbumart.cpp
    graphics/rbalbumart.h
    graphics/rbfont.cpp
    graphics/rbfont.h
    graphics/rbfontcache.cpp
    graphics/rbfontcache.h
    graphics/rbimage.cpp
    graphics/rbimage.h
    graphics/rbmovable.cpp
    graphics/rbmovable.h
    graphics/rbprogressbar.cpp
    graphics/rbprogressbar.h
    graphics/rbrenderinfo.cpp
    graphics/rbrenderinfo.h
    graphics/rbscene.cpp
    graphics/rbscene.h
    graphics/rbscreen.cpp
    graphics/rbscreen.h
    graphics/rbtext.cpp
    graphics/rbtext.h
    graphics/rbtextcache.cpp
    graphics/rbtextcache.h
    graphics/rbtoucharea.cpp
    graphics/rbtoucharea.h
    graphics/rbviewport.cpp
    graphics/rbviewport.h
    gui/codeeditor.cpp
    gui/codeeditor.h
    gui/configdocument.cpp
    gui/configdocument.h
    gui/configdocument.ui
    gui/devicestate.cpp
    gui/devicestate.h
    gui/editorwindow.cpp
    gui/editorwindow.h
    gui/editorwindow.ui
    gui/fontdownloader.cpp
    gui/fontdownloader.h
    gui/fontdownloader.ui
    gui/newprojectdialog.cpp
    gui/newprojectdialog.h
    gui/newprojectdialog.ui
    gui/preferencesdialog.cpp
    gui/preferencesdialog.h
    gui/preferencesdialog.ui
    gui/projectexporter.cpp
    gui/projectexporter.h
    gui/projectexporter.ui
    gui/rbconsole.cpp
    gui/rbconsole.h
    gui/rbconsole.ui
    gui/skindocument.cpp
    gui/skindocument.h
    gui/skinhighlighter.cpp
    gui/skinhighlighter.h
    gui/skintimer.cpp
    gui/skintimer.h
    gui/skintimer.ui
    gui/skinviewer.cpp
    gui/skinviewer.h
    gui/skinviewer.ui
    gui/syntaxcompleter.cpp
    gui/syntaxcompleter.h
    gui/tabcontent.h
    gui/targetdownloader.cpp
    gui/targetdownloader.h
    gui/targetdownloader.ui
    main.cpp
    models/parsetreemodel.cpp
    models/parsetreemodel.h
    models/parsetreenode.cpp
    models/parsetreenode.h
    models/projectmodel.cpp
    models/projectmodel.h
    models/targetdata.cpp
    models/targetdata.h
    qtfindreplacedialog/finddialog.cpp
    qtfindreplacedialog/finddialog.h
    qtfindreplacedialog/findform.cpp
    qtfindreplacedialog/findform.h
    qtfindreplacedialog/findreplace_global.h
    qtfindreplacedialog/findreplacedialog.cpp
    qtfindreplacedialog/findreplacedialog.h
    qtfindreplacedialog/findreplacedialog.ui
    qtfindreplacedialog/findreplaceform.cpp
    qtfindreplacedialog/findreplaceform.h
    qtfindreplacedialog/findreplaceform.ui
    qtfindreplacedialog/varianteditor.cpp
    qtfindreplacedialog/varianteditor.h
    quazip/crypt.h
    quazip/ioapi.c
    quazip/ioapi.h
    quazip/quazip.cpp
    quazip/quazip.h
    quazip/quazipfile.cpp
    quazip/quazipfile.h
    quazip/quazipfileinfo.h
    quazip/quazipnewinfo.cpp
    quazip/quazipnewinfo.h
    quazip/unzip.c
    quazip/unzip.h
    quazip/zip.c
    quazip/zip.h
    zlib/zconf.h
    zlib/zlib.h
    resources.qrc
    themeeditor.rc
    )
set_target_properties(RockboxThemeEditor PROPERTIES AUTOMOC ON)
set_target_properties(RockboxThemeEditor PROPERTIES AUTORCC ON)
set_target_properties(RockboxThemeEditor PROPERTIES AUTOUIC ON)

target_link_libraries(RockboxThemeEditor
    Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Core
    skin_parser z)
target_include_directories(RockboxThemeEditor PRIVATE models graphics gui qtfindreplacedialog quazip)
target_compile_definitions(RockboxThemeEditor PRIVATE FINDREPLACE_NOLIB)

if(APPLE)
    # MacOS Application Bundle specifics.
    target_sources(RockboxThemeEditor PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/resources/windowicon.icns)
    set_target_properties(RockboxThemeEditor
        PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/Info.plist)
    set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/resources/windowicon.icns
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

deploy_qt(TARGET RockboxThemeEditor
    QTBINDIR ${QT_BINDIR}
    ICONFILE ${CMAKE_CURRENT_LIST_DIR}/resources/rbthemeeditor.svg
    DESKTOPFILE ${CMAKE_CURRENT_LIST_DIR}/RockboxThemeEditor.desktop
    DMGBUILDCFG ${CMAKE_CURRENT_LIST_DIR}/dmgbuild.cfg)

