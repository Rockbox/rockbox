/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2009 by Maurus Cuelenaere
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
#include "bootloaderinstallchinachip.h"

#include "../chinachippatcher/chinachip.h"

BootloaderInstallChinaChip::BootloaderInstallChinaChip(QObject *parent)
        : BootloaderInstallBase(parent)
{
    (void)parent;
}

QString BootloaderInstallChinaChip::ofHint()
{
    return tr("Bootloader installation requires you to provide "
               "a firmware file of the original firmware (HXF file). "
               "You need to download this file yourself due to legal "
               "reasons. Please refer to the "
               "<a href='http://www.rockbox.org/manual.shtml'>manual</a> and the "
               "<a href='http://www.rockbox.org/wiki/OndaVX747"
               "#Download_and_extract_a_recent_ve'>OndaVX747</a> wiki page on "
               "how to obtain this file.<br/>"
               "Press Ok to continue and browse your computer for the firmware "
               "file.");
}

bool BootloaderInstallChinaChip::install()
{
    if(m_offile.isEmpty())
        return false;

    emit logItem(tr("Downloading bootloader file"), LOGINFO);

    connect(this, &BootloaderInstallBase::downloadDone, this, &BootloaderInstallChinaChip::installStage2);
    downloadBlStart(m_blurl);

    return true;
}

void BootloaderInstallChinaChip::installStage2()
{
    enum cc_error result;
    bool error = true;
    m_tempfile.open();
    QString blfile = m_tempfile.fileName();
    m_tempfile.close();

    QString backupfile = QFileInfo(m_blfile).absoluteDir().absoluteFilePath("ccpmp.bin");

    result = chinachip_patch(m_offile.toLocal8Bit(), blfile.toLocal8Bit(),
            m_blfile.toLocal8Bit(), backupfile.toLocal8Bit());
    switch(result) {
        case E_OK:
            error = false;
            break;
        case E_OPEN_FIRMWARE:
            emit logItem(tr("Could not open firmware file"), LOGERROR);
            break;
        case E_OPEN_BOOTLOADER:
            emit logItem(tr("Could not open bootloader file"), LOGERROR);
            break;
        case E_MEMALLOC:
            emit logItem(tr("Could not allocate memory"), LOGERROR);
            break;
        case E_LOAD_FIRMWARE:
            emit logItem(tr("Could not load firmware file"), LOGERROR);
            break;
        case E_INVALID_FILE:
            emit logItem(tr("File is not a valid ChinaChip firmware"), LOGERROR);
            break;
        case E_NO_CCPMP:
            emit logItem(tr("Could not find ccpmp.bin in input file"), LOGERROR);
            break;
        case E_OPEN_BACKUP:
            emit logItem(tr("Could not open backup file for ccpmp.bin"), LOGERROR);
            break;
        case E_WRITE_BACKUP:
            emit logItem(tr("Could not write backup file for ccpmp.bin"), LOGERROR);
            break;
        case E_LOAD_BOOTLOADER:
            emit logItem(tr("Could not load bootloader file"), LOGERROR);
            break;
        case E_GET_TIME:
            emit logItem(tr("Could not get current time"), LOGERROR);
            break;
        case E_OPEN_OUTFILE:
            emit logItem(tr("Could not open output file"), LOGERROR);
            break;
        case E_WRITE_OUTFILE:
            emit logItem(tr("Could not write output file"), LOGERROR);
            break;
        default:
            emit logItem(tr("Unexpected error from chinachippatcher"), LOGERROR);
            break;
    }

    emit done(error);
}

bool BootloaderInstallChinaChip::uninstall()
{
    /* TODO: only way is to restore the OF */
    return false;
}

BootloaderInstallBase::BootloaderType BootloaderInstallChinaChip::installed()
{
    /* TODO: find a way to figure this out */
    return BootloaderUnknown;
}

BootloaderInstallBase::Capabilities BootloaderInstallChinaChip::capabilities()
{
    return (Install | IsFile | NeedsOf);
}
