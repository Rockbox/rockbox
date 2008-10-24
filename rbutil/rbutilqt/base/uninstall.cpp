/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
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
#include "uninstall.h"
#include "utils.h"

Uninstaller::Uninstaller(QObject* parent,QString mountpoint): QObject(parent)
{
    m_mountpoint = mountpoint;
}

void Uninstaller::deleteAll(ProgressloggerInterface* dp)
{
    m_dp = dp;
    QString rbdir(m_mountpoint + ".rockbox/");
    m_dp->addItem(tr("Starting Uninstallation"),LOGINFO);
    m_dp->setProgressMax(0);
    recRmdir(rbdir);
    m_dp->setProgressMax(1);
    m_dp->setProgressValue(1);
    m_dp->addItem(tr("Finished Uninstallation"),LOGOK);
    m_dp->abort();
}

void Uninstaller::uninstall(ProgressloggerInterface* dp)
{
    m_dp = dp;
    m_dp->setProgressMax(0);
    m_dp->addItem(tr("Starting Uninstallation"),LOGINFO);

    QSettings installlog(m_mountpoint + "/.rockbox/rbutil.log", QSettings::IniFormat, this);

    for(int i=0; i< uninstallSections.size() ; i++)
    {
        m_dp->addItem(tr("Uninstalling ") + uninstallSections.at(i) + " ...",LOGINFO);
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
                    qDebug() << "file still in use:" << toDeleteList.at(j);
                }
                installlog.endGroup();
            }

            installlog.beginGroup(uninstallSections.at(i));
            QFileInfo toDelete(m_mountpoint + "/" + toDeleteList.at(j));
            if(toDelete.isFile())  // if it is a file remove it
            {
                if(deleteFile && !QFile::remove(toDelete.filePath()))
                    m_dp->addItem(tr("Could not delete: ")+ toDelete.filePath(),LOGWARNING);
                installlog.remove(toDeleteList.at(j));
                qDebug() << "deleted: " << toDelete.filePath() ;
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
    m_dp->setProgressMax(1);
    m_dp->setProgressValue(1);
    m_dp->addItem(tr("Uninstallation finished"),LOGOK);
    m_dp->abort();
}

QStringList Uninstaller::getAllSections()
{
    QSettings installlog(m_mountpoint + "/.rockbox/rbutil.log", QSettings::IniFormat, 0);
    QStringList allSections = installlog.childGroups();
    allSections.removeAt(allSections.lastIndexOf("Bootloader"));
    return allSections;
}


bool Uninstaller::uninstallPossible()
{
    return QFileInfo(m_mountpoint +"/.rockbox/rbutil.log").exists();
}

