#
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#


# common files
SOURCES += \
 rbutilqt.cpp \
 main.cpp \
 installwindow.cpp \
 base/httpget.cpp \
 configure.cpp \
 zip/zip.cpp \
 zip/unzip.cpp \
 base/zipinstaller.cpp \
 progressloggergui.cpp \
 installtalkwindow.cpp \
 base/talkfile.cpp \
 base/talkgenerator.cpp \
 base/autodetection.cpp \
 ../ipodpatcher/ipodpatcher.c \
 ../sansapatcher/sansapatcher.c \
 ../chinachippatcher/chinachip.c \
 browsedirtree.cpp \
 themesinstallwindow.cpp \
 base/uninstall.cpp \
 uninstallwindow.cpp \
 base/utils.cpp \
 preview.cpp \
 base/encoders.cpp \
 encttscfggui.cpp \
 base/encttssettings.cpp \
 base/ttsbase.cpp \
 base/ttsexes.cpp \
 base/ttssapi.cpp \
 base/ttsfestival.cpp \
 ../../tools/wavtrim.c \
 ../../tools/voicefont.c \
 base/voicefile.cpp \
 createvoicewindow.cpp \
 base/rbsettings.cpp \
 base/serverinfo.cpp \
 base/systeminfo.cpp \
 base/rbunzip.cpp \
 base/rbzip.cpp \
 base/system.cpp \
 sysinfo.cpp \
 systrace.cpp \
 base/bootloaderinstallbase.cpp \
 base/bootloaderinstallmi4.cpp \
 base/bootloaderinstallhex.cpp \
 base/bootloaderinstallipod.cpp \
 base/bootloaderinstallsansa.cpp \
 base/bootloaderinstallfile.cpp \
 base/bootloaderinstallchinachip.cpp \
 base/bootloaderinstallams.cpp \
 base/bootloaderinstalltcc.cpp \
 base/rockboxinfo.cpp \
 ../../tools/mkboot.c \
 ../../tools/iriver.c \


HEADERS += \
 rbutilqt.h \
 installwindow.h \
 base/httpget.h \
 configure.h \
 zip/zip.h \
 zip/unzip.h \
 zip/zipentry_p.h \
 zip/unzip_p.h \
 zip/zip_p.h \
 version.h \
 base/zipinstaller.h \
 installtalkwindow.h \
 base/talkfile.h \
 base/talkgenerator.h \
 base/autodetection.h \
 base/progressloggerinterface.h \
 progressloggergui.h \
 ../ipodpatcher/ipodpatcher.h \
 ../ipodpatcher/ipodio.h \
 ../ipodpatcher/parttypes.h \
 ../sansapatcher/sansapatcher.h \
 ../sansapatcher/sansaio.h \
 ../chinachippatcher/chinachip.h \
 irivertools/h100sums.h \
 irivertools/h120sums.h \
 irivertools/h300sums.h \
 browsedirtree.h \
 themesinstallwindow.h \
 base/uninstall.h \
 uninstallwindow.h \
 base/utils.h \
 preview.h \
 base/encoders.h \
 encttscfggui.h \
 base/encttssettings.h \
 base/ttsbase.h \
 base/ttsexes.h \
 base/ttsfestival.h \
 base/ttssapi.h \
 ../../tools/wavtrim.h \
 ../../tools/voicefont.h \
 base/voicefile.h \
 createvoicewindow.h \
 base/rbsettings.h \
 base/serverinfo.h \
 base/systeminfo.h \
 base/rbunzip.h \
 base/rbzip.h \
 sysinfo.h \
 base/system.h \
 systrace.h \
 base/bootloaderinstallbase.h \
 base/bootloaderinstallmi4.h \
 base/bootloaderinstallhex.h \
 base/bootloaderinstallipod.h \
 base/bootloaderinstallsansa.h \
 base/bootloaderinstallfile.h \
 base/bootloaderinstallchinachip.h \
 base/bootloaderinstallams.h \
 base/bootloaderinstalltcc.h \
 base/rockboxinfo.h \
 ../../tools/mkboot.h \
 ../../tools/iriver.h \

FORMS += \
 rbutilqtfrm.ui \
 aboutbox.ui \
 installwindowfrm.ui \
 progressloggerfrm.ui \
 configurefrm.ui \
 browsedirtreefrm.ui \
 installtalkfrm.ui \
 themesinstallfrm.ui \
 uninstallfrm.ui \
 previewfrm.ui \
 createvoicefrm.ui \
 sysinfofrm.ui \
 systracefrm.ui


TRANSLATIONS += \
 lang/rbutil_cs.ts \
 lang/rbutil_de.ts \
 lang/rbutil_fi.ts \
 lang/rbutil_fr.ts \
 lang/rbutil_gr.ts \
 lang/rbutil_he.ts \
 lang/rbutil_it.ts \
 lang/rbutil_ja.ts \
 lang/rbutil_nl.ts \
 lang/rbutil_pl.ts \
 lang/rbutil_pt.ts \
 lang/rbutil_pt_BR.ts \
 lang/rbutil_ru.ts \
 lang/rbutil_tr.ts \
 lang/rbutil_zh_CN.ts \
 lang/rbutil_zh_TW.ts \

RESOURCES += $$_PRO_FILE_PWD_/rbutilqt.qrc
!dbg {
    RESOURCES += $$_PRO_FILE_PWD_/rbutilqt-lang.qrc
}
# windows specific files
win32 {
    SOURCES +=  ../ipodpatcher/ipodio-win32.c
    SOURCES +=  ../ipodpatcher/ipodio-win32-scsi.c
    SOURCES +=  ../sansapatcher/sansaio-win32.c
    RC_FILE = rbutilqt.rc
    RESOURCES += $$_PRO_FILE_PWD_/rbutilqt-win.qrc
}

# unix specific files
unix {
    SOURCES +=  ../ipodpatcher/ipodio-posix.c
    SOURCES +=  ../sansapatcher/sansaio-posix.c
}

# mac specific files
macx {
    SOURCES += base/ttscarbon.cpp
    HEADERS += base/ttscarbon.h
    QMAKE_INFO_PLIST = Info.plist
    RC_FILE = icons/rbutilqt.icns
}

