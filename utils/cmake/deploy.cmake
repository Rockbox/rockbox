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
    set(LINUXDEPLOY ${CMAKE_BINARY_DIR}/linuxdeploy-x86_64.AppImage)
    set(LINUXDEPLOYQT ${CMAKE_BINARY_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage)
    add_custom_command(
        COMMENT "Downloading linuxdeploy"
        OUTPUT ${LINUXDEPLOY}
               ${LINUXDEPLOYQT}
        COMMAND ${CMAKE_COMMAND}
            -DOUTDIR=${CMAKE_BINARY_DIR}
            -DURL=https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
            -P ${CMAKE_CURRENT_LIST_DIR}/download.cmake
        COMMAND ${CMAKE_COMMAND}
            -DOUTDIR=${CMAKE_BINARY_DIR}
            -DURL=https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
            -P ${CMAKE_CURRENT_LIST_DIR}/download.cmake
    )
    # intermediate target needed to be able to get back to the actual file dependency.
    add_custom_target(linuxdeploy DEPENDS ${LINUXDEPLOY})

    function(deploy_qt)
        cmake_parse_arguments(deploy ""
            "TARGET;DESKTOPFILE;ICONFILE;QTBINDIR;DMGBUILDCFG"
            "EXECUTABLES"
            ${ARGN})
        if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            message(WARNING "Deploying a Debug build.")
        endif()

        add_custom_target(deploy_${deploy_TARGET}
            DEPENDS ${CMAKE_BINARY_DIR}/${deploy_TARGET}.AppImage)

        # need extra rules so we can use generator expressions
        # (using get_target_property() doesn't know neede values during generation)
        set(_deploy_deps "")
        foreach(_deploy_exe_tgt ${deploy_EXECUTABLES})
            add_custom_command(
                OUTPUT ${CMAKE_BINARY_DIR}/${_deploy_exe_tgt}.appimage.stamp
                COMMENT "Copying ${_deploy_exe_tgt} to AppImage"
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/AppImage-${deploy_TARGET}/usr/bin
                COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${_deploy_exe_tgt}>
                                            ${CMAKE_BINARY_DIR}/AppImage-${deploy_TARGET}/usr/bin
                COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/${_deploy_exe_tgt}.appimage.stamp
                DEPENDS ${_deploy_exe_tgt}
                )
            add_custom_target(deploy_${deploy_TARGET}_${_deploy_exe_tgt}
                DEPENDS ${CMAKE_BINARY_DIR}/${_deploy_exe_tgt}.appimage.stamp)

            set(_deploy_deps "${_deploy_deps};deploy_${deploy_TARGET}_${_deploy_exe_tgt}")
        endforeach()

        add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/${deploy_TARGET}.AppImage
            COMMENT "Creating AppImage ${deploy_TARGET}"
            COMMAND OUTPUT=${CMAKE_BINARY_DIR}/${deploy_TARGET}.AppImage
                    ${LINUXDEPLOY}
                    --plugin qt
                    --icon-file=${deploy_ICONFILE}
                    --desktop-file=${deploy_DESKTOPFILE}
                    --executable=$<TARGET_FILE:${deploy_TARGET}>
                    --appdir=${CMAKE_BINARY_DIR}/AppImage-${deploy_TARGET}
                    --output=appimage
                    --verbosity=2
                    DEPENDS ${deploy_TARGET} ${_deploy_deps} linuxdeploy
            )
        add_dependencies(deploy deploy_${deploy_TARGET})
    endfunction()
endif()

