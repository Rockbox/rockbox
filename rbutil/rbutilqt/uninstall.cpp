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

    QSettings installlog(m_mountpoint + "/.rockbox/rbutil.log", QSettings::IniFormat, 0);

    for(int i=0; i< uninstallSections.size() ; i++)
    {
        m_dp->addItem(tr("Uninstalling ") + uninstallSections.at(i) + " ...",LOGINFO);
        installlog.beginGroup(uninstallSections.at(i));
        QStringList toDeleteList = installlog.allKeys();
        QStringList dirList;

        // iterate over all entrys
        for(int j =0; j < toDeleteList.size(); j++ )
        {
            QFileInfo toDelete(m_mountpoint + "/" + toDeleteList.at(j));
            if(toDelete.isFile())  // if it is a file remove it
            {
                if(!QFile::remove(toDelete.filePath()))
                    m_dp->addItem(tr("Could not delete: ")+ toDelete.filePath(),LOGWARNING);
                installlog.remove(toDeleteList.at(j));
            }
            else  // if it is a dir, remember it for later deletion
            {
                dirList << toDeleteList.at(j);
            }
        }
        // delete the dirs
        for(int j=0; j < dirList.size(); j++ )
        {
            installlog.remove(dirList.at(j));
            QDir dir(m_mountpoint);
            dir.rmdir(dirList.at(j));
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
