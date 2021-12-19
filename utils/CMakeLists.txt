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
cmake_minimum_required(VERSION 3.12)
project(RockboxUtils)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message("-- Found ccache, enabling")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# Global compiler options
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang and AppleClang
    if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        # using clang with clang-cl front end
    elseif (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
        # using clang with regular front end
        add_compile_options(-Wall -Wextra)
    endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra)
endif()


enable_testing()

# Qt
find_package(QT NAMES Qt6 Qt5 REQUIRED)
if(QT_VERSION_MAJOR EQUAL 5)
    find_package(Qt5 COMPONENTS Core Widgets Svg Multimedia Network Test LinguistTools REQUIRED)
else()
    find_package(Qt6 REQUIRED COMPONENTS Core Core5Compat Widgets Svg Multimedia Network LinguistTools
                 OPTIONAL_COMPONENTS Test)
endif()
message("-- Found Qt${QT_VERSION_MAJOR}: ${Qt${QT_VERSION_MAJOR}_DIR}")

# If we're on Linux, try to find the used libs in the system.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # required system libs
    find_package(PkgConfig)
    pkg_check_modules(libusb QUIET REQUIRED IMPORTED_TARGET libusb-1.0)
    message("-- Found libusb: ${libusb_LINK_LIBRARIES}")

    # optional system libs.
    pkg_check_modules(speex QUIET IMPORTED_TARGET speex speexdsp)
    if (${speex_FOUND})
        message("-- Found speex: ${speex_LINK_LIBRARIES}")
    else()
        message("-- Could not find system speex, using our own copy.")
    endif()
endif()

if(APPLE)
    find_library(FRAMEWORK_IOKIT IOKit)
    find_library(FRAMEWORK_COREFOUNDATION CoreFoundation)
    find_library(FRAMEWORK_CARBON Carbon)
    find_library(FRAMEWORK_SYSTEMCONFIGURATION SystemConfiguration)
    find_library(FRAMEWORK_CORESERVICES CoreServices)
endif()

set(RBUTIL_TS_FILES
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/lang/rbutil_cs.ts
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/lang/rbutil_de.ts
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/lang/rbutil_fi.ts
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/lang/rbutil_fr.ts
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/lang/rbutil_gr.ts
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/lang/rbutil_he.ts
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/lang/rbutil_it.ts
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/lang/rbutil_ja.ts
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/lang/rbutil_nl.ts
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/lang/rbutil_pl.ts
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/lang/rbutil_pt_BR.ts
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/lang/rbutil_pt.ts
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/lang/rbutil_ru.ts
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/lang/rbutil_tr.ts
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/lang/rbutil_zh_CN.ts
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/lang/rbutil_zh_TW.ts
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
            ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/lang/rbutilqt-lang.qrc
            ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${RBUTIL_QM_FILES}
    )

add_executable(RockboxUtility WIN32 MACOSX_BUNDLE
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/main.cpp

    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/aboutbox.ui
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/configure.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/configurefrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/configure.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/createvoicefrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/createvoicewindow.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/createvoicewindow.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/encttscfggui.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/encttscfggui.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/installtalkfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/installtalkwindow.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/installtalkwindow.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/preview.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/previewfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/preview.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/progressloggerfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/progressloggergui.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/progressloggergui.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/rbutilqt.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/rbutilqtfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/rbutilqt.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/sysinfo.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/sysinfofrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/sysinfo.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/systrace.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/systracefrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/systrace.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/themesinstallfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/themesinstallwindow.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/themesinstallwindow.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/uninstallfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/uninstallwindow.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/uninstallwindow.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/version.h

    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/gui/backupdialog.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/gui/backupdialogfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/gui/backupdialog.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/gui/changelog.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/gui/changelogfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/gui/changelog.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/gui/comboboxviewdelegate.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/gui/comboboxviewdelegate.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/gui/infowidget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/gui/infowidgetfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/gui/infowidget.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/gui/selectiveinstallwidget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/gui/selectiveinstallwidgetfrm.ui
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/gui/selectiveinstallwidget.h

    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/rbutilqt.qrc
    ${CMAKE_CURRENT_BINARY_DIR}/rbutilqt-lang.qrc
    )

