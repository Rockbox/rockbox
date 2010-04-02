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


#include "system.h"

#include <QtCore>
#include <QDebug>

#include <cstdlib>
#include <stdio.h>

// Windows Includes
#if defined(Q_OS_WIN32)
#if defined(UNICODE)
#define _UNICODE
#endif
#include <windows.h>
#include <tchar.h>
#include <lm.h>
#include <windows.h>
#include <setupapi.h>
#endif

// Linux and Mac includes
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
#if defined(LIBUSB1)
#include <libusb-1.0/libusb.h>
#else
#include <usb.h>
#endif
#include <sys/utsname.h>
#include <unistd.h>
#include <pwd.h>
#endif

// Linux includes
#if defined(Q_OS_LINUX)
#include <mntent.h>
#endif

// Mac includes
#if defined(Q_OS_MACX)
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>

#include <CoreFoundation/CoreFoundation.h>
#include <SystemConfiguration/SystemConfiguration.h>
#endif

#include "utils.h"
#include "rbsettings.h"

/** @brief detect permission of user (only Windows at moment).
 *  @return enum userlevel.
 */
#if defined(Q_OS_WIN32)
enum System::userlevel System::userPermissions(void)
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

/** @brief detects user permissions (only Windows at moment).
 *  @return a user readable string with the permission.
 */
QString System::userPermissionsString(void)
{
    QString result;
    int perm = userPermissions();
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


/** @brief detects current Username.
 *  @return string with Username.
 */
QString System::userName(void)
{
#if defined(Q_OS_WIN32)
    wchar_t userbuf[UNLEN];
    DWORD usersize = UNLEN;
    BOOL status;

    status = GetUserNameW(userbuf, &usersize);

    return QString::fromWCharArray(userbuf);
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
    struct passwd *user;
    user = getpwuid(geteuid());
    return QString(user->pw_name);
#endif
}


/** @brief detects the OS Version
 *  @return String with OS Version.
 */
QString System::osVersionString(void)
{
    QString result;
#if defined(Q_OS_WIN32)
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

    result = QString("Windows version %1.%2, ").arg(osvi.dwMajorVersion).arg(osvi.dwMinorVersion);
    if(osvi.szCSDVersion)
        result += QString("build %1 (%2)").arg(osvi.dwBuildNumber)
            .arg(QString::fromWCharArray(osvi.szCSDVersion));
    else
        result += QString("build %1").arg(osvi.dwBuildNumber);
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
    struct utsname u;
    int ret;
    ret = uname(&u);

    result = QString("CPU: %1<br/>System: %2<br/>Release: %3<br/>Version: %4")
        .arg(u.machine).arg(u.sysname).arg(u.release).arg(u.version);
#endif
    result += QString("<br/>Qt version %1").arg(qVersion());
    return result;
}

QList<uint32_t> System::listUsbIds(void)
{
    return listUsbDevices().keys();
}

/** @brief detect devices based on usb pid / vid.
 *  @return list with usb VID / PID values.
 */
QMap<uint32_t, QString> System::listUsbDevices(void)
{
    QMap<uint32_t, QString> usbids;
    // usb pid detection
    qDebug() << "[System] Searching for USB devices";
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
#if defined(LIBUSB1)
    libusb_device **devs;
    int res;
    ssize_t count;
    res = libusb_init(NULL);

    count = libusb_get_device_list(NULL, &devs);
    libusb_device *dev;
    int i = 0;
    while((dev = devs[i++]) != NULL) {
        QString name;
        unsigned char buf[256];
        uint32_t id;
        struct libusb_device_descriptor descriptor;
        if(libusb_get_device_descriptor(dev, &descriptor) == 0) {
            id = descriptor.idVendor << 16 | descriptor.idProduct;

            libusb_device_handle *dh;
            if(libusb_open(dev, &dh) == 0) {
                libusb_get_string_descriptor_ascii(dh, descriptor.iManufacturer, buf, 256);
                name += QString::fromAscii((char*)buf) + " ";
                libusb_get_string_descriptor_ascii(dh, descriptor.iProduct, buf, 256);
                name += QString::fromAscii((char*)buf);
                libusb_close(dh);
            }
            if(name.isEmpty())
                name = QObject::tr("(no description available)");
            if(id) {
                usbids.insert(id, name);
                qDebug("[System] USB: 0x%08x, %s", id, name.toLocal8Bit().data());
            }
        }
    }

    libusb_free_device_list(devs, 1);
    libusb_exit(NULL);
#else
    usb_init();
    usb_find_busses();
    usb_find_devices();
    struct usb_bus *b;
    b = usb_busses;

    while(b) {
        if(b->devices) {
            struct usb_device *u;
            u = b->devices;
            while(u) {
                uint32_t id;
                id = u->descriptor.idVendor << 16 | u->descriptor.idProduct;
                // get identification strings
                usb_dev_handle *dev;
                QString name;
                char string[256];
                int res;
                dev = usb_open(u);
                if(dev) {
                    if(u->descriptor.iManufacturer) {
                        res = usb_get_string_simple(dev, u->descriptor.iManufacturer,
                                                    string, sizeof(string));
                        if(res > 0)
                            name += QString::fromAscii(string) + " ";
                    }
                    if(u->descriptor.iProduct) {
                        res = usb_get_string_simple(dev, u->descriptor.iProduct,
                                                    string, sizeof(string));
                        if(res > 0)
                            name += QString::fromAscii(string);
                    }
                }
                usb_close(dev);
                if(name.isEmpty()) name = QObject::tr("(no description available)");

                if(id) {
                    usbids.insert(id, name);
                    qDebug() << "[System] USB:" << QString("0x%1").arg(id, 8, 16) << name;
                }
                u = u->next;
            }
        }
        b = b->next;
    }
#endif
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
        QString description;

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
            SPDRP_LOCATION_INFORMATION,&data, (PBYTE)buffer, buffersize, &buffersize)) {
            if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                if(buffer) free(buffer);
                // double buffer size to avoid problems as per KB888609
                buffer = (LPTSTR)malloc(buffersize * 2);
            }
            else {
                break;
            }
        }
        description = QString::fromWCharArray(buffer);

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

        unsigned int vid, pid;
        if(_stscanf(buffer, _TEXT("USB\\Vid_%x&Pid_%x"), &vid, &pid) == 2) {
            uint32_t id;
            id = vid << 16 | pid;
            usbids.insert(id, description);
            qDebug("[System] USB VID: %04x, PID: %04x", vid, pid);
        }
        if(buffer) free(buffer);
    }
    SetupDiDestroyDeviceInfoList(deviceInfo);

