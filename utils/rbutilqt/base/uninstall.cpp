/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtCore>
#include "uninstall.h"
#include "utils.h"
#include "Logger.h"

Uninstaller::Uninstaller(QObject* parent,QString mountpoint): QObject(parent)
{
    m_mountpoint = mountpoint;
}

void Uninstaller::deleteAll(void)
{
    QString rbdir(m_mountpoint + ".rockbox/");
    emit logItem(tr("Starting Uninstallation"), LOGINFO);
    emit logProgress(0, 0);
    Utils::recursiveRmdir(rbdir);
    emit logProgress(1, 1);
    emit logItem(tr("Finished Uninstallation"), LOGOK);
    emit logFinished();
}

void Uninstaller::uninstall(void)
{
    emit logProgress(0, 0);
    emit logItem(tr("Starting Uninstallation"), LOGINFO);

    QSettings installlog(m_mountpoint + "/.rockbox/rbutil.log", QSettings::IniFormat, this);

    for(int i=0; i< uninstallSections.size() ; i++)
    {
        emit logItem(tr("Uninstalling %1...").arg(uninstallSections.at(i)), LOGINFO);
        QCoreApplication::processEvents();
        // create list of all other install sections
        QStringList sections = installlog.childGroups();
        sections.removeAt(sections.indexOf(uninstallSections.at(i)));
        installlog.beginGroup(uninstallSections.at(i));
        QStringList toDeleteList = installlog.allKeys();
        QStringList dirList;
        installlog.endGroup();

        // iterate over all entries
        for(int j =0; j < toDeleteList.size(); j++ )
        {
            // check if current file is in use by another section
            bool deleteFile = true;
            for(int s = 0; s < sections.size(); s++)
            {
                installlog.beginGroup(sections.at(s));
                if(installlog.contains(toDeleteList.at(j)))
                {
                    deleteFile = false;
                    LOG_INFO() << "file still in use:" << toDeleteList.at(j);
                }
                installlog.endGroup();
            }

            installlog.beginGroup(uninstallSections.at(i));
            QFileInfo toDelete(m_mountpoint + "/" + toDeleteList.at(j));
            if(toDelete.isFile())  // if it is a file remove it
            {
                if(deleteFile && !QFile::remove(toDelete.filePath()))
                    emit logItem(tr("Could not delete %1")
                          .arg(toDelete.filePath()), LOGWARNING);
                installlog.remove(toDeleteList.at(j));
                LOG_INFO() << "deleted:" << toDelete.filePath();
            }
            else  // if it is a dir, remember it for later deletion
            {
                // no need to keep track on folders still in use -- only empty
                // folders will be rm'ed.
                dirList << toDeleteList.at(j);
            }
            installlog.endGroup();
            QCoreApplication::processEvents();
        }
        // delete the dirs
        installlog.beginGroup(uninstallSections.at(i));
        for(int j=0; j < dirList.size(); j++ )
        {
            installlog.remove(dirList.at(j));
            QDir dir(m_mountpoint);
            dir.rmdir(dirList.at(j)); // rm works only on empty folders
        }

        installlog.endGroup();
        //installlog.removeGroup(uninstallSections.at(i))
    }
    uninstallSections.clear();
    installlog.sync();
    emit logProgress(1, 1);
    emit logItem(tr("Uninstallation finished"), LOGOK);
    emit logFinished();
}

QStringList Uninstaller::getAllSections()
{
    QSettings installlog(m_mountpoint + "/.rockbox/rbutil.log", QSettings::IniFormat, nullptr);
    QStringList allSections = installlog.childGroups();
    allSections.removeAt(allSections.lastIndexOf("Bootloader"));
    return allSections;
}


bool Uninstaller::uninstallPossible()
{
    return QFileInfo::exists(m_mountpoint +"/.rockbox/rbutil.log");
}