if(WIN32)
    target_sources(RockboxUtility PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/rbutilqt-win.qrc
        ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/rbutilqt.rc)
    # needed on Windows, otherwise svg icons won't show.
    target_link_libraries(RockboxUtility Qt${QT_VERSION_MAJOR}::QSvgIconPlugin)
endif()
if(APPLE)
    # MacOS Application Bundle specifics.
    target_sources(RockboxUtility PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/icons/rbutilqt.icns)
    set_target_properties(RockboxUtility
        PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/Info.plist)
    set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/rbutilqt/icons/rbutilqt.icns
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

target_link_libraries(RockboxUtility rbbase cutelogger
    Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Multimedia)

target_include_directories(RockboxUtility PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/gui
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt)
target_compile_definitions(RockboxUtility PRIVATE
    QT_DEPRECATED_WARNINGS
    RBUTIL _LARGEFILE64_SOURCE UNICODE)

set_property(TARGET RockboxUtility PROPERTY AUTOMOC ON)
set_property(TARGET RockboxUtility PROPERTY AUTORCC ON)
set_property(TARGET RockboxUtility PROPERTY AUTOUIC ON)

add_library(rbbase
    ${CMAKE_CURRENT_LIST_DIR}/../tools/iriver.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/iriver.h
    ${CMAKE_CURRENT_LIST_DIR}/../tools/mkboot.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/mkboot.h
    ${CMAKE_CURRENT_LIST_DIR}/../tools/wavtrim.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/wavtrim.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/archiveutil.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/archiveutil.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/autodetection.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/autodetection.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallams.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallams.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallbase.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallbase.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallbspatch.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallbspatch.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallchinachip.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallchinachip.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallfile.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallfile.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallhelper.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallhelper.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallhex.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallhex.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallimx.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallimx.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallipod.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallipod.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallmi4.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallmi4.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallmpio.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallmpio.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstalls5l.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstalls5l.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallsansa.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstallsansa.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstalltcc.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/bootloaderinstalltcc.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/encoderbase.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/encoderbase.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/encoderexe.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/encoderexe.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/encoderlame.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/encoderlame.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/encoderrbspeex.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/encoderrbspeex.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/encttssettings.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/encttssettings.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/httpget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/httpget.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/mspackutil.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/mspackutil.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/progressloggerinterface.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/playerbuildinfo.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/playerbuildinfo.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/rbsettings.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/rbsettings.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/rockboxinfo.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/rockboxinfo.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/system.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/system.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/talkfile.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/talkfile.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/talkgenerator.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/talkgenerator.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ttsbase.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ttsbase.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ttsespeak.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ttsespeakng.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ttsexes.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ttsexes.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ttsfestival.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ttsfestival.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ttsflite.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ttsmimic.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ttsmssp.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ttssapi.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ttssapi.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ttssapi4.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ttsswift.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/uninstall.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/uninstall.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/utils.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/voicefile.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/voicefile.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/zipinstaller.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/zipinstaller.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ziputil.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ziputil.h
    )

if(APPLE)
    target_sources(rbbase PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ttscarbon.cpp
        ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base/ttscarbon.h)
endif()

target_link_libraries(rbbase
    cutelogger quazip mspack bspatch rbspeex voicefont
    sansapatcher ipodpatcher chinachippatcher
    mkamsboot mkimxboot mkmpioboot mktccboot mks5lboot
    Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network)
