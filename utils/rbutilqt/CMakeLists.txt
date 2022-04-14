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

set(RBUTIL_TS_FILES
    ${CMAKE_CURRENT_LIST_DIR}/lang/rbutil_cs.ts
    ${CMAKE_CURRENT_LIST_DIR}/lang/rbutil_de.ts
    ${CMAKE_CURRENT_LIST_DIR}/lang/rbutil_fi.ts
    ${CMAKE_CURRENT_LIST_DIR}/lang/rbutil_fr.ts
    ${CMAKE_CURRENT_LIST_DIR}/lang/rbutil_gr.ts
    ${CMAKE_CURRENT_LIST_DIR}/lang/rbutil_he.ts
    ${CMAKE_CURRENT_LIST_DIR}/lang/rbutil_it.ts
    ${CMAKE_CURRENT_LIST_DIR}/lang/rbutil_ja.ts
    ${CMAKE_CURRENT_LIST_DIR}/lang/rbutil_nl.ts
    ${CMAKE_CURRENT_LIST_DIR}/lang/rbutil_pl.ts
    ${CMAKE_CURRENT_LIST_DIR}/lang/rbutil_pt_BR.ts
    ${CMAKE_CURRENT_LIST_DIR}/lang/rbutil_pt.ts
    ${CMAKE_CURRENT_LIST_DIR}/lang/rbutil_ru.ts
    ${CMAKE_CURRENT_LIST_DIR}/lang/rbutil_tr.ts
    ${CMAKE_CURRENT_LIST_DIR}/lang/rbutil_zh_CN.ts
    ${CMAKE_CURRENT_LIST_DIR}/lang/rbutil_zh_TW.ts
    )
if(QT_VERSION_MAJOR EQUAL 5)
    qt5_add_translation(RBUTIL_QM_FILES ${RBUTIL_TS_FILES})
else()
    qt6_add_translation(RBUTIL_QM_FILES ${RBUTIL_TS_FILES})
endif()

# The qrc file searches input files based on its location, and the qm files are
# generated in the build dir. Copy the qrc file to the build dir, make it
# depend on the qm files. cmake will then get deps right.
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/rbutilqt-lang.qrc
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_LIST_DIR}/lang/rbutilqt-lang.qrc
            ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${RBUTIL_QM_FILES}
    )

add_executable(RockboxUtility WIN32 MACOSX_BUNDLE
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp

    ${CMAKE_CURRENT_LIST_DIR}/aboutbox.ui
    ${CMAKE_CURRENT_LIST_DIR}/configure.cpp
    ${CMAKE_CURRENT_LIST_DIR}/configurefrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/configure.h
    ${CMAKE_CURRENT_LIST_DIR}/createvoicefrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/createvoicewindow.cpp
    ${CMAKE_CURRENT_LIST_DIR}/createvoicewindow.h
    ${CMAKE_CURRENT_LIST_DIR}/encttscfggui.cpp
    ${CMAKE_CURRENT_LIST_DIR}/encttscfggui.h
    ${CMAKE_CURRENT_LIST_DIR}/installtalkfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/installtalkwindow.cpp
    ${CMAKE_CURRENT_LIST_DIR}/installtalkwindow.h
    ${CMAKE_CURRENT_LIST_DIR}/preview.cpp
    ${CMAKE_CURRENT_LIST_DIR}/previewfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/preview.h
    ${CMAKE_CURRENT_LIST_DIR}/progressloggerfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/progressloggergui.cpp
    ${CMAKE_CURRENT_LIST_DIR}/progressloggergui.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqtfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt.h
    ${CMAKE_CURRENT_LIST_DIR}/sysinfo.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sysinfofrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/sysinfo.h
    ${CMAKE_CURRENT_LIST_DIR}/systrace.cpp
    ${CMAKE_CURRENT_LIST_DIR}/systracefrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/systrace.h
    ${CMAKE_CURRENT_LIST_DIR}/themesinstallfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/themesinstallwindow.cpp
    ${CMAKE_CURRENT_LIST_DIR}/themesinstallwindow.h
    ${CMAKE_CURRENT_LIST_DIR}/uninstallfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/uninstallwindow.cpp
    ${CMAKE_CURRENT_LIST_DIR}/uninstallwindow.h
    ${CMAKE_CURRENT_LIST_DIR}/version.h

    ${CMAKE_CURRENT_LIST_DIR}/gui/backupdialog.cpp
    ${CMAKE_CURRENT_LIST_DIR}/gui/backupdialogfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/gui/backupdialog.h
    ${CMAKE_CURRENT_LIST_DIR}/gui/changelog.cpp
    ${CMAKE_CURRENT_LIST_DIR}/gui/changelogfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/gui/changelog.h
    ${CMAKE_CURRENT_LIST_DIR}/gui/comboboxviewdelegate.cpp
    ${CMAKE_CURRENT_LIST_DIR}/gui/comboboxviewdelegate.h
    ${CMAKE_CURRENT_LIST_DIR}/gui/infowidget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/gui/infowidgetfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/gui/infowidget.h
    ${CMAKE_CURRENT_LIST_DIR}/gui/selectiveinstallwidget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/gui/selectiveinstallwidgetfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/gui/selectiveinstallwidget.h

    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt.qrc
    ${CMAKE_CURRENT_BINARY_DIR}/rbutilqt-lang.qrc
    )

