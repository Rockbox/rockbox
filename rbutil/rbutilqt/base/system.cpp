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
#include <sys/utsname.h>
#include <unistd.h>
#include <pwd.h>
#endif

// Linux includes
#if defined(Q_OS_LINUX)
#include <libusb-1.0/libusb.h>
#include <mntent.h>
#endif

// Mac includes
#if defined(Q_OS_MACX)
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>

#include <CoreFoundation/CoreFoundation.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <CoreServices/CoreServices.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#endif

#include "utils.h"
#include "rbsettings.h"
#include "Logger.h"

/** @brief detect permission of user (only Windows at moment).
 *  @return enum userlevel.
 */
#if defined(Q_OS_WIN32)
enum System::userlevel System::userPermissions(void)
{
    LPUSER_INFO_1 buf = NULL;
    wchar_t userbuf[UNLEN];
    DWORD usersize = UNLEN;
    BOOL status;
    enum userlevel result = ERR;

    status = GetUserNameW(userbuf, &usersize);
    if(!status)
        return ERR;

    if(NetUserGetInfo(NULL, userbuf, (DWORD)1, (LPBYTE*)&buf) == NERR_Success) {
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
    }
    if(buf != NULL)
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
            result = tr("Guest");
            break;
        case ADMIN:
            result = tr("Admin");
            break;
        case USER:
            result = tr("User");
            break;
        default:
            result = tr("Error");
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

    if(GetUserNameW(userbuf, &usersize) == 0)
        return QString();

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
    SYSTEM_INFO sysinfo;
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    GetSystemInfo(&sysinfo);

    result = QString("Windows version %1.%2, ").arg(osvi.dwMajorVersion).arg(osvi.dwMinorVersion);
    if(osvi.szCSDVersion)
        result += QString("build %1 (%2)").arg(osvi.dwBuildNumber)
            .arg(QString::fromWCharArray(osvi.szCSDVersion));
    else
        result += QString("build %1").arg(osvi.dwBuildNumber);
    result += QString("<br/>CPU: %1, %2 processor(s)").arg(sysinfo.dwProcessorType)
              .arg(sysinfo.dwNumberOfProcessors);
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
    struct utsname u;
    int ret;
    ret = uname(&u);

#if defined(Q_OS_MACX)
    SInt32 cores;
    Gestalt(gestaltCountOfCPUs, &cores);
#else
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif
    if(ret != -1) {
        result = QString("CPU: %1, %2 processor(s)").arg(u.machine).arg(cores);
        result += QString("<br/>System: %2<br/>Release: %3<br/>Version: %4")
            .arg(u.sysname).arg(u.release).arg(u.version);
    }
    else {
        result = QString("(Error when retrieving system information)");
    }
#if defined(Q_OS_MACX)
    SInt32 major;
    SInt32 minor;
    SInt32 bugfix;
    Gestalt(gestaltSystemVersionMajor, &major);
    Gestalt(gestaltSystemVersionMinor, &minor);
    Gestalt(gestaltSystemVersionBugFix, &bugfix);

