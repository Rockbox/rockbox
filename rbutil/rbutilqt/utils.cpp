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

#include <QtCore>
#include <QDebug>
#include <cstdlib>
#include <stdio.h>

#if defined(Q_OS_WIN32)
#if defined(UNICODE)
#define _UNICODE
#endif
#include <windows.h>
#include <tchar.h>
#include <lm.h>
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
#include <usb.h>
#include <sys/utsname.h>
#include <unistd.h>
#endif
#if defined(Q_OS_LINUX)
#include <mntent.h>
#endif
#if defined(Q_OS_MACX)
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#endif
#if defined(Q_OS_WIN32)
#if defined(UNICODE)
#define _UNICODE
#endif
#include <tchar.h>
#include <windows.h>
#include <setupapi.h>
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


QString getUserName(void)
{
#if defined(Q_OS_WIN32)
    wchar_t userbuf[UNLEN];
    DWORD usersize = UNLEN;
    BOOL status;
 
    status = GetUserNameW(userbuf, &usersize);

    return QString::fromWCharArray(userbuf);
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
    return QString(getlogin());
#endif
}


#if defined(Q_OS_WIN32)
enum userlevel getUserPermissions(void)
{
    LPUSER_INFO_1 buf;
    NET_API_STATUS napistatus;
    wchar_t userbuf[UNLEN];
    DWORD usersize = UNLEN;
    BOOL status;
    enum userlevel result;

    status = GetUserNameW(userbuf, &usersize);
    if(!status)
        return ERR;
 
    napistatus = NetUserGetInfo(NULL, userbuf, (DWORD)1, (LPBYTE*)&buf);

    switch(buf->usri1_priv) {
        case USER_PRIV_GUEST:
            result = GUEST;
            break;
        case USER_PRIV_USER:
            result = USER;
            break;
        case USER_PRIV_ADMIN:
            result = ADMIN;
            break;
        default:
            result = ERR;
            break;
    }
    NetApiBufferFree(buf);

    return result;
}

QString getUserPermissionsString(void)
{
    QString result;
    int perm = getUserPermissions();
    switch(perm) {
        case GUEST:
            result = QObject::tr("Guest");
            break;
        case ADMIN:
            result = QObject::tr("Admin");
            break;
        case USER:
            result = QObject::tr("User");
            break;
        default:
            result = QObject::tr("Error");
            break;
    }
    return result;
}
#endif

QString getOsVersionString(void)
{
    QString result;
#if defined(Q_OS_WIN32)
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

    result = QString("Windows version %1.%2, ").arg(osvi.dwMajorVersion).arg(osvi.dwMinorVersion);
    result += QString("build %1 (%2)").arg(osvi.dwBuildNumber).arg(QString::fromWCharArray(osvi.szCSDVersion));
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
    struct utsname u;
    int ret;
    ret = uname(&u);

    result = QString("CPU: %1<br/>System: %2<br/>Release: %3<br/>Version: %4")
        .arg(u.machine).arg(u.sysname).arg(u.release).arg(u.version);
#endif
    return result;

}

/** @brief detect devices based on usb pid / vid.
 *  @return list with usb VID / PID values.
 */
QList<uint32_t> listUsbIds(void)
{
    QList<uint32_t> usbids;
    // usb pid detection
#if defined(Q_OS_LINUX) | defined(Q_OS_MACX)
    usb_init();
    usb_find_busses();
    usb_find_devices();
    struct usb_bus *b;
    b = usb_busses;

    while(b) {
        qDebug() << "bus:" << b->dirname << b->devices;
        if(b->devices) {
            qDebug() << "devices present.";
            struct usb_device *u;
            u = b->devices;
            while(u) {
                uint32_t id;
                id = u->descriptor.idVendor << 16 | u->descriptor.idProduct;
                if(id) usbids.append(id);
                u = u->next;
            }
        }
        b = b->next;
    }
#endif

#if defined(Q_OS_WIN32)
    HDEVINFO deviceInfo;
    SP_DEVINFO_DATA infoData;
    DWORD i;

    // Iterate over all devices
    // by doing it this way it's unneccessary to use GUIDs which might be not
    // present in current MinGW. It also seemed to be more reliably than using
    // a GUID.
    // See KB259695 for an example.
    deviceInfo = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);

    infoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for(i = 0; SetupDiEnumDeviceInfo(deviceInfo, i, &infoData); i++) {
        DWORD data;
        LPTSTR buffer = NULL;
        DWORD buffersize = 0;

        // get device desriptor first
        // for some reason not doing so results in bad things (tm)
        while(!SetupDiGetDeviceRegistryProperty(deviceInfo, &infoData,
            SPDRP_DEVICEDESC,&data, (PBYTE)buffer, buffersize, &buffersize)) {
            if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                if(buffer) free(buffer);
                // double buffer size to avoid problems as per KB888609
                buffer = (LPTSTR)malloc(buffersize * 2);
            }
            else {
                break;
            }
        }

        // now get the hardware id, which contains PID and VID.
        while(!SetupDiGetDeviceRegistryProperty(deviceInfo, &infoData,
            SPDRP_HARDWAREID,&data, (PBYTE)buffer, buffersize, &buffersize)) {
            if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                if(buffer) free(buffer);
                // double buffer size to avoid problems as per KB888609
                buffer = (LPTSTR)malloc(buffersize * 2);
            }
            else {
                break;
            }
        }

        unsigned int vid, pid, rev;
        if(_stscanf(buffer, _TEXT("USB\\Vid_%x&Pid_%x&Rev_%x"), &vid, &pid, &rev) != 3) {
            qDebug() << "Error getting USB ID -- possibly no USB device";
        }
        else {
            uint32_t id;
            id = vid << 16 | pid;
            usbids.append(id);
            qDebug("VID: %04x PID: %04x", vid, pid);
        }
        if(buffer) free(buffer);
    }
    SetupDiDestroyDeviceInfoList(deviceInfo);

#endif
    return usbids;
}