target_include_directories(rbbase PUBLIC ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/base)
target_include_directories(rbbase PRIVATE ${CMAKE_CURRENT_LIST_DIR}/rbutilqt)
target_include_directories(rbbase PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../tools)
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
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/logger/include/AbstractAppender.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/logger/include/AbstractStringAppender.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/logger/include/ConsoleAppender.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/logger/include/CuteLogger_global.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/logger/include/FileAppender.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/logger/include/Logger.h
    #${CMAKE_CURRENT_LIST_DIR}/rbutilqt/logger/include/OutputDebugAppender.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/logger/src/AbstractAppender.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/logger/src/AbstractStringAppender.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/logger/src/ConsoleAppender.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/logger/src/FileAppender.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/logger/src/Logger.cpp
    #${CMAKE_CURRENT_LIST_DIR}/rbutilqt/logger/src/OutputDebugAppender.cpp
    )
target_include_directories(cutelogger PUBLIC ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/logger/include)
target_link_libraries(cutelogger PUBLIC Qt${QT_VERSION_MAJOR}::Core)
target_compile_definitions(cutelogger PUBLIC CUTELOGGER_STATIC)
set_property(TARGET cutelogger PROPERTY AUTOMOC ON)

# TODO: Upstream has cmake support, use that instead.
add_library(quazip
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/quazip/ioapi.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/quazip/minizip_crypt.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/quazip/qioapi.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/quazip/quazip.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/quazip/quazipfile.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/quazip/quazipfile.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/quazip/quazipfileinfo.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/quazip/quazipfileinfo.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/quazip/quazip_global.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/quazip/quazip.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/quazip/quazipnewinfo.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/quazip/quazipnewinfo.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/quazip/unzip.c
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/quazip/unzip.h
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/quazip/zip.c
    ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/quazip/zip.h
    )
target_include_directories(quazip PUBLIC ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/quazip)
target_link_libraries(quazip z Qt${QT_VERSION_MAJOR}::Core)
if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(quazip Qt${QT_VERSION_MAJOR}::Core5Compat)
endif()
target_compile_definitions(quazip PUBLIC QUAZIP_STATIC)
set_property(TARGET quazip PROPERTY AUTOMOC ON)

add_library(mspack
    rbutilqt/mspack/cabc.c
    rbutilqt/mspack/cabd.c
    rbutilqt/mspack/cab.h
    #rbutilqt/mspack/chmc.c
    #rbutilqt/mspack/chmd.c
    #rbutilqt/mspack/chm.h
    rbutilqt/mspack/des.h
    #rbutilqt/mspack/hlpc.c
    #rbutilqt/mspack/hlpd.c
    #rbutilqt/mspack/hlp.h
    #rbutilqt/mspack/kwajc.c
    #rbutilqt/mspack/kwajd.c
    #rbutilqt/mspack/kwaj.h
    #rbutilqt/mspack/litc.c
    #rbutilqt/mspack/litd.c
    #rbutilqt/mspack/lit.h
    #rbutilqt/mspack/lzssd.c
    #rbutilqt/mspack/lzss.h
    rbutilqt/mspack/lzxc.c
    rbutilqt/mspack/lzxd.c
    rbutilqt/mspack/lzx.h
    rbutilqt/mspack/mspack.h
    rbutilqt/mspack/mszipc.c
    rbutilqt/mspack/mszipd.c
    rbutilqt/mspack/mszip.h
    rbutilqt/mspack/qtmd.c
    rbutilqt/mspack/qtm.h
    rbutilqt/mspack/readbits.h
    rbutilqt/mspack/readhuff.h
    rbutilqt/mspack/sha.h
    rbutilqt/mspack/system-mspack.c
    rbutilqt/mspack/system-mspack.h
    #rbutilqt/mspack/szddc.c
    #rbutilqt/mspack/szddd.c
    #rbutilqt/mspack/szdd.h
    )

target_include_directories(mspack PUBLIC ${CMAKE_CURRENT_LIST_DIR}/rbutilqt/mspack)


add_library(ipodpatcher
    ipodpatcher/arc4.h
    ipodpatcher/arc4.c
    ipodpatcher/fat32format.c
    ipodpatcher/ipodpatcher.c
    ipodpatcher/ipodio.h
    ipodpatcher/ipodpatcher.h
    ipodpatcher/parttypes.h
    )
