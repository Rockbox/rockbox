/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2008 by Dominik Wenger
 *   $Id: bootloaderinstallams.cpp 24778 2010-02-19 23:45:29Z funman $
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
#include "bootloaderinstallmpio.h"
#include "Logger.h"

#include "../mkmpioboot/mkmpioboot.h"

BootloaderInstallMpio::BootloaderInstallMpio(QObject *parent)
        : BootloaderInstallBase(parent)
{
}

QString BootloaderInstallMpio::ofHint()
{
    return tr("Bootloader installation requires you to provide "
              "a firmware file of the original firmware (bin file). "
              "You need to download this file yourself due to legal "
              "reasons. Please refer to the "
              "<a href='http://www.rockbox.org/manual.shtml'>manual</a> and "
              "the <a href='http://www.rockbox.org/wiki/MPIOHD200Port'>MPIOHD200Port</a> "
              "wiki page on how to obtain this file.<br/>"
              "Press Ok to continue and browse your computer for the firmware "
              "file.");
}

bool BootloaderInstallMpio::install(void)
{
    if(m_offile.isEmpty())
        return false;

    LOG_INFO() << "installing bootloader";

    // download firmware from server
    emit logItem(tr("Downloading bootloader file"), LOGINFO);

    connect(this, &BootloaderInstallBase::downloadDone, this, &BootloaderInstallMpio::installStage2);
    downloadBlStart(m_blurl);

    return true;
}

void BootloaderInstallMpio::installStage2(void)
{
    LOG_INFO() << "installStage2";

    int origin = 0xe0000;   /* MPIO HD200 bootloader address */

    m_tempfile.open();
    QString bootfile = m_tempfile.fileName();
    m_tempfile.close();

    int ret = mkmpioboot(m_offile.toLocal8Bit().data(),
            bootfile.toLocal8Bit().data(), m_blfile.toLocal8Bit().data(), origin);

    if(ret != 0)
    {
        QString error;
        switch(ret)
        {
            case -1:
                error = tr("Could not open the original firmware.");
                break;
            case -2:
                error = tr("Could not read the original firmware.");
                break;
            case -3:
                error = tr("Loaded firmware file does not look like MPIO original firmware file.");
                break;
            case -4:
                error = tr("Could not open downloaded bootloader.");
                break;
            case -5:
                error = tr("Place for bootloader in OF file not empty.");
                break;
            case -6:
                error = tr("Could not read the downloaded bootloader.");
                break;
            case -7:
                error = tr("Bootloader checksum error.");
                break;
            case -8:
                error = tr("Could not open output file.");
                break;
            case -9:
                error = tr("Could not write output file.");
                break;
            default:
                error = tr("Unknown error number: %1").arg(ret);
                break;
        }

        LOG_ERROR() << "Patching original firmware failed:" << error;
        emit logItem(tr("Patching original firmware failed: %1").arg(error), LOGERROR);
        emit done(true);
        return;
    }

    //end of install
    LOG_INFO() << "install successful";
    emit logItem(tr("Success: modified firmware file created"), LOGINFO);
    logInstall(LogAdd);
    emit done(false);
    return;
}

bool BootloaderInstallMpio::uninstall(void)
{
    emit logItem(tr("To uninstall, perform a normal upgrade with an unmodified "
                    "original firmware"), LOGINFO);
    logInstall(LogRemove);
    emit done(true);
    return false;
}

BootloaderInstallBase::BootloaderType BootloaderInstallMpio::installed(void)
{
    return BootloaderUnknown;
}

BootloaderInstallBase::Capabilities BootloaderInstallMpio::capabilities(void)
{
    return (Install | NeedsOf);
}

