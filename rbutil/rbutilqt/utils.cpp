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

#include <cstdlib>

#include <QDir>

#if defined(Q_OS_WIN32)
#if defined(UNICODE)
#define _UNICODE
#endif
#include <windows.h>
#include <tchar.h>
#endif
#include <QDebug>

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


//! @brief get system proxy value.
QUrl systemProxy(void)
{
#if defined(Q_OS_LINUX)
    return QUrl(getenv("http_proxy"));
#elif defined(Q_OS_WIN32)
    HKEY hk;
    wchar_t proxyval[80];
    DWORD buflen = 80;
    long ret;
    DWORD enable;
    DWORD enalen = sizeof(DWORD);

    ret = RegOpenKeyEx(HKEY_CURRENT_USER,
        _TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"),
        0, KEY_QUERY_VALUE, &hk);
    if(ret != ERROR_SUCCESS) return QUrl("");

    ret = RegQueryValueEx(hk, _TEXT("ProxyServer"), NULL, NULL, (LPBYTE)proxyval, &buflen);
    if(ret != ERROR_SUCCESS) return QUrl("");
    
    ret = RegQueryValueEx(hk, _TEXT("ProxyEnable"), NULL, NULL, (LPBYTE)&enable, &enalen);
    if(ret != ERROR_SUCCESS) return QUrl("");

    RegCloseKey(hk);

    //qDebug() << QString::fromWCharArray(proxyval) << QString("%1").arg(enable);
    if(enable != 0)
        return QUrl("http://" + QString::fromWCharArray(proxyval));
    else
        return QUrl("");      
#else
    return QUrl("");        
#endif
}

QString installedVersion(QString mountpoint)
{
    // read rockbox-info.txt
    QFile info(mountpoint +"/.rockbox/rockbox-info.txt");
    if(!info.open(QIODevice::ReadOnly))
    {
        return "";
    }
    
    QString target, features,version;
    while (!info.atEnd()) {
        QString line = info.readLine();
        
        if(line.contains("Version:"))
        {
            return line.remove("Version:").trimmed();
        }        
    }
    info.close();
    return "";
}