    result += QString("<br/>OS X %1.%2.%3 ").arg(major).arg(minor).arg(bugfix);
    // 1: 86k, 2: ppc, 10: i386
    SInt32 arch;
    Gestalt(gestaltSysArchitecture, &arch);
    switch(arch) {
        case 1:
            result.append("(86k)");
            break;
        case 2:
            result.append("(ppc)");
            break;
        case 10:
            result.append("(x86)");
            break;
        default:
            result.append("(unknown)");
            break;
    }
#endif
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
QMultiMap<uint32_t, QString> System::listUsbDevices(void)
{
    QMultiMap<uint32_t, QString> usbids;
    // usb pid detection
    LOG_INFO() << "Searching for USB devices";
#if defined(Q_OS_LINUX)
    libusb_device **devs;
    if(libusb_init(nullptr) != 0) {
        LOG_ERROR() << "Initializing libusb-1 failed.";
        return usbids;
    }

    if(libusb_get_device_list(nullptr, &devs) < 1) {
        LOG_ERROR() << "Error getting device list.";
        return usbids;
    }
    libusb_device *dev;
    int i = 0;
    while((dev = devs[i++]) != nullptr) {
        QString name;
        unsigned char buf[256];
        uint32_t id;
        struct libusb_device_descriptor descriptor;
        if(libusb_get_device_descriptor(dev, &descriptor) == 0) {
            id = descriptor.idVendor << 16 | descriptor.idProduct;

            libusb_device_handle *dh;
            if(libusb_open(dev, &dh) == 0) {
                libusb_get_string_descriptor_ascii(dh, descriptor.iManufacturer, buf, 256);
                name += QString::fromLatin1((char*)buf) + " ";
                libusb_get_string_descriptor_ascii(dh, descriptor.iProduct, buf, 256);
                name += QString::fromLatin1((char*)buf);
                libusb_close(dh);
            }
            if(name.isEmpty())
                name = tr("(no description available)");
            if(id) {
                usbids.insert(id, name);
                LOG_INFO("USB: 0x%08x, %s", id, name.toLocal8Bit().data());
            }
        }
    }

    libusb_free_device_list(devs, 1);
    libusb_exit(nullptr);
#endif

#if defined(Q_OS_MACX)
    kern_return_t result = KERN_FAILURE;
    CFMutableDictionaryRef usb_matching_dictionary;
    io_iterator_t usb_iterator = IO_OBJECT_NULL;
    usb_matching_dictionary = IOServiceMatching(kIOUSBDeviceClassName);
    result = IOServiceGetMatchingServices(kIOMasterPortDefault, usb_matching_dictionary,
                                          &usb_iterator);
    if(result) {
        LOG_ERROR() << "USB: IOKit: Could not get matching services.";
        return usbids;
    }

    io_object_t usbCurrentObj;
    while((usbCurrentObj = IOIteratorNext(usb_iterator))) {
        uint32_t id;
        QString name;
        /* get vendor ID */
        CFTypeRef vidref = NULL;
        int vid = 0;
        vidref = IORegistryEntryCreateCFProperty(usbCurrentObj, CFSTR("idVendor"),
                                kCFAllocatorDefault, 0);
        CFNumberGetValue((CFNumberRef)vidref, kCFNumberIntType, &vid);
        CFRelease(vidref);

        /* get product ID */
        CFTypeRef pidref = NULL;
        int pid = 0;
        pidref = IORegistryEntryCreateCFProperty(usbCurrentObj, CFSTR("idProduct"),
                                kCFAllocatorDefault, 0);
        CFNumberGetValue((CFNumberRef)pidref, kCFNumberIntType, &pid);
        CFRelease(pidref);
        id = vid << 16 | pid;

        /* get product vendor */
        char vendor_buf[256];
        CFIndex vendor_buflen = 256;
        CFTypeRef vendor_name_ref = NULL;

        vendor_name_ref = IORegistryEntrySearchCFProperty(usbCurrentObj,
                                 kIOServicePlane, CFSTR("USB Vendor Name"),
                                 kCFAllocatorDefault, 0);
        if(vendor_name_ref != NULL) {
            CFStringGetCString((CFStringRef)vendor_name_ref, vendor_buf, vendor_buflen,
                               kCFStringEncodingUTF8);
            name += QString::fromUtf8(vendor_buf) + " ";
            CFRelease(vendor_name_ref);
        }
        else {
            name += QObject::tr("(unknown vendor name) ");
        }

        /* get product name */
        char product_buf[256];
        CFIndex product_buflen = 256;
        CFTypeRef product_name_ref = NULL;

        product_name_ref = IORegistryEntrySearchCFProperty(usbCurrentObj,
                                kIOServicePlane, CFSTR("USB Product Name"),
                                kCFAllocatorDefault, 0);
        if(product_name_ref != NULL) {
            CFStringGetCString((CFStringRef)product_name_ref, product_buf, product_buflen,
                               kCFStringEncodingUTF8);
            name += QString::fromUtf8(product_buf);
            CFRelease(product_name_ref);
        }
        else {
            name += QObject::tr("(unknown product name)");
        }

        if(id) {
            usbids.insertMulti(id, name);
            LOG_INFO() << "USB:" << QString("0x%1").arg(id, 8, 16) << name;
        }

    }
    IOObjectRelease(usb_iterator);
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

        // get device descriptor first
        // for some reason not doing so results in bad things (tm)
        while(!SetupDiGetDeviceRegistryProperty(deviceInfo, &infoData,
            SPDRP_DEVICEDESC, &data, (PBYTE)buffer, buffersize, &buffersize)) {
            if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                if(buffer) free(buffer);
                // double buffer size to avoid problems as per KB888609
                buffer = (LPTSTR)malloc(buffersize * 2);
            }
            else {
                break;
            }
        }
        if(!buffer) {
            LOG_WARNING() << "Got no device description"
                          << "(SetupDiGetDeviceRegistryProperty), item" << i;
            continue;
        }
        description = QString::fromWCharArray(buffer);

