/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2008 by Dominik Riebeling
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
#include "utils.h"
#include "Logger.h"


BootloaderInstallIpod::BootloaderInstallIpod(QObject *parent)
        : BootloaderInstallBase(parent)
{
    (void)parent;
    // initialize sector buffer. The sector buffer is part of the ipod_t
    // structure, so a second instance of this class will have its own buffer.
    ipod_alloc_buffer(&ipod, BUFFER_SIZE);
}


BootloaderInstallIpod::~BootloaderInstallIpod()
{
    if(ipod.sectorbuf) {
        ipod_dealloc_buffer(&ipod);
    }
}


bool BootloaderInstallIpod::install(void)
{
    if(ipod.sectorbuf == nullptr) {
        emit logItem(tr("Error: can't allocate buffer memory!"), LOGERROR);
        emit done(true);
        return false;
    }
    // save buffer pointer before cleaning up ipod_t structure
    unsigned char* sb = ipod.sectorbuf;
    memset(&ipod, 0, sizeof(struct ipod_t));
    ipod.sectorbuf = sb;

    if(!ipodInitialize(&ipod)) {
        emit done(true);
        return false;
    }

    if(ipod.nimages <= 0) {
        emit logItem(tr("Failed to read firmware directory"), LOGERROR);
        emit done(true);
        return false;
    }
    if(getmodel(&ipod,(ipod.ipod_directory[ipod.ososimage].vers>>8)) < 0) {
        emit logItem(tr("Unknown version number in firmware (%1)").arg(
                    ipod.ipod_directory[ipod.ososimage].vers), LOGERROR);
        emit done(true);
        return false;
    }
    if(ipod.macpod) {
        emit logItem(tr("Warning: This is a MacPod, Rockbox only runs on WinPods. \n"
                    "See http://www.rockbox.org/wiki/IpodConversionToFAT32"), LOGERROR);
        emit done(true);
        return false;
    }
    emit logItem(tr("Downloading bootloader file"), LOGINFO);

    downloadBlStart(m_blurl);
    connect(this, &BootloaderInstallBase::downloadDone, this, &BootloaderInstallIpod::installStage2);
    return true;
}


void BootloaderInstallIpod::installStage2(void)
{
    emit logItem(tr("Installing Rockbox bootloader"), LOGINFO);
    QCoreApplication::processEvents();

    if(ipod_reopen_rw(&ipod) < 0) {
        emit logItem(tr("Could not open Ipod in R/W mode"), LOGERROR);
        emit done(true);
        return;
    }
    QCoreApplication::processEvents();

    m_tempfile.open();
    QString blfile = m_tempfile.fileName();
    m_tempfile.close();
    if(add_bootloader(&ipod, blfile.toLatin1().data(), FILETYPE_DOT_IPOD) == 0) {
        emit logItem(tr("Successfull added bootloader"), LOGOK);
        ipod_close(&ipod);
#if defined(Q_OS_MACX)
        m_remountDevice = ipod.diskname;
        connect(this, SIGNAL(remounted(bool)), this, SLOT(installStage3(bool)));
        waitRemount();
#else
        installStage3(true);
#endif
    }
    else {
        emit logItem(tr("Failed to add bootloader"), LOGERROR);
        ipod_close(&ipod);
        emit done(true);
        return;
    }
}


void BootloaderInstallIpod::installStage3(bool mounted)
{
    if(mounted) {
        logInstall(LogAdd);
        emit logItem(tr("Bootloader Installation complete."), LOGINFO);
        emit done(false);
        return;
    }
    else {
        emit logItem(tr("Writing log aborted"), LOGERROR);
        emit done(true);
    }
    LOG_INFO() << "version installed:"
               << m_blversion.toString(Qt::ISODate);
}


