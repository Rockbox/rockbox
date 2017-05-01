/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2017 by Lorenzo Miori
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtCore>
#include "bootloaderinstallbase.h"
#include "bootloaderinstallypr.h"
#include "utils.h"
#include "Logger.h"

BootloaderInstallYpr::BootloaderInstallYpr(QObject *parent)
        : BootloaderInstallBase(parent)
{
}

QString BootloaderInstallYpr::ofHint()
{
    return tr("Bootloader installation requires you to provide "
               "a firmware file of the original firmware (hex file). "
               "You need to download this file yourself due to legal "
               "reasons. Please refer to the "
               "<a href='http://www.rockbox.org/manual.shtml'>manual</a> and the "
               "<a href='http://www.rockbox.org/wiki/SamsungYPR0'>SamsungYPR0</a> wiki page on "
               "how to obtain this file.<br/>"
               "Press Ok to continue and browse your computer for the firmware "
               "file.");
}

bool BootloaderInstallYpr::install(void)
{
    if(m_offile.isEmpty())
        return false;

    return true;
}


void BootloaderInstallYpr::installStage2(void)
{
//    emit logItem(tr("Adding bootloader to firmware file"), LOGINFO);
//    QCoreApplication::processEvents();

//    // local temp file
//    QTemporaryFile tempbin;

//    // finally copy file to player
//    if(!Utils::resolvePathCase(m_blfile).isEmpty()) {
//        emit logItem(tr("A firmware file is already present on player"), LOGERROR);
//        emit done(true);
//        return;
//    }
//    if(targethex.copy(m_blfile)) {
//        emit logItem(tr("Success: modified firmware file created"), LOGINFO);
//    }
//    else {
//        emit logItem(tr("Copying modified firmware file failed"), LOGERROR);
//        emit done(true);
//        return;
//    }

//    logInstall(LogAdd);
    emit done(false);

    return;
}


bool BootloaderInstallYpr::uninstall(void)
{
    emit logItem(tr("Uninstallation not possible, only installation info removed"), LOGINFO);
    logInstall(LogRemove);
    emit done(true);
    return false;
}


BootloaderInstallBase::BootloaderType BootloaderInstallYpr::installed(void)
{
    return BootloaderUnknown;
}


BootloaderInstallBase::Capabilities BootloaderInstallYpr::capabilities(void)
{
    return (Install | NeedsOf);
}

QString BootloaderInstallYpr::scrambleError(int err)
{
    QString error;
    switch(err) {
        case -1: error = tr("Can't open input file"); break;
        case -2: error = tr("Can't open output file"); break;
        case -3: error = tr("invalid file: header length wrong"); break;
        case -4: error = tr("invalid file: unrecognized header"); break;
        case -5: error = tr("invalid file: \"length\" field wrong"); break;
        case -6: error = tr("invalid file: \"length2\" field wrong"); break;
        case -7: error = tr("invalid file: internal checksum error"); break;
        case -8: error = tr("invalid file: \"length3\" field wrong"); break;
        default: error = tr("unknown"); break;
    }
    return error;
}