        // now get the hardware id, which contains PID and VID.
        while(!SetupDiGetDeviceRegistryProperty(deviceInfo, &infoData,
            SPDRP_HARDWAREID, &data, (PBYTE)buffer, buffersize, &buffersize)) {
            if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                if(buffer) free(buffer);
                // double buffer size to avoid problems as per KB888609
                buffer = (LPTSTR)malloc(buffersize * 2);
            }
            else {
                break;
            }
        }

        if(buffer) {
            // convert buffer text to upper case to avoid depending on the case of
            // the keys (W7 uses different casing than XP at least), in addition
            // XP may use "Vid_" and "Pid_".
            QString data = QString::fromWCharArray(buffer).toUpper();
            QRegExp rex("USB\\\\VID_([0-9A-F]{4})&PID_([0-9A-F]{4}).*");
            if(rex.indexIn(data) >= 0) {
                uint32_t id;
                id = rex.cap(1).toUInt(0, 16) << 16 | rex.cap(2).toUInt(0, 16);
                usbids.insert(id, description);
                LOG_INFO() << "USB:" << QString("0x%1").arg(id, 8, 16);
            }
            free(buffer);
        }
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

    //LOG_INFO() << QString::fromWCharArray(proxyval) << QString("%1").arg(enable);
    if(enable != 0)
        return QUrl("http://" + QString::fromWCharArray(proxyval));
    else
        return QUrl("");
#elif defined(Q_OS_MACX)

    CFDictionaryRef dictref;
    CFStringRef stringref;
    CFNumberRef numberref;
    int enable = 0;
    int port = 0;
    unsigned int bufsize = 0;
    char *buf;
    QUrl proxy;

    dictref = SCDynamicStoreCopyProxies(NULL);
    if(dictref == NULL)
        return proxy;
    numberref = (CFNumberRef)CFDictionaryGetValue(dictref, kSCPropNetProxiesHTTPEnable);
    if(numberref != NULL)
        CFNumberGetValue(numberref, kCFNumberIntType, &enable);
    if(enable == 1) {
        // get proxy string
        stringref = (CFStringRef)CFDictionaryGetValue(dictref, kSCPropNetProxiesHTTPProxy);
        if(stringref != NULL) {
            // get number of characters. CFStringGetLength uses UTF-16 code pairs
            bufsize = CFStringGetLength(stringref) * 2 + 1;
            buf = (char*)malloc(sizeof(char) * bufsize);
            if(buf == NULL) {
                LOG_ERROR() << "can't allocate memory for proxy string!";
                CFRelease(dictref);
                return QUrl("");
            }
            CFStringGetCString(stringref, buf, bufsize, kCFStringEncodingUTF16);
            numberref = (CFNumberRef)CFDictionaryGetValue(dictref, kSCPropNetProxiesHTTPPort);
            if(numberref != NULL)
                CFNumberGetValue(numberref, kCFNumberIntType, &port);
            proxy.setScheme("http");
            proxy.setHost(QString::fromUtf16((unsigned short*)buf));
            proxy.setPort(port);

            free(buf);
            }
    }
    CFRelease(dictref);

    return proxy;
#else
    return QUrl("");
#endif
}