if(WIN32)
    target_sources(ipodpatcher PRIVATE
        ipodpatcher/ipodio-win32.c
        ipodpatcher/ipodio-win32-scsi.c
    )
else()
    target_sources(ipodpatcher PRIVATE
        ipodpatcher/ipodio-posix.c
    )
endif()
target_compile_definitions(ipodpatcher PUBLIC RBUTIL _LARGEFILE64_SOURCE)
target_include_directories(ipodpatcher PUBLIC ${CMAKE_CURRENT_LIST_DIR}/ipodpatcher)

add_library(sansapatcher
    sansapatcher/parttypes.h
    sansapatcher/sansaio.h
    sansapatcher/sansapatcher.c
    sansapatcher/sansapatcher.h
    )
if(WIN32)
    target_sources(sansapatcher PRIVATE
        sansapatcher/sansaio-win32.c
    )
else()
    target_sources(sansapatcher PRIVATE
        sansapatcher/sansaio-posix.c
    )
endif()

target_compile_definitions(sansapatcher PUBLIC RBUTIL _LARGEFILE64_SOURCE)
target_include_directories(sansapatcher PUBLIC ${CMAKE_CURRENT_LIST_DIR}/sansapatcher)

add_library(chinachippatcher
    chinachippatcher/chinachip.c
    chinachippatcher/chinachip.h
    )
target_compile_definitions(chinachippatcher PUBLIC RBUTIL _LARGEFILE64_SOURCE)

add_library(mkamsboot
    mkamsboot/dualboot.c
    mkamsboot/dualboot.h
    mkamsboot/md5.c
    mkamsboot/md5.h
    mkamsboot/mkamsboot.c
    mkamsboot/mkamsboot.h
    )

target_compile_definitions(mkamsboot PUBLIC _LARGEFILE64_SOURCE)
target_link_libraries(mkamsboot ucl)

add_library(ucl
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/alloc.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/fake16.h
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/getbit.h
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/internal.h
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/io.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/Makefile
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2_99.ch
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2b_99.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2b_d.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2b_ds.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2b_to.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2d_99.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2d_d.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2d_ds.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2d_to.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2e_99.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2e_d.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2e_ds.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/n2e_to.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_conf.h
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_crc.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_dll.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_init.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_mchw.ch
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_ptr.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_ptr.h
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_str.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_swd.ch
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_util.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/src/ucl_util.h
    )

target_include_directories(ucl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../tools/ucl/include)

add_library(rbspeex
    ${CMAKE_CURRENT_LIST_DIR}/../tools/rbspeex/rbspeex.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/rbspeex/rbspeex.h
    )
target_compile_definitions(rbspeex PRIVATE HAVE_CONFIG_H ROCKBOX_VOICE_ENCODER STATIC)
target_include_directories(rbspeex PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../tools/rbspeex)


if (${speex_FOUND})
    target_link_libraries(rbspeex PUBLIC PkgConfig::speex)
else()
    target_sources(rbspeex PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/bits.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/cb_search.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/exc_10_16_table.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/exc_10_32_table.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/exc_20_32_table.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/exc_5_256_table.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/exc_5_64_table.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/exc_8_128_table.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/filters.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/gain_table.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/gain_table_lbr.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/hexc_10_32_table.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/hexc_table.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/high_lsp_tables.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/lsp.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/lsp_tables_nb.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/ltp.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/modes.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/modes_wb.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/nb_celp.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/quant_lsp.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/sb_celp.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/speex.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/speex_callbacks.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/lpc.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/vbr.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/vq.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/window.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex/resample.c
        )
    target_include_directories(rbspeex PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../lib/rbcodec/codecs/libspeex)
endif()

add_library(mkimxboot
    mkimxboot/dualboot.c
    mkimxboot/md5.c
    mkimxboot/md5.h
    mkimxboot/mkimxboot.c
    mkimxboot/mkimxboot.h
    )
