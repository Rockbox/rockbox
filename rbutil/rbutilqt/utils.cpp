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
    qDebug() << __func__ << path << "->" << realpath;
    return realpath;
}

#if defined(Q_OS_WIN32)
QString getMountpointByDevice(int drive)
{
    QString result;
    for(int letter = 'A'; letter <= 'Z'; letter++) {
        DWORD written;
        HANDLE h;
        TCHAR uncpath[MAX_PATH];
        UCHAR buffer[0x400];
        PVOLUME_DISK_EXTENTS extents = (PVOLUME_DISK_EXTENTS)buffer;

        _stprintf(uncpath, _TEXT("\\\\.\\%c:"), letter);
        h = CreateFile(uncpath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL, OPEN_EXISTING, 0, NULL);
        if(h == INVALID_HANDLE_VALUE) {
            qDebug() << "error getting extents for" << uncpath;
            continue;
        }
        // get the extents
        if(DeviceIoControl(h, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                    NULL, 0, extents, sizeof(buffer), &written, NULL)) {
            for(int a = 0; a < extents->NumberOfDiskExtents; a++) {
                qDebug() << "Disk:" << extents->Extents[a].DiskNumber;
                if(extents->Extents[a].DiskNumber == drive) {
                    result = letter;
                    qDebug("found: %c", letter);
                    break;
                }
            }

        }

    }
    return result;

}
#endif