if(WIN32)
    target_sources(RockboxUtility PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/rbutilqt-win.qrc
        ${CMAKE_CURRENT_LIST_DIR}/rbutilqt.rc)
    if(QT_VERSION_MAJOR EQUAL 5)
        # needed on Windows, otherwise svg icons won't show.
        target_link_libraries(RockboxUtility Qt${QT_VERSION_MAJOR}::QSvgIconPlugin)
    endif()
endif()
if(APPLE)
    # MacOS Application Bundle specifics.
    target_sources(RockboxUtility PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/icons/rbutilqt.icns)
    set_target_properties(RockboxUtility
        PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/Info.plist)
    set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/icons/rbutilqt.icns
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

target_link_libraries(RockboxUtility rbbase cutelogger gitversion
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg)

if (TARGET Qt${QT_VERSION_MAJOR}::Multimedia) # optional
    target_link_libraries(RockboxUtility
        Qt${QT_VERSION_MAJOR}::Multimedia)
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(RockboxUtility Qt${QT_VERSION_MAJOR}::SvgWidgets)
endif()

target_include_directories(RockboxUtility PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/gui
    ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(RockboxUtility PRIVATE
    QT_DEPRECATED_WARNINGS
    RBUTIL _LARGEFILE64_SOURCE UNICODE)

set_property(TARGET RockboxUtility PROPERTY AUTOMOC ON)
set_property(TARGET RockboxUtility PROPERTY AUTORCC ON)
set_property(TARGET RockboxUtility PROPERTY AUTOUIC ON)

deploy_qt(TARGET RockboxUtility
    QTBINDIR ${QT_BINDIR}
    ICONFILE ${CMAKE_CURRENT_LIST_DIR}/../../docs/logo/rockbox-clef.svg
    DESKTOPFILE ${CMAKE_CURRENT_LIST_DIR}/RockboxUtility.desktop
    DMGBUILDCFG ${CMAKE_CURRENT_LIST_DIR}/dmgbuild.cfg
    EXECUTABLES ipodpatcher-bin sansapatcher-bin
    )

add_library(rbbase
    ${CMAKE_CURRENT_LIST_DIR}/../../tools/iriver.c
    ${CMAKE_CURRENT_LIST_DIR}/../../tools/iriver.h
    ${CMAKE_CURRENT_LIST_DIR}/../../tools/mkboot.c
    ${CMAKE_CURRENT_LIST_DIR}/../../tools/mkboot.h
    ${CMAKE_CURRENT_LIST_DIR}/../../tools/wavtrim.c
    ${CMAKE_CURRENT_LIST_DIR}/../../tools/wavtrim.h
    ${CMAKE_CURRENT_LIST_DIR}/base/archiveutil.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/archiveutil.h
    ${CMAKE_CURRENT_LIST_DIR}/base/autodetection.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/autodetection.h
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallams.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallams.h
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallbase.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallbase.h
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallbspatch.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallbspatch.h
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallchinachip.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallchinachip.h
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallfile.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallfile.h
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallhelper.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallhelper.h
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallhex.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallhex.h
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallimx.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallimx.h
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallipod.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallipod.h
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallmi4.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallmi4.h
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallmpio.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallmpio.h
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstalls5l.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstalls5l.h
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallsansa.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstallsansa.h
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstalltcc.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/bootloaderinstalltcc.h
    ${CMAKE_CURRENT_LIST_DIR}/base/encoderbase.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/encoderbase.h
    ${CMAKE_CURRENT_LIST_DIR}/base/encoderexe.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/encoderexe.h
    ${CMAKE_CURRENT_LIST_DIR}/base/encoderlame.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/encoderlame.h
    ${CMAKE_CURRENT_LIST_DIR}/base/encoderrbspeex.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/encoderrbspeex.h
    ${CMAKE_CURRENT_LIST_DIR}/base/encttssettings.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/encttssettings.h
    ${CMAKE_CURRENT_LIST_DIR}/base/httpget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/httpget.h
    ${CMAKE_CURRENT_LIST_DIR}/base/mspackutil.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/mspackutil.h
    ${CMAKE_CURRENT_LIST_DIR}/base/progressloglevels.h
    ${CMAKE_CURRENT_LIST_DIR}/base/playerbuildinfo.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/playerbuildinfo.h
    ${CMAKE_CURRENT_LIST_DIR}/base/rbsettings.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/rbsettings.h
    ${CMAKE_CURRENT_LIST_DIR}/base/rockboxinfo.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/rockboxinfo.h
    ${CMAKE_CURRENT_LIST_DIR}/base/system.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/system.h
    ${CMAKE_CURRENT_LIST_DIR}/base/talkfile.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/talkfile.h
    ${CMAKE_CURRENT_LIST_DIR}/base/talkgenerator.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/talkgenerator.h
    ${CMAKE_CURRENT_LIST_DIR}/base/ttsbase.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/ttsbase.h
    ${CMAKE_CURRENT_LIST_DIR}/base/ttsespeak.h
    ${CMAKE_CURRENT_LIST_DIR}/base/ttsespeakng.h
    ${CMAKE_CURRENT_LIST_DIR}/base/ttsexes.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/ttsexes.h
    ${CMAKE_CURRENT_LIST_DIR}/base/ttsfestival.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/ttsfestival.h
    ${CMAKE_CURRENT_LIST_DIR}/base/ttsflite.h
    ${CMAKE_CURRENT_LIST_DIR}/base/ttsmimic.h
    ${CMAKE_CURRENT_LIST_DIR}/base/ttsmssp.h
    ${CMAKE_CURRENT_LIST_DIR}/base/ttssapi.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/ttssapi.h
    ${CMAKE_CURRENT_LIST_DIR}/base/ttssapi4.h
    ${CMAKE_CURRENT_LIST_DIR}/base/ttsswift.h
    ${CMAKE_CURRENT_LIST_DIR}/base/uninstall.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/uninstall.h
    ${CMAKE_CURRENT_LIST_DIR}/base/utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/utils.h
    ${CMAKE_CURRENT_LIST_DIR}/base/voicefile.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/voicefile.h
    ${CMAKE_CURRENT_LIST_DIR}/base/zipinstaller.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/zipinstaller.h
    ${CMAKE_CURRENT_LIST_DIR}/base/ziputil.cpp
    ${CMAKE_CURRENT_LIST_DIR}/base/ziputil.h
    )

if(APPLE)
    target_sources(rbbase PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/base/ttscarbon.cpp
        ${CMAKE_CURRENT_LIST_DIR}/base/ttscarbon.h)
endif()

target_link_libraries(rbbase
    cutelogger quazip mspack bspatch rbspeex voicefont
    sansapatcher ipodpatcher chinachippatcher
    mkamsboot mkimxboot mkmpioboot mktccboot mks5lboot
    Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network)
target_include_directories(rbbase PUBLIC ${CMAKE_CURRENT_LIST_DIR}/base)
target_include_directories(rbbase PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(rbbase PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../../tools)
target_compile_definitions(rbbase PUBLIC RBUTIL _LARGEFILE64_SOURCE UNICODE)
target_compile_definitions(rbbase PRIVATE QT_DEPRECATED_WARNINGS)
if(WIN32)
    target_link_libraries(rbbase setupapi ws2_32 netapi32 crypt32 iphlpapi)
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    target_link_libraries(rbbase PkgConfig::libusb)
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    target_link_libraries(rbbase ${FRAMEWORK_IOKIT} ${FRAMEWORK_COREFOUNDATION}
        ${FRAMEWORK_CARBON} ${FRAMEWORK_SYSTEMCONFIGURATION} ${FRAMEWORK_CORESERVICES})
endif()
set_property(TARGET rbbase PROPERTY AUTOMOC ON)

# TODO: Upstream has cmake support, use that instead.
add_library(cutelogger)
# need to put those separate, otherwise cmake will not add them correctly -- seems to be related to moc'ing.
target_sources(cutelogger PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/logger/include/AbstractAppender.h
    ${CMAKE_CURRENT_LIST_DIR}/logger/include/AbstractStringAppender.h
    ${CMAKE_CURRENT_LIST_DIR}/logger/include/ConsoleAppender.h
    ${CMAKE_CURRENT_LIST_DIR}/logger/include/CuteLogger_global.h
    ${CMAKE_CURRENT_LIST_DIR}/logger/include/FileAppender.h
    ${CMAKE_CURRENT_LIST_DIR}/logger/include/Logger.h
    #${CMAKE_CURRENT_LIST_DIR}/logger/include/OutputDebugAppender.h
    ${CMAKE_CURRENT_LIST_DIR}/logger/src/AbstractAppender.cpp
    ${CMAKE_CURRENT_LIST_DIR}/logger/src/AbstractStringAppender.cpp
    ${CMAKE_CURRENT_LIST_DIR}/logger/src/ConsoleAppender.cpp
    ${CMAKE_CURRENT_LIST_DIR}/logger/src/FileAppender.cpp
    ${CMAKE_CURRENT_LIST_DIR}/logger/src/Logger.cpp
    #${CMAKE_CURRENT_LIST_DIR}/logger/src/OutputDebugAppender.cpp
    )
target_include_directories(cutelogger PUBLIC ${CMAKE_CURRENT_LIST_DIR}/logger/include)
target_link_libraries(cutelogger PUBLIC Qt${QT_VERSION_MAJOR}::Core)
target_compile_definitions(cutelogger PUBLIC CUTELOGGER_STATIC)
set_property(TARGET cutelogger PROPERTY AUTOMOC ON)

# TODO: Upstream has cmake support, use that instead.
add_library(quazip
    ${CMAKE_CURRENT_LIST_DIR}/quazip/ioapi.h
    ${CMAKE_CURRENT_LIST_DIR}/quazip/minizip_crypt.h
    ${CMAKE_CURRENT_LIST_DIR}/quazip/qioapi.cpp
    ${CMAKE_CURRENT_LIST_DIR}/quazip/quazip.cpp
    ${CMAKE_CURRENT_LIST_DIR}/quazip/quazipfile.cpp
    ${CMAKE_CURRENT_LIST_DIR}/quazip/quazipfile.h
    ${CMAKE_CURRENT_LIST_DIR}/quazip/quazipfileinfo.cpp
    ${CMAKE_CURRENT_LIST_DIR}/quazip/quazipfileinfo.h
    ${CMAKE_CURRENT_LIST_DIR}/quazip/quazip_global.h
    ${CMAKE_CURRENT_LIST_DIR}/quazip/quazip.h
    ${CMAKE_CURRENT_LIST_DIR}/quazip/quazipnewinfo.cpp
    ${CMAKE_CURRENT_LIST_DIR}/quazip/quazipnewinfo.h
    ${CMAKE_CURRENT_LIST_DIR}/quazip/unzip.c
    ${CMAKE_CURRENT_LIST_DIR}/quazip/unzip.h
    ${CMAKE_CURRENT_LIST_DIR}/quazip/zip.c
    ${CMAKE_CURRENT_LIST_DIR}/quazip/zip.h
    )
target_include_directories(quazip PUBLIC ${CMAKE_CURRENT_LIST_DIR}/quazip)
target_link_libraries(quazip z Qt${QT_VERSION_MAJOR}::Core)
if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(quazip Qt${QT_VERSION_MAJOR}::Core5Compat)
endif()
target_compile_definitions(quazip PUBLIC QUAZIP_STATIC)
set_property(TARGET quazip PROPERTY AUTOMOC ON)

add_library(mspack
    #mspack/cabc.c
    mspack/cabd.c
    mspack/cab.h
    #mspack/chmc.c
    #mspack/chmd.c
    #mspack/chm.h
    mspack/des.h
    #mspack/hlpc.c
    #mspack/hlpd.c
    #mspack/hlp.h
    #mspack/kwajc.c
    #mspack/kwajd.c
    #mspack/kwaj.h
    #mspack/litc.c
    #mspack/litd.c
    #mspack/lit.h
    #mspack/lzssd.c
    #mspack/lzss.h
    mspack/lzxc.c
    mspack/lzxd.c
    mspack/lzx.h
    mspack/mspack.h
    mspack/mszipc.c
    mspack/mszipd.c
    mspack/mszip.h
    mspack/qtmd.c
    mspack/qtm.h
    mspack/readbits.h
    mspack/readhuff.h
    mspack/sha.h
    mspack/system-mspack.c
    mspack/system-mspack.h
    #mspack/szddc.c
    #mspack/szddd.c
    #mspack/szdd.h
    )

target_include_directories(mspack PUBLIC ${CMAKE_CURRENT_LIST_DIR}/mspack)


# Unit tests
add_executable(test_compareversion
    base/utils.cpp
    base/utils.h
    test/stubs/stubs-compareversion.cpp
    test/test-compareversion.cpp)

target_link_libraries(test_compareversion Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test)
target_compile_definitions(test_compareversion PRIVATE UNICODE)
if(APPLE)
    target_link_libraries(test_compareversion ${FRAMEWORK_IOKIT} ${FRAMEWORK_COREFOUNDATION}
        ${FRAMEWORK_CARBON} ${FRAMEWORK_SYSTEMCONFIGURATION} ${FRAMEWORK_CORESERVICES})
endif()
target_include_directories(test_compareversion PRIVATE base test/stubs)
qtest_discover_tests(test_compareversion)
set_property(TARGET test_compareversion PROPERTY AUTOMOC ON)

add_executable(test_httpget
    test/test-httpget.cpp
    base/httpget.cpp
    base/httpget.h)
target_link_libraries(test_httpget Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Network)
target_include_directories(test_httpget PRIVATE base test/stubs)
qtest_discover_tests(test_httpget)
set_property(TARGET test_httpget PROPERTY AUTOMOC ON)

add_executable(test_playerbuildinfo
    base/playerbuildinfo.cpp
    base/playerbuildinfo.h
    base/rbsettings.h
    base/rockboxinfo.h
    rbutilqt.qrc  # needed for rbutil.ini resource.
    test/stubs/stubs-playerbuildinfo.cpp
    test/test-playerbuildinfo.cpp)
target_link_libraries(test_playerbuildinfo Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test)
target_include_directories(test_playerbuildinfo PRIVATE base test/stubs)
qtest_discover_tests(test_playerbuildinfo)
set_property(TARGET test_playerbuildinfo PROPERTY AUTOMOC ON)
set_property(TARGET test_playerbuildinfo PROPERTY AUTORCC ON)

add_executable(test_rockboxinfo
    base/rockboxinfo.cpp
    base/rockboxinfo.h
    test/test-rockboxinfo.cpp)

target_link_libraries(test_rockboxinfo Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test)
target_include_directories(test_rockboxinfo PRIVATE base test/stubs)
target_compile_definitions(test_rockboxinfo PRIVATE UNICODE)
qtest_discover_tests(test_rockboxinfo)
set_property(TARGET test_rockboxinfo PROPERTY AUTOMOC ON)

add_executable(test_talkgenerator
    base/talkgenerator.cpp
    base/talkgenerator.h
    base/encttssettings.cpp
    base/encttssettings.h
    base/ttsbase.h
    test/stubs/stubs-talkgenerator.cpp
    test/test-talkgenerator.qrc
    test/test-talkgenerator.cpp)

target_link_libraries(test_talkgenerator Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test)
target_include_directories(test_talkgenerator PRIVATE base test/stubs
                            ${CMAKE_CURRENT_LIST_DIR}/../../tools)
target_compile_definitions(test_talkgenerator PRIVATE UNICODE)
qtest_discover_tests(test_talkgenerator)
set_property(TARGET test_talkgenerator PROPERTY AUTOMOC ON)
set_property(TARGET test_talkgenerator PROPERTY AUTORCC ON)

