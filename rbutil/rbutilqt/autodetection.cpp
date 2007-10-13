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

#include "autodetection.h"

#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
#include <stdio.h>
#include <usb.h>
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
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <setupapi.h>
#endif

Autodetection::Autodetection(QObject* parent): QObject(parent)
{

}

bool Autodetection::detect()
{
    m_device = "";
    m_mountpoint = "";
    m_errdev = "";

    detectUsb();

    // Try detection via rockbox.info / rbutil.log
    QStringList mountpoints = getMountpoints();

    for(int i=0; i< mountpoints.size();i++)
    {
        // do the file checking
        QDir dir(mountpoints.at(i));
        if(dir.exists())
        {
            qDebug() << "file checking:" << mountpoints.at(i);
            // check logfile first.
            if(QFile(mountpoints.at(i) + "/.rockbox/rbutil.log").exists()) {
                QSettings log(mountpoints.at(i) + "/.rockbox/rbutil.log",
                              QSettings::IniFormat, this);
                if(!log.value("platform").toString().isEmpty()) {
                    if(m_device.isEmpty())
                        m_device = log.value("platform").toString();
                    m_mountpoint = mountpoints.at(i);
                    qDebug() << "rbutil.log detected:" << m_device << m_mountpoint;
                    return true;
                }
            }

            // check rockbox-info.txt afterwards.
            QFile file(mountpoints.at(i) + "/.rockbox/rockbox-info.txt");
            if(file.exists())
            {
                file.open(QIODevice::ReadOnly | QIODevice::Text);
                QString line = file.readLine();
                if(line.startsWith("Target: "))
                {
                    line.remove("Target: ");
                    if(m_device.isEmpty())
                        m_device = line.trimmed(); // trim whitespaces
                    m_mountpoint = mountpoints.at(i);
                    qDebug() << "rockbox-info.txt detected:" << m_device << m_mountpoint;
                    return true;
                }
            }
            // check for some specific files in root folder
            QDir root(mountpoints.at(i));
            QStringList rootentries = root.entryList(QDir::Files);
            if(rootentries.contains("archos.mod", Qt::CaseInsensitive))
            {
                // archos.mod in root folder -> Archos Player
                m_device = "player";
                m_mountpoint = mountpoints.at(i);
                return true;
            }
            if(rootentries.contains("ONDIOST.BIN", Qt::CaseInsensitive))
            {
                // ONDIOST.BIN in root -> Ondio FM
                m_device = "ondiofm";
                m_mountpoint = mountpoints.at(i);
                return true;
            }
            if(rootentries.contains("ONDIOSP.BIN", Qt::CaseInsensitive))
            {
                // ONDIOSP.BIN in root -> Ondio SP
                m_device = "ondiosp";
                m_mountpoint = mountpoints.at(i);
                return true;
            }
            if(rootentries.contains("ajbrec.ajz", Qt::CaseInsensitive))
            {
                qDebug() << "ajbrec.ajz found. Trying detectAjbrec()";
                if(detectAjbrec(mountpoints.at(i))) {
                    m_mountpoint = mountpoints.at(i);
                    qDebug() << m_device;
                    return true;
                }
            }
            // detection based on player specific folders
            QStringList rootfolders = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            if(rootfolders.contains("GBSYSTEM"), Qt::CaseInsensitive)
            {
                // GBSYSTEM folder -> Gigabeat
                m_device = "gigabeatf";
                m_mountpoint = mountpoints.at(i);
                return true;
            }
        }

    }

    int n;
    //try ipodpatcher
    struct ipod_t ipod;
    n = ipod_scan(&ipod);
    if(n == 1) {
        qDebug() << "Ipod found:" << ipod.modelstr << "at" << ipod.diskname;
        m_device = ipod.targetname;
        m_mountpoint = resolveMountPoint(ipod.diskname);
        return true;
    }

    //try sansapatcher
    struct sansa_t sansa;
    n = sansa_scan(&sansa);
    if(n == 1) {
        qDebug() << "Sansa found:" << sansa.targetname << "at" << sansa.diskname;
        m_device = QString("sansa%1").arg(sansa.targetname);
        m_mountpoint = resolveMountPoint(sansa.diskname);
        return true;
    }

    if(m_mountpoint.isEmpty() && m_device.isEmpty() && m_errdev.isEmpty())
        return false;
    return true;
}


QStringList Autodetection::getMountpoints()
{
    QStringList tempList;
#if defined(Q_OS_WIN32)
    QFileInfoList list = QDir::drives();
    for(int i=0; i<list.size();i++)
    {
        tempList << list.at(i).absolutePath();
    }

#elif defined(Q_OS_MACX)
    int num;
    struct statfs *mntinf;

    num = getmntinfo(&mntinf, MNT_WAIT);
    while(num--) {
        tempList << QString(mntinf->f_mntonname);
        mntinf++;
    }
#elif defined(Q_OS_LINUX)

    FILE *mn = setmntent("/etc/mtab", "r");
    if(!mn)
        return QStringList("");

    struct mntent *ent;
    while((ent = getmntent(mn)))
        tempList << QString(ent->mnt_dir);
    endmntent(mn);

#else
#error Unknown Plattform
#endif
    return tempList;
}