# MacOS: Build dmg
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    function(deploy_qt)
        cmake_parse_arguments(deploy ""
            "TARGET;DESKTOPFILE;ICONFILE;QTBINDIR;DMGBUILDCFG"
            "EXECUTABLES"
            ${ARGN})
        if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            message(WARNING "Deploying a Debug build.")
        endif()
        set(DMGBUILD ${CMAKE_BINARY_DIR}/venv/bin/python3 -m dmgbuild)
        set(DMGBUILD_STAMP ${CMAKE_BINARY_DIR}/dmgbuild.stamp)
        find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${QTBINDIR}")

        # need extra rules so we can use generator expressions
        # (using get_target_property() doesn't know neede values during generation)
        set(_deploy_deps "")
        foreach(_deploy_exe_tgt ${deploy_EXECUTABLES})
            add_custom_command(
                OUTPUT ${CMAKE_BINARY_DIR}/${_deploy_exe_tgt}.app.stamp
                COMMENT "Copying ${_deploy_exe_tgt} to App"
                COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_BUNDLE_CONTENT_DIR:${deploy_TARGET}>/bin
                COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${_deploy_exe_tgt}>
                                            $<TARGET_BUNDLE_CONTENT_DIR:${deploy_TARGET}>/bin
                COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/${_deploy_exe_tgt}.app.stamp
                DEPENDS ${_deploy_exe_tgt}
                )
            add_custom_target(deploy_${deploy_TARGET}_${_deploy_exe_tgt}
                DEPENDS ${CMAKE_BINARY_DIR}/${_deploy_exe_tgt}.app.stamp)

            set(_deploy_deps "${_deploy_deps};deploy_${deploy_TARGET}_${_deploy_exe_tgt}")
        endforeach()

        add_custom_command(
            COMMENT "Setting up dmgbuild virtualenv"
            OUTPUT ${DMGBUILD_STAMP}
            COMMAND python3 -m venv ${CMAKE_BINARY_DIR}/venv
            COMMAND ${CMAKE_BINARY_DIR}/venv/bin/python -m pip install -q dmgbuild
        )

        add_custom_command(
            # TODO: find a better way to figure the app bundle name.
            OUTPUT ${CMAKE_BINARY_DIR}/${deploy_TARGET}.dmg
            COMMENT "Running macdeployqt and creating dmg ${deploy_TARGET}"
            COMMAND ${MACDEPLOYQT_EXECUTABLE} ${deploy_TARGET}.app
            COMMAND ${DMGBUILD} -s ${deploy_DMGBUILDCFG}
                    -Dappbundle=${deploy_TARGET}.app
                    ${deploy_TARGET} ${CMAKE_BINARY_DIR}/${deploy_TARGET}.dmg
            DEPENDS ${deploy_TARGET}
                    ${DMGBUILD_STAMP}
                    ${_deploy_deps}
            )
        add_custom_target(deploy_${deploy_TARGET}
            DEPENDS ${CMAKE_BINARY_DIR}/${deploy_TARGET}.dmg)
        add_dependencies(deploy deploy_${deploy_TARGET})
    endfunction()
endif()

# Windows. Copy to dist folder, run windeployqt on the binary, compress to zip.
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    function(deploy_qt)
        cmake_parse_arguments(deploy ""
            "TARGET;DESKTOPFILE;ICONFILE;QTBINDIR;DMGBUILDCFG"
            "EXECUTABLES"
            ${ARGN})
        if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            message(WARNING "Deploying a Debug build.")
        endif()
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QTBINDIR}")
        set(deploydir ${CMAKE_BINARY_DIR}/deploy-${deploy_TARGET})
        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(
                COMMENT "Creating deploy folder and running windeployqt"
                OUTPUT ${deploydir}/${deploy_TARGET}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${deploydir}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${deploy_TARGET}> ${deploydir}
                COMMAND ${WINDEPLOYQT_EXECUTABLE}
                        # on MinGW, release is mistaken as debug for Qt less than 5.14.
                        # For later versions the opposite is true: adding --debug or
                        # --release will fail with "platform plugin not found."
                        $<IF:$<VERSION_LESS:${Qt${QT_VERSION_MAJOR}Core_VERSION},5.14.0>,$<IF:$<CONFIG:Debug>,--debug,--release>,>
                        ${deploydir}/$<TARGET_FILE_NAME:${deploy_TARGET}>
                DEPENDS ${deploy_TARGET}
            )
        else()
            add_custom_command(
                COMMENT "Creating deploy folder"
                OUTPUT ${deploydir}/${deploy_TARGET}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${deploydir}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${deploy_TARGET}> ${deploydir}
                DEPENDS ${deploy_TARGET}
            )
        endif()
        # need extra rules so we can use generator expressions
        # (using get_target_property() doesn't know neede values during generation)
        set(_deploy_deps "")
        foreach(_deploy_exe_tgt ${deploy_EXECUTABLES})
            add_custom_command(
                OUTPUT ${CMAKE_BINARY_DIR}/${_deploy_exe_tgt}.app.stamp
                COMMENT "Copying ${_deploy_exe_tgt} to deploy folder"
                COMMAND ${CMAKE_COMMAND} -E make_directory ${deploydir}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${_deploy_exe_tgt}> ${deploydir}
                COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/${_deploy_exe_tgt}.app.stamp
                DEPENDS ${_deploy_exe_tgt}
                )
            add_custom_target(deploy_${deploy_TARGET}_${_deploy_exe_tgt}
                DEPENDS ${CMAKE_BINARY_DIR}/${_deploy_exe_tgt}.app.stamp)

            set(_deploy_deps "${_deploy_deps};deploy_${deploy_TARGET}_${_deploy_exe_tgt}")
        endforeach()
        add_custom_command(
            COMMENT "Compressing to zip"
            OUTPUT ${CMAKE_BINARY_DIR}/${deploy_TARGET}.zip
            WORKING_DIRECTORY ${deploydir}
            COMMAND ${CMAKE_COMMAND} -E tar c ${CMAKE_BINARY_DIR}/${deploy_TARGET}.zip
                    --format=zip .
            DEPENDS ${deploydir}/${deploy_TARGET} ${_deploy_deps}
            )

        add_custom_target(deploy_${deploy_TARGET}
            DEPENDS ${CMAKE_BINARY_DIR}/${deploy_TARGET}.zip)
        add_dependencies(deploy deploy_${deploy_TARGET})
    endfunction()
endif()

