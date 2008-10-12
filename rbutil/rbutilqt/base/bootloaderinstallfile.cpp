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
#include <QtDebug>
#include <QtDebug>
#include "bootloaderinstallfile.h"
#include "utils.h"


BootloaderInstallFile::BootloaderInstallFile(QObject *parent)
        : BootloaderInstallBase(parent)
{
}


bool BootloaderInstallFile::install(void)
{
    emit logItem(tr("Downloading bootloader"), LOGINFO);
    qDebug() << __func__;
    downloadBlStart(m_blurl);
    connect(this, SIGNAL(downloadDone()), this, SLOT(installStage2()));
    return true;
}

void BootloaderInstallFile::installStage2(void)
{
    emit logItem(tr("Installing Rockbox bootloader"), LOGINFO);

    // if an old bootloader is present (Gigabeat) move it out of the way.
    QString fwfile(resolvePathCase(m_blfile));
    if(!fwfile.isEmpty()) {
        QString moved = resolvePathCase(m_blfile) + ".ORIG";
        qDebug() << "renaming" << fwfile << "->" << moved;
        QFile::rename(fwfile, moved);
    }

    // if no old file found resolve path without basename
    QFileInfo fi(m_blfile);
    QString absPath = resolvePathCase(fi.absolutePath());

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
        absPath = resolvePathCase(basePath);
        QDir d(absPath);
        d.mkpath(lastElement);
        absPath = resolvePathCase(fi.absolutePath());

        if(absPath.isEmpty()) {
            emit logItem(tr("Error accessing output folder"), LOGERROR);
            emit done(true);
            return;
        }
    }
    fwfile = absPath + "/" + fi.fileName();

    // place (new) bootloader
    m_tempfile.open();
    qDebug() << "renaming" << m_tempfile.fileName() << "->" << fwfile;
    m_tempfile.close();
    m_tempfile.rename(fwfile);

    emit logItem(tr("Bootloader successful installed"), LOGOK);
    logInstall(LogAdd);

    emit done(false);
}


bool BootloaderInstallFile::uninstall(void)
{
    qDebug() << __func__;
    emit logItem(tr("Removing Rockbox bootloader"), LOGINFO);
    // check if a .ORIG file is present, and allow moving it back.
    QString origbl = resolvePathCase(m_blfile + ".ORIG");
    if(origbl.isEmpty()) {
        emit logItem(tr("No original firmware file found."), LOGERROR);
        emit done(true);
        return false;
    }
    QString fwfile = resolvePathCase(m_blfile);
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
    emit done(false);

    return true;
}


//! @brief check if bootloader is installed.
//! @return BootloaderRockbox, BootloaderOther or BootloaderUnknown.
BootloaderInstallBase::BootloaderType BootloaderInstallFile::installed(void)
{
    qDebug("%s()", __func__);
    if(!resolvePathCase(m_blfile).isEmpty()
        && !resolvePathCase(m_blfile + ".ORIG").isEmpty())
        return BootloaderRockbox;
    else if(!resolvePathCase(m_blfile).isEmpty())
        return BootloaderOther;
    else
        return BootloaderUnknown;
}


BootloaderInstallBase::Capabilities BootloaderInstallFile::capabilities(void)
{
    qDebug() << __func__;
    return Install | IsFile | CanCheckInstalled | Backup;
}

