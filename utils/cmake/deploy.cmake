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

# include this file to
# - get a new target "deploy"
# - get a function "deploy_qt()" which will add a deploy target that creates a
#   zip / AppImage / dmg and depends on "deploy".

if(NOT have_deploy)
    add_custom_target(deploy)
    set(have_deploy ON)
endif()

# Linux: Build AppImage
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    function(deploy_qt target qtbindir iconfile desktopfile dmgbuildcfg)
        if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            message(WARNING "Deploying a Debug build.")
        endif()

        set(LINUXDEPLOY ${CMAKE_BINARY_DIR}/linuxdeploy-x86_64.AppImage)
        set(LINUXDEPLOYQT ${CMAKE_BINARY_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage)
        add_custom_command(
            COMMENT "Downloading linuxdeploy"
            OUTPUT ${LINUXDEPLOY}
            OUTPUT ${LINUXDEPLOYQT}
            COMMAND ${CMAKE_COMMAND}
                -DOUTDIR=${CMAKE_BINARY_DIR}
                -DURL=https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
                -P ${CMAKE_SOURCE_DIR}/cmake/download.cmake
            COMMAND ${CMAKE_COMMAND}
                -DOUTDIR=${CMAKE_BINARY_DIR}
                -DURL=https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
                -P ${CMAKE_SOURCE_DIR}/cmake/download.cmake
            )
        add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/${target}.AppImage
            COMMENT "Creating AppImage ${target}"
            COMMAND OUTPUT=${CMAKE_BINARY_DIR}/${target}.AppImage
                    ${LINUXDEPLOY}
                    --icon-file=${iconfile}
                    --desktop-file=${desktopfile}
                    --executable=$<TARGET_FILE:${target}>
                    --appdir=AppImage-${target}
                    --output=appimage
                    --verbosity=2
            DEPENDS ${target}
                    ${LINUXDEPLOY}
            )
        add_custom_target(deploy_${target}
            DEPENDS ${CMAKE_BINARY_DIR}/${target}.AppImage)
        add_dependencies(deploy deploy_${target})
    endfunction()
endif()

# MacOS: Build dmg
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    function(deploy_qt target qtbindir iconfile desktopfile dmgbuildcfg)
        if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            message(WARNING "Deploying a Debug build.")
        endif()
        set(DMGBUILD ${CMAKE_BINARY_DIR}/venv/bin/dmgbuild)
        find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${qtbindir}")

        add_custom_command(
            COMMENT "Setting up dmgbuild virtualenv"
            OUTPUT ${DMGBUILD}
            COMMAND python3 -m venv ${CMAKE_BINARY_DIR}/venv
            COMMAND ${CMAKE_BINARY_DIR}/venv/bin/python -m pip install -q dmgbuild
        )

        add_custom_command(
            # TODO: find a better way to figure the app bundle name.
            OUTPUT ${CMAKE_BINARY_DIR}/${target}.dmg
            COMMENT "Running macdeployqt and creating dmg ${target}"
            COMMAND ${MACDEPLOYQT_EXECUTABLE} ${target}.app
            COMMAND ${DMGBUILD} -s ${dmgbuildcfg}
                    -Dappbundle=${target}.app
                    ${target} ${CMAKE_BINARY_DIR}/${target}.dmg
            DEPENDS ${target}
                    ${DMGBUILD}
            )
        add_custom_target(deploy_${target}
            DEPENDS ${CMAKE_BINARY_DIR}/${target}.dmg)
        add_dependencies(deploy deploy_${target})
    endfunction()
endif()

# Windows. Copy to dist folder, run windeployqt on the binary, compress to zip.
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    function(deploy_qt target qtbindir iconfile desktopfile dmgbuildcfg)
        if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            message(WARNING "Deploying a Debug build.")
        endif()
        set(_targetfile ${target}.exe)  # TODO: Use property. OUTPUT_NAME seems to fail.
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${qtbindir}")
        set(deploydir ${CMAKE_BINARY_DIR}/deploy-${target})
        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(
                COMMENT "Creating deploy folder and running windeployqt"
                OUTPUT ${deploydir}/${_targetfile}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${deploydir}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_targetfile} ${deploydir}
                COMMAND ${WINDEPLOYQT_EXECUTABLE}
                        $<IF:$<CONFIG:Debug>,--debug,--release>  # on MinGW, release is mistaken as debug.
                        ${deploydir}/${_targetfile}
                DEPENDS ${target}
            )
        else()
            add_custom_command(
                COMMENT "Creating deploy folder"
                OUTPUT ${deploydir}/${_targetfile}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${deploydir}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_targetfile} ${deploydir}
                DEPENDS ${target}
            )
        endif()
        add_custom_command(
            COMMENT "Compressing to zip"
            OUTPUT ${CMAKE_BINARY_DIR}/${target}.zip
            WORKING_DIRECTORY ${deploydir}
            COMMAND ${CMAKE_COMMAND} -E tar c ${CMAKE_BINARY_DIR}/${target}.zip
                    --format=zip .
            DEPENDS ${deploydir}/${_targetfile}
            )

        add_custom_target(deploy_${target}
            DEPENDS ${CMAKE_BINARY_DIR}/${target}.zip)
        add_dependencies(deploy deploy_${target})
    endfunction()
endif()

