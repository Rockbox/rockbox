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
#include "bootloaderinstallmi4.h"
#include "utils.h"

BootloaderInstallMi4::BootloaderInstallMi4(QObject *parent)
        : BootloaderInstallBase(parent)
{
}


bool BootloaderInstallMi4::install(void)
{
    emit logItem(tr("Downloading bootloader"), LOGINFO);
    qDebug() << __func__;
    downloadBlStart(m_blurl);
    connect(this, SIGNAL(downloadDone()), this, SLOT(installStage2()));
    return true;
}

void BootloaderInstallMi4::installStage2(void)
{
    emit logItem(tr("Installing Rockbox bootloader"), LOGINFO);

    // move old bootloader out of the way
    QString fwfile(resolvePathCase(m_blfile));
    QFile oldbl(fwfile);
    QString moved = QFileInfo(resolvePathCase(m_blfile)).absolutePath()
                        + "/OF.mi4";
    qDebug() << "renaming" << fwfile << "->" << moved;
    oldbl.rename(moved);

    // place new bootloader
    m_tempfile.open();
    qDebug() << "renaming" << m_tempfile.fileName() << "->" << fwfile;
    m_tempfile.close();
    m_tempfile.rename(fwfile);

    emit logItem(tr("Bootloader successful installed"), LOGOK);
    logInstall(LogAdd);

    emit done(true);
}


bool BootloaderInstallMi4::uninstall(void)
{
    qDebug() << __func__;

    // check if it's actually a Rockbox bootloader
    emit logItem(tr("Checking for Rockbox bootloader"), LOGINFO);
    if(installed() != BootloaderRockbox) {
        emit logItem(tr("No Rockbox bootloader found"), LOGERROR);
        return false;
    }

    // check if OF file present
    emit logItem(tr("Checking for original firmware file"), LOGINFO);
    QString original = QFileInfo(resolvePathCase(m_blfile)).absolutePath()
                        + "/OF.mi4";

    if(resolvePathCase(original).isEmpty()) {
        emit logItem(tr("Error finding original firmware file"), LOGERROR);
        return false;
    }

    // finally remove RB bootloader
    QString resolved = resolvePathCase(m_blfile);
    QFile blfile(resolved);
    blfile.remove();

    QFile oldbl(resolvePathCase(original));
    oldbl.rename(m_blfile);
    emit logItem(tr("Rockbox bootloader successful removed"), LOGINFO);
    logInstall(LogRemove);
    emit done(false);

    return true;
}


//! check if a bootloader is installed and return its state.
BootloaderInstallBase::BootloaderType BootloaderInstallMi4::installed(void)
{
    // for MI4 files we can check if we actually have a RB bootloader
    // installed.
    // RB bootloader has "RBBL" at 0x1f8 in the mi4 file.

    // make sure to resolve case to prevent case issues
    QString resolved;
    resolved = resolvePathCase(m_blfile);
    if(resolved.isEmpty()) {
        qDebug("%s: BootloaderNone", __func__);
        return BootloaderNone;
    }

    QFile f(resolved);
    f.open(QIODevice::ReadOnly);
    f.seek(0x1f8);
    char magic[4];
    f.read(magic, 4);
    f.close();

    if(!memcmp(magic, "RBBL", 4)) {
        qDebug("%s: BootloaderRockbox", __func__);
        return BootloaderRockbox;
    }
    else {
        qDebug("%s: BootloaderOther", __func__);
        return BootloaderOther;
    }
}


BootloaderInstallBase::Capabilities BootloaderInstallMi4::capabilities(void)
{
    qDebug() << __func__;
    return Install | Uninstall | Backup | IsFile | CanCheckInstalled | CanCheckVersion;
}

