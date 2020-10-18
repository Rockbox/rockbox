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

TEMPLATE = app
TARGET = ipodpatcher
QT -= core

SOURCES += \
    main.c \
    ipodpatcher.c \
    ipodio-posix.c \
    ipodio-win32-scsi.c \
    ipodio-win32.c \
    fat32format.c \
    arc4.c \

HEADERS += \
    arc4.h \
    ipodio.h \
    ipodpatcher.h \
    parttypes.h \

DEFINES += RELEASE=1 _LARGEFILE64_SOURCE

RC_FILE = ipodpatcher.rc

macx {
    LIBS += -framework CoreFoundation -framework IOKit
}


unix {
    target.path = /usr/local/bin
    INSTALLS += target
}
