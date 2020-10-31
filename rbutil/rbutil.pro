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

unix:!mac{
    CCACHE = $$system(which ccache)
    !isEmpty(CCACHE) {
        message("using ccache at $$CCACHE")
        QMAKE_CXX = ccache $$QMAKE_CXX
        QMAKE_CC = ccache $$QMAKE_CC
    }
}

TEMPLATE = subdirs
SUBDIRS = rbutilqt ipodpatcher sansapatcher

rbutilqt.depends = ipodpatcher sansapatcher


unix:!macx {
    LINUXDEPLOYQTURL = https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
    LINUXDEPLOYURL = https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage

    appimage_dl.commands = \
        curl -C- -fLO $$LINUXDEPLOYQTURL -fLO $$LINUXDEPLOYURL ; \
        chmod +x *.AppImage; \
        touch appimage_dl

    appimage_prepare.commands = \
        mkdir -p AppImage/usr/bin; \
        cp sansapatcher/sansapatcher AppImage/usr/bin; \
        cp ipodpatcher/ipodpatcher AppImage/usr/bin; \
        cp rbutilqt/RockboxUtility AppImage/usr/bin

    appimage_prepare.depends = ipodpatcher sansapatcher rbutilqt appimage_dl

    appimage.commands = \
        ./linuxdeploy-x86_64.AppImage \
        --appdir AppImage \
        --verbosity 2 --plugin qt --output appimage \
        -e AppImage/usr/bin/RockboxUtility \
        -d $$_PRO_FILE_PWD_/rbutilqt/RockboxUtility.desktop \
        -i $$_PRO_FILE_PWD_/../docs/logo/rockbox-clef.svg
    appimage.depends = appimage_prepare

    QMAKE_EXTRA_TARGETS += appimage_dl appimage_prepare appimage
}

macx {
    dmgbuild.commands = \
        python3 -m venv venv; \
        venv/bin/python -m pip install dmgbuild

    appbundle_merge.commands = \
        cp -pr rbutilqt/RockboxUtility.app .; \
        cp ipodpatcher/ipodpatcher.app/Contents/MacOS/ipodpatcher RockboxUtility.app/Contents/MacOS; \
        cp sansapatcher/sansapatcher.app/Contents/MacOS/sansapatcher RockboxUtility.app/Contents/MacOS

    appbundle_deploy.commands = \
        $$[QT_INSTALL_BINS]/macdeployqt RockboxUtility.app
    appbundle_deploy.depends = appbundle_merge

    dmg.commands = \
        venv/bin/dmgbuild -s $$_PRO_FILE_PWD_/rbutilqt/dmgbuild.cfg \
            -Dbasepath=$$_PRO_FILE_PWD_ \"Rockbox Utility\" RockboxUtility.dmg

    dmg.depends = appbundle_merge appbundle_deploy dmgbuild

    QMAKE_EXTRA_TARGETS += dmgbuild appbundle_merge appbundle_deploy dmg
}