target_include_directories(mkimxboot PUBLIC ${CMAKE_CURRENT_LIST_DIR}/mkimxboot)
target_link_libraries(mkimxboot sbtools)

add_library(sbtools
    imxtools/sbtools/crc.c
    imxtools/sbtools/crypto.c
    imxtools/sbtools/crypto.h
    imxtools/sbtools/elf.c
    imxtools/sbtools/elf.h
    imxtools/sbtools/misc.c
    imxtools/sbtools/misc.h
    imxtools/sbtools/sb.c
    imxtools/sbtools/sb.h
    )

target_include_directories(sbtools PUBLIC ${CMAKE_CURRENT_LIST_DIR}/imxtools/sbtools)
target_link_libraries(sbtools PUBLIC rbtomcrypt)

add_library(mkmpioboot
    mkmpioboot/mkmpioboot.c
    mkmpioboot/mkmpioboot.h
    )
target_include_directories(mkmpioboot PUBLIC ${CMAKE_CURRENT_LIST_DIR}/mkmpioboot)

add_library(mktccboot
    mktccboot/mktccboot.c
    mktccboot/mktccboot.h
    )
target_include_directories(mktccboot PUBLIC ${CMAKE_CURRENT_LIST_DIR}/mktccboot)
target_include_directories(mktccboot PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../tools)
target_link_libraries(mktccboot PUBLIC telechips)

add_library(mks5lboot
    mks5lboot/dualboot.c
    mks5lboot/dualboot.h
    mks5lboot/ipoddfu.c
    mks5lboot/mkdfu.c
    mks5lboot/mks5lboot.h
    )
target_include_directories(mks5lboot PUBLIC ${CMAKE_CURRENT_LIST_DIR}/mks5lboot)

add_library(bspatch
    bspatch/bspatch.c
    bspatch/bspatch.h
    )
target_include_directories(bspatch PUBLIC ${CMAKE_CURRENT_LIST_DIR}/bspatch)
target_link_libraries(bspatch bz2)

add_library(voicefont
    ${CMAKE_CURRENT_LIST_DIR}/../tools/voicefont.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/voicefont.h
    )
target_include_directories(voicefont PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../tools)
target_compile_definitions(voicefont PUBLIC RBUTIL)

add_library(telechips
    ${CMAKE_CURRENT_LIST_DIR}/../tools/telechips.c
    ${CMAKE_CURRENT_LIST_DIR}/../tools/telechips.h
    )
target_include_directories(telechips PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../tools)

find_library(LIBBZ2 bz2)
if(${LIBBZ2} STREQUAL "LIBBZ2-NOTFOUND")
    message("-- bz2 not found, building our own")
    add_library(bz2
        bzip2/blocksort.c
        bzip2/bzlib.c
        bzip2/bzlib.h
        bzip2/bzlib_private.h
        bzip2/compress.c
        bzip2/crctable.c
        bzip2/decompress.c
        bzip2/huffman.c
        bzip2/randtable.c
        )
    target_include_directories(bz2 PUBLIC ${CMAKE_CURRENT_LIST_DIR}/bzip2)
else()
    message("-- Found libbz2: ${LIBBZ2}")
endif()