QString Autodetection::resolveMountPoint(QString device)
{
    qDebug() << "Autodetection::resolveMountPoint(QString)" << device;

#if defined(Q_OS_LINUX)
    FILE *mn = setmntent("/etc/mtab", "r");
    if(!mn)
        return QString("");

    struct mntent *ent;
    while((ent = getmntent(mn))) {
        if(QString(ent->mnt_fsname).startsWith(device)
           && QString(ent->mnt_type).contains("vfat", Qt::CaseInsensitive)) {
            endmntent(mn);
            return QString(ent->mnt_dir);
        }
    }
    endmntent(mn);

#endif

#if defined(Q_OS_MACX)
    int num;
    struct statfs *mntinf;

    num = getmntinfo(&mntinf, MNT_WAIT);
    while(num--) {
        if(QString(mntinf->f_mntfromname).startsWith(device)
           && QString(mntinf->f_fstypename).contains("vfat", Qt::CaseInsensitive))
            return QString(mntinf->f_mntonname);
        mntinf++;
    }
#endif
    return QString("");

}


/** @brief detect devices based on usb pid / vid.
 *  @return true upon success, false otherwise.
 */
bool Autodetection::detectUsb()
{
    // autodetection uses the buildin device settings only
    QSettings dev(":/ini/rbutil.ini", QSettings::IniFormat, this);

    // get a list of ID -> target name
    QStringList platforms;
    dev.beginGroup("platforms");
    platforms = dev.childKeys();
    dev.endGroup();

    // usbids holds the mapping in the form
    // ((VID<<16)|(PID)), targetname
    // the ini file needs to hold the IDs as hex values.
    QMap<int, QString> usbids;
    QMap<int, QString> usberror;

    for(int i = 0; i < platforms.size(); i++) {
        dev.beginGroup("platforms");
        QString target = dev.value(platforms.at(i)).toString();
        dev.endGroup();
        dev.beginGroup(target);
        if(!dev.value("usbid").toString().isEmpty())
            usbids.insert(dev.value("usbid").toString().toInt(0, 16), target);
        if(!dev.value("usberror").toString().isEmpty())
            usberror.insert(dev.value("usberror").toString().toInt(0, 16), target);
        dev.endGroup();
    }

    // usb pid detection
#if defined(Q_OS_LINUX) | defined(Q_OS_MACX)
    usb_init();
    usb_find_busses();
    usb_find_devices();
    struct usb_bus *b;
    b = usb_get_busses();

    while(b) {
        qDebug() << "bus:" << b->dirname << b->devices;
        if(b->devices) {
            qDebug() << "devices present.";
            struct usb_device *u;
            u = b->devices;
            while(u) {
                uint32_t id;
                id = u->descriptor.idVendor << 16 | u->descriptor.idProduct;
                m_usbconid.append(id);
                qDebug("%x", id);

                if(usbids.contains(id)) {
                    m_device = usbids.value(id);
                    return true;
                }
                if(usberror.contains(id)) {
                    m_errdev = usberror.value(id);
                    // we detected something, so return true
                    qDebug() << "detected device with problems via usb!";
                    return true;
                }
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
            m_usbconid.append(id);
            qDebug("VID: %04x PID: %04x", vid, pid);
            if(usbids.contains(id)) {
                    m_device = usbids.value(id);
                    if(buffer) free(buffer);
                    SetupDiDestroyDeviceInfoList(deviceInfo);
                    qDebug() << "detectUsb: Got" << m_device;
                    return true;
                }
                if(usberror.contains(id)) {
                    m_errdev = usberror.value(id);
                    // we detected something, so return true
                    if(buffer) free(buffer);
                    SetupDiDestroyDeviceInfoList(deviceInfo);
                    qDebug() << "detectUsb: Got" << m_device;
                    qDebug() << "detected device with problems via usb!";
                    return true;
                }
        }
        if(buffer) free(buffer);
    }
    SetupDiDestroyDeviceInfoList(deviceInfo);

#endif
    return false;
}


bool Autodetection::detectAjbrec(QString root)
{
    QFile f(root + "/ajbrec.ajz");
    char header[24];
    f.open(QIODevice::ReadOnly);
    if(!f.read(header, 24)) return false;

    // check the header of the file.
    // recorder v1 had a 6 bytes sized header
    // recorder v2, FM, Ondio SP and FM have a 24 bytes header.

    // recorder v1 has the binary length in the first 4 bytes, so check
    // for them first.
    int len = (header[0]<<24) | (header[1]<<16) | (header[2]<<8) | header[3];
    qDebug() << "possible bin length:" << len;
    qDebug() << "file len:" << f.size();
    if((f.size() - 6) == len)
        m_device = "recorder";
    
    // size didn't match, now we need to assume we have a headerlength of 24.
    switch(header[11]) {
        case 2:
            m_device = "recorderv2";
            break;
            
        case 4:
            m_device = "fmrecorder";
            break;
            
        case 8:
            m_device = "ondiofm";
            break;

        case 16:
            m_device = "ondiosp";
            break;

        default:
            break;
    }
    f.close();
    
    if(m_device.isEmpty()) return false;
    return true;
}
