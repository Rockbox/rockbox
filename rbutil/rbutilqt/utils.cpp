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

#include <QDir>

#if defined(Q_OS_WIN32)
#if defined(UNICODE)
#define _UNICODE
#endif
#include <windows.h>
#include <tchar.h>
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


//Function to get the system proxy
QUrl systemProxy(void)
{
#if defined(Q_OS_LINUX)
    return QUrl(getenv("http_proxy"));
#endif
#if defined(Q_OS_WIN32)
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
#endif
}