add_library(rbtomcrypt
    tomcrypt/src/ciphers/aes/aes.c
    tomcrypt/src/ciphers/aes/aes_tab.c
    tomcrypt/src/misc/crypt/crypt_argchk.c
    tomcrypt/src/misc/crypt/crypt_register_cipher.c
    tomcrypt/src/misc/crypt/crypt_cipher_is_valid.c
    tomcrypt/src/misc/crypt/crypt_cipher_descriptor.c
    tomcrypt/src/misc/zeromem.c
    tomcrypt/src/misc/compare_testvector.c
    tomcrypt/src/modes/cbc/cbc_start.c
    tomcrypt/src/modes/cbc/cbc_decrypt.c
    tomcrypt/src/modes/cbc/cbc_encrypt.c
    tomcrypt/src/hashes/sha1.c
    tomcrypt/src/headers/tomcrypt_argchk.h
    tomcrypt/src/headers/tomcrypt_cfg.h
    tomcrypt/src/headers/tomcrypt_cipher.h
    tomcrypt/src/headers/tomcrypt_custom.h
    tomcrypt/src/headers/tomcrypt.h
    tomcrypt/src/headers/tomcrypt_hash.h
    tomcrypt/src/headers/tomcrypt_mac.h
    tomcrypt/src/headers/tomcrypt_macros.h
    tomcrypt/src/headers/tomcrypt_math.h
    tomcrypt/src/headers/tomcrypt_misc.h
    tomcrypt/src/headers/tomcrypt_pkcs.h
    tomcrypt/src/headers/tomcrypt_pk.h
    tomcrypt/src/headers/tomcrypt_private.h
    tomcrypt/src/headers/tomcrypt_prng.h
    )
target_include_directories(rbtomcrypt PUBLIC ${CMAKE_CURRENT_LIST_DIR}/tomcrypt/src/headers)


# Unit tests
add_executable(test_compareversion
    rbutilqt/base/utils.cpp
    rbutilqt/base/utils.h
    rbutilqt/test/stubs/stubs-compareversion.cpp
    rbutilqt/test/test-compareversion.cpp)

target_link_libraries(test_compareversion Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test)
target_compile_definitions(test_compareversion PRIVATE UNICODE)
if(APPLE)
    target_link_libraries(test_compareversion ${FRAMEWORK_IOKIT} ${FRAMEWORK_COREFOUNDATION}
        ${FRAMEWORK_CARBON} ${FRAMEWORK_SYSTEMCONFIGURATION} ${FRAMEWORK_CORESERVICES})
endif()
target_include_directories(test_compareversion PRIVATE rbutilqt/base rbutilqt/test/stubs)
add_test(test_compareversion test_compareversion)
set_property(TARGET test_compareversion PROPERTY AUTOMOC ON)

add_executable(test_httpget
    rbutilqt/test/test-httpget.cpp
    rbutilqt/base/httpget.cpp
    rbutilqt/base/httpget.h)
target_link_libraries(test_httpget Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Network)
target_include_directories(test_httpget PRIVATE rbutilqt/base rbutilqt/test/stubs)
add_test(test_httpget test_httpget)
set_property(TARGET test_httpget PROPERTY AUTOMOC ON)

add_executable(test_playerbuildinfo
    rbutilqt/base/playerbuildinfo.cpp
    rbutilqt/base/playerbuildinfo.h
    rbutilqt/base/rbsettings.h
    rbutilqt/base/rockboxinfo.h
    rbutilqt/rbutilqt.qrc  # needed for rbutil.ini resource.
    rbutilqt/test/stubs/stubs-playerbuildinfo.cpp
    rbutilqt/test/test-playerbuildinfo.cpp)
target_link_libraries(test_playerbuildinfo Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test)
target_include_directories(test_playerbuildinfo PRIVATE rbutilqt/base rbutilqt/test/stubs)
add_test(test_playerbuildinfo test_playerbuildinfo)
set_property(TARGET test_playerbuildinfo PROPERTY AUTOMOC ON)
set_property(TARGET test_playerbuildinfo PROPERTY AUTORCC ON)

add_executable(test_rockboxinfo
    rbutilqt/base/rockboxinfo.cpp
    rbutilqt/base/rockboxinfo.h
    rbutilqt/test/test-rockboxinfo.cpp)

target_link_libraries(test_rockboxinfo Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test)
target_include_directories(test_rockboxinfo PRIVATE rbutilqt/base rbutilqt/test/stubs)
target_compile_definitions(test_rockboxinfo PRIVATE UNICODE)
add_test(test_rockboxinfo test_rockboxinfo)
set_property(TARGET test_rockboxinfo PROPERTY AUTOMOC ON)