#endif
    return usbids;
}


/** @brief detects current system proxy
 *  @return QUrl with proxy or empty
 */
QUrl System::systemProxy(void)
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
#elif defined(Q_OS_MACX)

    CFDictionaryRef dictref;
    CFStringRef stringref;
    CFNumberRef numberref;
    int enable;
    int port;
    unsigned int bufsize = 0;
    char *buf;
    QUrl proxy;

    dictref = SCDynamicStoreCopyProxies(NULL);
    stringref = (CFStringRef)CFDictionaryGetValue(dictref, kSCPropNetProxiesHTTPProxy);
    numberref = (CFNumberRef)CFDictionaryGetValue(dictref, kSCPropNetProxiesHTTPEnable);
    CFNumberGetValue(numberref, kCFNumberIntType, &enable);
    if(enable == 1) {
        // get number of characters. CFStringGetLength uses UTF-16 code pairs
        bufsize = CFStringGetLength(stringref) * 2 + 1;
        buf = (char*)malloc(sizeof(char) * bufsize);
        if(buf == NULL) {
            qDebug() << "[System] can't allocate memory for proxy string!";
            CFRelease(dictref);
            return QUrl("");
        }
        CFStringGetCString(stringref, buf, bufsize, kCFStringEncodingUTF16);
        numberref = (CFNumberRef)CFDictionaryGetValue(dictref, kSCPropNetProxiesHTTPPort);
        CFNumberGetValue(numberref, kCFNumberIntType, &port);
        proxy.setScheme("http");
        proxy.setHost(QString::fromUtf16((unsigned short*)buf));
        proxy.setPort(port);

        free(buf);
    }
    CFRelease(dictref);

    return proxy;
#else
    return QUrl("");
#endif
}


