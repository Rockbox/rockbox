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
#include <QtDebug>
#include "bootloaderinstallfile.h"
#include "utils.h"
#include "Logger.h"


BootloaderInstallFile::BootloaderInstallFile(QObject *parent)
        : BootloaderInstallBase(parent)
{
}


bool BootloaderInstallFile::install(void)
{
    emit logItem(tr("Downloading bootloader"), LOGINFO);
    LOG_INFO() << "installing bootloader";
    downloadBlStart(m_blurl);
    connect(this, &BootloaderInstallBase::downloadDone, this, &BootloaderInstallFile::installStage2);
    return true;
}

void BootloaderInstallFile::installStage2(void)
{
    emit logItem(tr("Installing Rockbox bootloader"), LOGINFO);
    QCoreApplication::processEvents();

    // if an old bootloader is present (Gigabeat) move it out of the way.
    QString fwfile(Utils::resolvePathCase(m_blfile));
    if(!fwfile.isEmpty()) {
        QString moved = Utils::resolvePathCase(m_blfile) + ".ORIG";
        LOG_INFO() << "renaming" << fwfile << "to" << moved;
        QFile::rename(fwfile, moved);
    }

    // if no old file found resolve path without basename
    QFileInfo fi(m_blfile);
    QString absPath = Utils::resolvePathCase(fi.absolutePath());

    // if it's not possible to locate the base path try to create it
    if(absPath.isEmpty()) {
        QStringList pathElements = m_blfile.split("/");
        // remove filename from list and save last path element
        pathElements.removeLast();
        QString lastElement = pathElements.last();
        // remove last path element for base
        pathElements.removeLast();
        QString basePath = pathElements.join("/");

        // check for base and bail out if not found. Otherwise create folder.
        absPath = Utils::resolvePathCase(basePath);
        QDir d(absPath);
        d.mkpath(lastElement);
        absPath = Utils::resolvePathCase(fi.absolutePath());

        if(absPath.isEmpty()) {
            emit logItem(tr("Error accessing output folder"), LOGERROR);
            emit done(true);
            return;
        }
    }
    fwfile = absPath + "/" + fi.fileName();

    // place (new) bootloader
    m_tempfile.open();
    LOG_INFO() << "renaming" << m_tempfile.fileName()
               << "to" << fwfile;
    m_tempfile.close();

    if(!Utils::resolvePathCase(fwfile).isEmpty()) {
        emit logItem(tr("A firmware file is already present on player"), LOGERROR);
        emit done(true);
        return;
    }
    if(m_tempfile.copy(fwfile)) {
        emit logItem(tr("Bootloader successful installed"), LOGOK);
    }
    else {
        emit logItem(tr("Copying modified firmware file failed"), LOGERROR);
        emit done(true);
        return;
    }

    logInstall(LogAdd);

    emit done(false);
}


bool BootloaderInstallFile::uninstall(void)
{
    LOG_INFO() << "Uninstalling bootloader";
    emit logItem(tr("Removing Rockbox bootloader"), LOGINFO);
    // check if a .ORIG file is present, and allow moving it back.
    QString origbl = Utils::resolvePathCase(m_blfile + ".ORIG");
    if(origbl.isEmpty()) {
        emit logItem(tr("No original firmware file found."), LOGERROR);
        emit done(true);
        return false;
    }
    QString fwfile = Utils::resolvePathCase(m_blfile);
    if(!QFile::remove(fwfile)) {
        emit logItem(tr("Can't remove Rockbox bootloader file."), LOGERROR);
        emit done(true);
        return false;
    }
    if(!QFile::rename(origbl, fwfile)) {
        emit logItem(tr("Can't restore bootloader file."), LOGERROR);
        emit done(true);
        return false;
    }
    emit logItem(tr("Original bootloader restored successfully."), LOGOK);
    logInstall(LogRemove);
    emit logProgress(1, 1);
    emit done(false);

    return true;
}


//! @brief check if bootloader is installed.
//! @return BootloaderRockbox, BootloaderOther or BootloaderUnknown.
BootloaderInstallBase::BootloaderType BootloaderInstallFile::installed(void)
{
    LOG_INFO() << "checking installed bootloader";
    if(!Utils::resolvePathCase(m_blfile).isEmpty()
        && !Utils::resolvePathCase(m_blfile + ".ORIG").isEmpty())
        return BootloaderRockbox;
    else if(!Utils::resolvePathCase(m_blfile).isEmpty())
        return BootloaderOther;
    else
        return BootloaderUnknown;
}


BootloaderInstallBase::Capabilities BootloaderInstallFile::capabilities(void)
{
    LOG_INFO() << "getting capabilities";
    return Install | Uninstall | IsFile | CanCheckInstalled | Backup;
}

