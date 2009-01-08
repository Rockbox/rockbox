/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2008 by Dominik Riebeling
 *   $Id$
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
#include "bootloaderinstallsansa.h"

#include "../sansapatcher/sansapatcher.h"

BootloaderInstallSansa::BootloaderInstallSansa(QObject *parent)
        : BootloaderInstallBase(parent)
{
    (void)parent;
    // initialize sector buffer. sansa_sectorbuf is instantiated by
    // sansapatcher.
    sansa_sectorbuf = NULL;
    sansa_alloc_buffer(&sansa_sectorbuf, BUFFER_SIZE);
}


BootloaderInstallSansa::~BootloaderInstallSansa()
{
    free(sansa_sectorbuf);
}


/** Start bootloader installation.
 */
bool BootloaderInstallSansa::install(void)
{
    if(sansa_sectorbuf == NULL) {
        emit logItem(tr("Error: can't allocate buffer memory!"), LOGERROR);
        return false;
        emit done(true);
    }

    emit logItem(tr("Searching for Sansa"), LOGINFO);

    struct sansa_t sansa;

    int n = sansa_scan(&sansa);
    if(n == -1) {
        emit logItem(tr("Permission for disc access denied!\n"
                "This is required to install the bootloader"),
                LOGERROR);
        emit done(true);
        return false;
    }
    if(n == 0) {
        emit logItem(tr("No Sansa detected!"), LOGERROR);
        emit done(true);
        return false;
    }
    emit logItem(tr("Downloading bootloader file"), LOGINFO);

    downloadBlStart(m_blurl);
    connect(this, SIGNAL(downloadDone()), this, SLOT(installStage2()));
    return true;
}


/** Finish bootloader installation.
 */
void BootloaderInstallSansa::installStage2(void)
{
    struct sansa_t sansa;
    sansa_scan(&sansa);

    if(sansa_open(&sansa, 0) < 0) {
        emit logItem(tr("could not open Sansa"), LOGERROR);
        emit done(true);
        return;
    }

    if(sansa_read_partinfo(&sansa, 0) < 0)
    {
        emit logItem(tr("could not read partitiontable"), LOGERROR);
        emit done(true);
        return;
    }

    int i = is_sansa(&sansa);
    if(i < 0) {

        emit logItem(tr("Disk is not a Sansa (Error: %1), aborting.").arg(i), LOGERROR);
        emit done(true);
        return;
    }

    if(sansa.hasoldbootloader) {
        emit logItem(tr("OLD ROCKBOX INSTALLATION DETECTED, ABORTING.\n"
               "You must reinstall the original Sansa firmware before running\n"
               "sansapatcher for the first time.\n"
               "See http://www.rockbox.org/wiki/SansaE200Install\n"),
               LOGERROR);
        emit done(true);
        return;
    }

    if(sansa_reopen_rw(&sansa) < 0) {
        emit logItem(tr("Could not open Sansa in R/W mode"), LOGERROR);
        emit done(true);
        return;
    }

    m_tempfile.open();
    QString blfile = m_tempfile.fileName();
    m_tempfile.close();
    if(sansa_add_bootloader(&sansa, blfile.toLatin1().data(),
            FILETYPE_MI4) == 0) {
        emit logItem(tr("Successfully installed bootloader"), LOGOK);
        logInstall(LogAdd);
        emit done(false);
        sansa_close(&sansa);
        return;
    }
    else {
        emit logItem(tr("Failed to install bootloader"), LOGERROR);
        sansa_close(&sansa);
        emit done(true);
        return;
    }

}


/** Uninstall the bootloader.
 */
bool BootloaderInstallSansa::uninstall(void)
{
    struct sansa_t sansa;

    if(sansa_scan(&sansa) != 1) {
        emit logItem(tr("Can't find Sansa"), LOGERROR);
        emit done(true);
        return false;
    }

    if (sansa_open(&sansa, 0) < 0) {
        emit logItem(tr("Could not open Sansa"), LOGERROR);
        emit done(true);
        return false;
    }

    if (sansa_read_partinfo(&sansa,0) < 0) {
        emit logItem(tr("Could not read partition table"), LOGERROR);
        emit done(true);
        return false;
    }

    int i = is_sansa(&sansa);
    if(i < 0) {
        emit logItem(tr("Disk is not a Sansa (Error %1), aborting.").arg(i), LOGERROR);
        emit done(true);
        return false;
    }

    if (sansa.hasoldbootloader) {
        emit logItem(tr("OLD ROCKBOX INSTALLATION DETECTED, ABORTING.\n"
                    "You must reinstall the original Sansa firmware before running\n"
                    "sansapatcher for the first time.\n"
                    "See http://www.rockbox.org/wiki/SansaE200Install\n"),
                    LOGERROR);
        emit done(true);
        return false;
    }

    if (sansa_reopen_rw(&sansa) < 0) {
        emit logItem(tr("Could not open Sansa in R/W mode"), LOGERROR);
        emit done(true);
        return false;
    }

    if (sansa_delete_bootloader(&sansa)==0) {
        emit logItem(tr("Successfully removed bootloader"), LOGOK);
        logInstall(LogRemove);
        emit done(false);
        sansa_close(&sansa);
        return true;
    }
    else {
        emit logItem(tr("Removing bootloader failed."),LOGERROR);
        emit done(true);
        sansa_close(&sansa);
        return false;
    }

    return false;
}


/** Check if bootloader is already installed
 */
BootloaderInstallBase::BootloaderType BootloaderInstallSansa::installed(void)
{
    struct sansa_t sansa;
    int num;

    if(sansa_scan(&sansa) != 1) {
        return BootloaderUnknown;
    }
    if (sansa_open(&sansa, 0) < 0) {
        return BootloaderUnknown;
    }
    if (sansa_read_partinfo(&sansa,0) < 0) {
        sansa_close(&sansa);
        return BootloaderUnknown;
    }
    if(is_sansa(&sansa) < 0) {
        sansa_close(&sansa);
        return BootloaderUnknown;
    }
    if((num = sansa_list_images(&sansa)) == 2) {
        sansa_close(&sansa);
        return BootloaderRockbox;
    }
    else if(num == 1) {
        sansa_close(&sansa);
        return BootloaderOther;
    }
    return BootloaderUnknown;

}


/** Get capabilities of subclass installer.
 */
BootloaderInstallBase::Capabilities BootloaderInstallSansa::capabilities(void)
{
    return (Install | Uninstall | IsRaw | CanCheckInstalled);
}

