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
TARGET = sansapatcher
QT -= core

SOURCES += \
    sansaio-posix.c \
    sansaio-win32.c \
    sansapatcher.c \
    main.c

HEADERS += \
    parttypes.h \
    sansaio.h \
    sansapatcher.h \

RC_FILE = sansapatcher.rc

DEFINES += _LARGEFILE64_SOURCE

unix {
    target.path = /usr/local/bin
    INSTALLS += target
}