bool BootloaderInstallIpod::uninstall(void)
{
    emit logItem(tr("Uninstalling bootloader"), LOGINFO);
    QCoreApplication::processEvents();

    if(!ipodInitialize(&ipod)) {
        emit done(true);
        return false;
    }

    if (ipod.nimages <= 0) {
        emit logItem(tr("Failed to read firmware directory"),LOGERROR);
        emit done(true);
        return false;
    }
    if (getmodel(&ipod,(ipod.ipod_directory[ipod.ososimage].vers>>8)) < 0) {
        emit logItem(tr("Unknown version number in firmware (%1)").arg(
                    ipod.ipod_directory[ipod.ososimage].vers), LOGERROR);
        emit done(true);
        return false;
    }

    if (ipod_reopen_rw(&ipod) < 0) {
        emit logItem(tr("Could not open Ipod in R/W mode"), LOGERROR);
        emit done(true);
        return false;
    }

    if (ipod_has_bootloader(&ipod) == 0) {
        emit logItem(tr("No bootloader detected."), LOGERROR);
        emit done(true);
        return false;
    }

    if (delete_bootloader(&ipod)==0) {
        emit logItem(tr("Successfully removed bootloader"), LOGOK);
        logInstall(LogRemove);
        emit logProgress(1, 1);
        emit done(false);
        ipod_close(&ipod);
        return true;
    }
    else {
        emit logItem(tr("Removing bootloader failed."), LOGERROR);
        emit done(true);
        ipod_close(&ipod);
        return false;
    }
}


BootloaderInstallBase::BootloaderType BootloaderInstallIpod::installed(void)
{
    BootloaderInstallBase::BootloaderType result = BootloaderRockbox;

    if(!ipodInitialize(&ipod)) {
        LOG_INFO() << "installed: BootloaderUnknown";
        result = BootloaderUnknown;
    }
    else {
        read_directory(&ipod);
        getmodel(&ipod,(ipod.ipod_directory[ipod.ososimage].vers>>8));
        if(!ipod_has_bootloader(&ipod)) {
            result = BootloaderOther;
        }
        else {
            LOG_INFO() << "installed: BootloaderRockbox";
        }
    }
    ipod_close(&ipod);

    return result;
}


BootloaderInstallBase::Capabilities BootloaderInstallIpod::capabilities(void)
{
    return (Install | Uninstall | IsRaw);
}


/** @initialize Ipod by opening its file handle and checking if its an ipod.
 *  Note: the caller has to make sure the file handle gets closed!
 */
bool BootloaderInstallIpod::ipodInitialize(struct ipod_t *ipod)
{
    if(!m_blfile.isEmpty()) {
        QString devicename = Utils::resolveDevicename(m_blfile);
        if(devicename.isEmpty()) {
            emit logItem(tr("Error: could not retrieve device name"), LOGERROR);
            return false;
        }
#if defined(Q_OS_WIN32)
        sprintf(ipod->diskname, "\\\\.\\PhysicalDrive%i", devicename.toInt());
#elif defined(Q_OS_MACX)
        sprintf(ipod->diskname, "%s",
            qPrintable(devicename.remove(QRegExp("s[0-9]+$"))));
#else
        sprintf(ipod->diskname, "%s",
            qPrintable(devicename.remove(QRegExp("[0-9]+$"))));
#endif
        LOG_INFO() << "ipodpatcher: overriding scan, using"
                   << ipod->diskname;
    }
    else {
        emit logItem(tr("Error: no mountpoint specified!"), LOGERROR);
        LOG_ERROR() << "no mountpoint specified!";
    }
    int result = ipod_open(ipod, 1);
    if(result == -2) {
        emit logItem(tr("Could not open Ipod: permission denied"), LOGERROR);
        return false;
    }
    else if(result < 0) {
        emit logItem(tr("Could not open Ipod"), LOGERROR);
        return false;
    }

    if(read_partinfo(ipod, 1) < 0) {
        emit logItem(tr("Error reading partition table - possibly not an Ipod"), LOGERROR);
        ipod_close(ipod);
        return false;
    }

    if(ipod->pinfo[0].start == 0) {
        emit logItem(tr("No firmware partition on disk"), LOGERROR);
        ipod_close(ipod);
        return false;
    }
    read_directory(ipod);
    return true;
}

