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
 gui/infowidget.cpp \
 rbutilqt.cpp \
 main.cpp \
 base/httpget.cpp \
 configure.cpp \
 base/zipinstaller.cpp \
 progressloggergui.cpp \
 installtalkwindow.cpp \
 base/talkfile.cpp \
 base/talkgenerator.cpp \
 base/autodetection.cpp \
 themesinstallwindow.cpp \
 base/uninstall.cpp \
 uninstallwindow.cpp \
 base/utils.cpp \
 preview.cpp \
 base/encoderbase.cpp \
 base/encoderrbspeex.cpp \
 base/encoderlame.cpp \
 base/encoderexe.cpp \
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
 base/system.cpp \
 sysinfo.cpp \
 systrace.cpp \
 base/bootloaderinstallbase.cpp \
 base/bootloaderinstallbspatch.cpp \
 base/bootloaderinstallhelper.cpp \
 base/bootloaderinstallmi4.cpp \
 base/bootloaderinstallhex.cpp \
 base/bootloaderinstallipod.cpp \
 base/bootloaderinstallsansa.cpp \
 base/bootloaderinstallfile.cpp \
 base/bootloaderinstallchinachip.cpp \
 base/bootloaderinstallams.cpp \
 base/bootloaderinstalltcc.cpp \
 base/bootloaderinstallmpio.cpp \
 base/bootloaderinstallimx.cpp \
 base/bootloaderinstalls5l.cpp \
 base/rockboxinfo.cpp \
 ../../tools/mkboot.c \
 ../../tools/iriver.c \
 base/ziputil.cpp \
 gui/comboboxviewdelegate.cpp \
 gui/selectiveinstallwidget.cpp \
 gui/backupdialog.cpp \
 gui/changelog.cpp \
 mspack/cabd.c \
 mspack/lzxd.c \
 mspack/mszipd.c \
 mspack/qtmd.c \
 mspack/system-mspack.c \
 base/mspackutil.cpp \
 base/archiveutil.cpp \


HEADERS += \
 gui/infowidget.h \
 rbutilqt.h \
 base/httpget.h \
 configure.h \
 version.h \
 base/zipinstaller.h \
 installtalkwindow.h \
 base/talkfile.h \
 base/talkgenerator.h \
 base/autodetection.h \
 base/progressloggerinterface.h \
 progressloggergui.h \
 irivertools/h100sums.h \
 irivertools/h120sums.h \
 irivertools/h300sums.h \
 themesinstallwindow.h \
 base/uninstall.h \
 uninstallwindow.h \
 base/utils.h \
 preview.h \
 base/encoderbase.h \
 base/encoderrbspeex.h \
 base/encoderlame.h \
 base/encoderexe.h \
 encttscfggui.h \
 base/encttssettings.h \
 base/ttsbase.h \
 base/ttsexes.h \
 base/ttsespeak.h \
 base/ttsespeakng.h \
 base/ttsflite.h \
 base/ttsfestival.h \
 base/ttsmimic.h \
 base/ttsswift.h \
 base/ttssapi.h \
 base/ttssapi4.h \
 base/ttsmssp.h \
 ../../tools/wavtrim.h \
 ../../tools/voicefont.h \
 base/voicefile.h \
 createvoicewindow.h \
 base/rbsettings.h \
 base/serverinfo.h \
 base/systeminfo.h \
 sysinfo.h \
 base/system.h \
 systrace.h \
 base/bootloaderinstallbase.h \
 base/bootloaderinstallbspatch.h \
 base/bootloaderinstallhelper.h \
 base/bootloaderinstallmi4.h \
 base/bootloaderinstallhex.h \
 base/bootloaderinstallipod.h \
 base/bootloaderinstallsansa.h \
 base/bootloaderinstallfile.h \
 base/bootloaderinstallchinachip.h \
 base/bootloaderinstallams.h \
 base/bootloaderinstalltcc.h \
 base/bootloaderinstallmpio.h \
 base/bootloaderinstallimx.h \
 base/bootloaderinstalls5l.h \
 base/rockboxinfo.h \
 ../../tools/mkboot.h \
 ../../tools/iriver.h \
 base/ziputil.h \
 lame/lame.h \
 gui/comboboxviewdelegate.h \
 gui/selectiveinstallwidget.h \
 gui/backupdialog.h \
 gui/changelog.h \
 mspack/cab.h \
 mspack/chm.h \
 mspack/des.h \
 mspack/hlp.h \
 mspack/kwaj.h \
 mspack/lit.h \
 mspack/lzss.h \
 mspack/lzx.h \
 mspack/mspack.h \
 mspack/mszip.h \
 mspack/qtm.h \
 mspack/readbits.h \
 mspack/readhuff.h \
 mspack/sha.h \
 mspack/system-mspack.h \
 mspack/szdd.h \
 base/mspackutil.h \
 base/archiveutil.h \


FORMS += \
 gui/infowidgetfrm.ui \
 rbutilqtfrm.ui \
 aboutbox.ui \
 progressloggerfrm.ui \
 configurefrm.ui \
 installtalkfrm.ui \
 themesinstallfrm.ui \
 uninstallfrm.ui \
 previewfrm.ui \
 createvoicefrm.ui \
 sysinfofrm.ui \
 systracefrm.ui \
 gui/selectiveinstallwidgetfrm.ui \
 gui/backupdialogfrm.ui \
 gui/changelogfrm.ui


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
    RC_FILE = rbutilqt.rc
    RESOURCES += $$_PRO_FILE_PWD_/rbutilqt-win.qrc
}

# mac specific files
macx {
    SOURCES += base/ttscarbon.cpp
    HEADERS += base/ttscarbon.h
    QMAKE_INFO_PLIST = Info.plist
    RC_FILE = icons/rbutilqt.icns
}

