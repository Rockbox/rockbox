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
#include "bootloaderinstallipod.h"

#include "../ipodpatcher/ipodpatcher.h"


BootloaderInstallIpod::BootloaderInstallIpod(QObject *parent)
        : BootloaderInstallBase(parent)
{
    (void)parent;
    // initialize sector buffer. ipod_sectorbuf is defined in ipodpatcher.
    ipod_sectorbuf = NULL;
    ipod_alloc_buffer(&ipod_sectorbuf, BUFFER_SIZE);
}


BootloaderInstallIpod::~BootloaderInstallIpod()
{
    free(ipod_sectorbuf);
}


bool BootloaderInstallIpod::install(void)
{
    if(ipod_sectorbuf == NULL) {
        emit logItem(tr("Error: can't allocate buffer memory!"), LOGERROR);
        emit done(true);
        return false;
    }

    struct ipod_t ipod;

    int n = ipod_scan(&ipod);
    if(n == -1) {
        emit logItem(tr("No Ipod detected\n"
                "Permission for disc access denied!"),
                LOGERROR);
        emit done(true);
        return false;
    }
    if(n == 0) {
        emit logItem(tr("No Ipod detected!"), LOGERROR);
        emit done(true);
        return false;
    }

    if(ipod.macpod) {
        emit logItem(tr("Warning: This is a MacPod, Rockbox only runs on WinPods.\n"
                    "See http://www.rockbox.org/wiki/IpodConversionToFAT32"), LOGERROR);
        emit done(true);
        return false;
    }
    emit logItem(tr("Downloading bootloader file"), LOGINFO);

    downloadBlStart(m_blurl);
    connect(this, SIGNAL(downloadDone()), this, SLOT(installStage2()));
    return true;
}


void BootloaderInstallIpod::installStage2(void)
{
    struct ipod_t ipod;

    if(!ipodInitialize(&ipod)) {
        emit done(true);
        return;
    }

    read_directory(&ipod);

    if(ipod.nimages <= 0) {
        emit logItem(tr("Failed to read firmware directory"), LOGERROR);
        emit done(true);
        return;
    }
    if(getmodel(&ipod,(ipod.ipod_directory[0].vers>>8)) < 0) {
        emit logItem(tr("Unknown version number in firmware (%1)").arg(
                    ipod.ipod_directory[0].vers), LOGERROR);
        emit done(true);
        return;
    }

    if(ipod.macpod) {
        emit logItem(tr("Warning: This is a MacPod. Rockbox only runs on WinPods.\n"
                    "See http://www.rockbox.org/wiki/IpodConversionToFAT32"), LOGERROR);
        emit done(true);
        return;
    }

    if(ipod_reopen_rw(&ipod) < 0) {
        emit logItem(tr("Could not open Ipod in R/W mode"), LOGERROR);
        emit done(true);
        return;
    }

    m_tempfile.open();
    QString blfile = m_tempfile.fileName();
    m_tempfile.close();
    if(add_bootloader(&ipod, blfile.toLatin1().data(), FILETYPE_DOT_IPOD) == 0) {
        emit logItem(tr("Successfull added bootloader"), LOGOK);
        logInstall(LogAdd);
        emit done(false);
        ipod_close(&ipod);
        return;
    }
    else {
        emit logItem(tr("Failed to add bootloader"), LOGERROR);
        ipod_close(&ipod);
        emit done(true);
        return;
    }
    qDebug() << "version installed:" << m_blversion.toString(Qt::ISODate);
}


bool BootloaderInstallIpod::uninstall(void)
{
    struct ipod_t ipod;

    if(!ipodInitialize(&ipod)) {
        emit done(true);
        return false;
    }

    read_directory(&ipod);

    if (ipod.nimages <= 0) {
        emit logItem(tr("Failed to read firmware directory"),LOGERROR);
        emit done(true);
        return false;
    }
    if (getmodel(&ipod,(ipod.ipod_directory[0].vers>>8)) < 0) {
        emit logItem(tr("Unknown version number in firmware (%1)").arg(
                    ipod.ipod_directory[0].vers), LOGERROR);
        emit done(true);
        return false;
    }

    if (ipod_reopen_rw(&ipod) < 0) {
        emit logItem(tr("Could not open Ipod in RW mode"), LOGERROR);
        emit done(true);
        return false;
    }

    if (ipod.ipod_directory[0].entryOffset == 0) {
        emit logItem(tr("No bootloader detected."), LOGERROR);
        emit done(true);
        return false;
    }

    if (delete_bootloader(&ipod)==0) {
        emit logItem(tr("Successfully removed Bootloader"), LOGOK);
        logInstall(LogRemove);
        emit done(false);
        ipod_close(&ipod);
        return true;
    }
    else {
        emit logItem(tr("Removing the bootloader failed."), LOGERROR);
        emit done(true);
        ipod_close(&ipod);
        return false;
    }
}


BootloaderInstallBase::BootloaderType BootloaderInstallIpod::installed(void)
{
    struct ipod_t ipod;
    BootloaderInstallBase::BootloaderType result = BootloaderRockbox;

    if(!ipodInitialize(&ipod)) {
        qDebug() << "BootloaderInstallIpod::installed(): BootloaderUnknown";
        result = BootloaderUnknown;
    }

    if (ipod.ipod_directory[0].entryOffset == 0) {
        qDebug() << "BootloaderInstallIpod::installed(): BootloaderOther";
        result = BootloaderOther;
    }
    qDebug() << "BootloaderInstallIpod::installed(): BootloaderRockbox";
    ipod_close(&ipod);

    return result;
}


BootloaderInstallBase::Capabilities BootloaderInstallIpod::capabilities(void)
{
    return (Install | Uninstall | IsRaw);
}


bool BootloaderInstallIpod::ipodInitialize(struct ipod_t *ipod)
{
    ipod_scan(ipod);
    if(ipod_open(ipod, 0) < 0) {
        emit logItem(tr("Could not open Ipod"), LOGERROR);
        return false;
    }

    if(read_partinfo(ipod, 0) < 0) {
        emit logItem(tr("Could not read partition table"), LOGERROR);
        return false;
    }

    if(ipod->pinfo[0].start == 0) {
        emit logItem(tr("No firmware partition on disk"), LOGERROR);

        return false;
    }
    return true;
}

