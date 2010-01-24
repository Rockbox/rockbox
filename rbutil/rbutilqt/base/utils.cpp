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

#include "utils.h"
#include "system.h"
#include "rbsettings.h"
#include "systeminfo.h"

#ifdef UNICODE
#define _UNICODE
#endif

#include <QtCore>
#include <QDebug>
#include <cstdlib>
#include <stdio.h>

#if defined(Q_OS_WIN32)
#include <windows.h>
#include <tchar.h>
#include <winioctl.h>
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
#include <sys/statvfs.h>
#endif

// recursive function to delete a dir with files
bool recRmdir( const QString &dirName )
{
    QString dirN = dirName;
    QDir dir(dirN);
    // make list of entries in directory
    QStringList list = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
    QFileInfo fileInfo;
    QString curItem, lstAt;
    for(int i = 0; i < list.size(); i++){ // loop through all items of list
        QString name = list.at(i);
        curItem = dirN + "/" + name;
        fileInfo.setFile(curItem);
        if(fileInfo.isDir())    // is directory
            recRmdir(curItem);  // call recRmdir() recursively for deleting subdirectory
        else                    // is file
            QFile::remove(curItem); // ok, delete file
    }
    dir.cdUp();
    return dir.rmdir(dirN); // delete empty dir and return if (now empty) dir-removing was successfull
}


//! @brief resolves the given path, ignoring case.
//! @param path absolute path to resolve.
//! @return returns exact casing of path, empty string if path not found.
QString resolvePathCase(QString path)
{
    QStringList elems;
    QString realpath;

    elems = path.split("/", QString::SkipEmptyParts);
    int start;
#if defined(Q_OS_WIN32)
    // on windows we must make sure to start with the first entry (i.e. the
    // drive letter) instead of a single / to make resolving work.
    start = 1;
    realpath = elems.at(0) + "/";
#else
    start = 0;
    realpath = "/";
#endif

    for(int i = start; i < elems.size(); i++) {
        QStringList direlems
            = QDir(realpath).entryList(QDir::AllEntries|QDir::Hidden|QDir::System);
        if(direlems.contains(elems.at(i), Qt::CaseInsensitive)) {
            // need to filter using QRegExp as QStringList::filter(QString)
            // matches any substring
            QString expr = QString("^" + elems.at(i) + "$");
            QRegExp rx = QRegExp(expr, Qt::CaseInsensitive);
            QStringList a = direlems.filter(rx);

            if(a.size() != 1)
                return QString("");
            if(!realpath.endsWith("/"))
                realpath += "/";
            realpath += a.at(0);
        }
        else
            return QString("");
    }
    qDebug() << "[Utils] resolving path" << path << "->" << realpath;
    return realpath;
}


//! @brief figure the free disk space on a filesystem
//! @param path path on the filesystem to check
//! @return size in bytes
qulonglong filesystemFree(QString path)
{
    qlonglong size = 0;
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX) 
    // the usage of statfs() is deprecated by the LSB so use statvfs().
    struct statvfs fs;
    int ret;

    ret = statvfs(qPrintable(path), &fs);

    if(ret == 0)
        size = (qulonglong)fs.f_frsize * (qulonglong)fs.f_bavail;
#endif
#if defined(Q_OS_WIN32)
    BOOL ret;
    ULARGE_INTEGER freeAvailBytes;

    ret = GetDiskFreeSpaceExW((LPCTSTR)path.utf16(), &freeAvailBytes, NULL, NULL);
    if(ret)
        size = freeAvailBytes.QuadPart;
#endif
    return size;
}

//! \brief searches for a Executable in the Environement Path
QString findExecutable(QString name)
{
    QString exepath;
    //try autodetect tts   
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX) || defined(Q_OS_OPENBSD)
    QStringList path = QString(getenv("PATH")).split(":", QString::SkipEmptyParts);
#elif defined(Q_OS_WIN)
    QStringList path = QString(getenv("PATH")).split(";", QString::SkipEmptyParts);
#endif
    qDebug() << "[Utils] system path:" << path;
    for(int i = 0; i < path.size(); i++) 
    {
        QString executable = QDir::fromNativeSeparators(path.at(i)) + "/" + name;
#if defined(Q_OS_WIN)
        executable += ".exe";
        QStringList ex = executable.split("\"", QString::SkipEmptyParts);
        executable = ex.join("");
#endif
        qDebug() << "[Utils] executable:" << executable;
        if(QFileInfo(executable).isExecutable())
        {
            return QDir::toNativeSeparators(executable);
        }
    }
    return "";
}


/** @brief checks different Enviroment things. Ask if user wants to continue.
 *  @param settings A pointer to rbutils settings class
 *  @param permission if it should check for permission
 *  @param targetId the targetID to check for. if it is -1 no check is done.
 *  @return string with error messages if problems occurred, empty strings if none.
 */
QString check(bool permission)
{
    QString text = "";

    // check permission
    if(permission)
    {
#if defined(Q_OS_WIN32)
        if(System::userPermissions() != System::ADMIN)
        {
            text += QObject::tr("<li>Permissions insufficient for bootloader "
                    "installation.\nAdministrator priviledges are necessary.</li>");
        }
#endif
    }

    // Check TargetId
    RockboxInfo rbinfo(RbSettings::value(RbSettings::Mountpoint).toString());
    QString installed = rbinfo.target();
    if(!installed.isEmpty() && installed != SystemInfo::value(SystemInfo::CurConfigureModel).toString())
    {
        text += QObject::tr("<li>Target mismatch detected.\n"
                "Installed target: %1, selected target: %2.</li>")
            .arg(installed, SystemInfo::value(SystemInfo::CurPlatformName).toString());
            // FIXME: replace installed by human-friendly name
    }

    if(!text.isEmpty())
        return QObject::tr("Problem detected:") + "<ul>" + text + "</ul>";
    else
        return text;
}


RockboxInfo::RockboxInfo(QString mountpoint)
{
    qDebug() << "[RockboxInfo] trying to find rockbox-info at" << mountpoint;
    QFile file(mountpoint + "/.rockbox/rockbox-info.txt");
    m_success = false;
    if(!file.exists())
        return;

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    // read file contents
    while (!file.atEnd())
    {
        QString line = file.readLine();

        if(line.contains("Version:"))
        {
            m_version = line.remove("Version:").trimmed();
        }
        else if(line.contains("Target: "))
        {
            m_target = line.remove("Target: ").trimmed();
        }
        else if(line.contains("Features:"))
        {
            m_features = line.remove("Features:").trimmed();
        }
        else if(line.contains("Target id:"))
        {
            m_targetid = line.remove("Target id:").trimmed();
        }
        else if(line.contains("Memory:"))
        {
            m_ram = line.remove("Memory:").trimmed().toInt();
        }
    }

    file.close();
    m_success = true;
    return;
}

