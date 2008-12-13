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
#include "autodetection.h"

#include "../ipodpatcher/ipodpatcher.h"
#include "../sansapatcher/sansapatcher.h"

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
#include <winioctl.h>
#endif

#if defined(Q_OS_OPENBSD)
#include <sys/param.h>
#include <sys/mount.h>
#endif

#include "detect.h"
#include "utils.h"

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
    QStringList mounts = mountpoints();

    for(int i=0; i< mounts.size();i++)
    {
        // do the file checking
        QDir dir(mounts.at(i));
        qDebug() << "paths to check for player specific files:" << mounts;
        if(dir.exists())
        {
            // check logfile first.
            if(QFile(mounts.at(i) + "/.rockbox/rbutil.log").exists()) {
                QSettings log(mounts.at(i) + "/.rockbox/rbutil.log",
                              QSettings::IniFormat, this);
                if(!log.value("platform").toString().isEmpty()) {
                    if(m_device.isEmpty())
                        m_device = log.value("platform").toString();
                    m_mountpoint = mounts.at(i);
                    qDebug() << "rbutil.log detected:" << m_device << m_mountpoint;
                    return true;
                }
            }

            // check rockbox-info.txt afterwards.
            QFile file(mounts.at(i) + "/.rockbox/rockbox-info.txt");
            if(file.exists())
            {
                file.open(QIODevice::ReadOnly | QIODevice::Text);
                QString line = file.readLine();
                if(line.startsWith("Target: "))
                {
                    line.remove("Target: ");
                    if(m_device.isEmpty())
                        m_device = line.trimmed(); // trim whitespaces
                    m_mountpoint = mounts.at(i);
                    qDebug() << "rockbox-info.txt detected:" << m_device << m_mountpoint;
                    return true;
                }
            }
            // check for some specific files in root folder
            QDir root(mounts.at(i));
            QStringList rootentries = root.entryList(QDir::Files);
            if(rootentries.contains("archos.mod", Qt::CaseInsensitive))
            {
                // archos.mod in root folder -> Archos Player
                m_device = "player";
                m_mountpoint = mounts.at(i);
                return true;
            }
            if(rootentries.contains("ONDIOST.BIN", Qt::CaseInsensitive))
            {
                // ONDIOST.BIN in root -> Ondio FM
                m_device = "ondiofm";
                m_mountpoint = mounts.at(i);
                return true;
            }
            if(rootentries.contains("ONDIOSP.BIN", Qt::CaseInsensitive))
            {
                // ONDIOSP.BIN in root -> Ondio SP
                m_device = "ondiosp";
                m_mountpoint = mounts.at(i);
                return true;
            }
            if(rootentries.contains("ajbrec.ajz", Qt::CaseInsensitive))
            {
                qDebug() << "ajbrec.ajz found. Trying detectAjbrec()";
                if(detectAjbrec(mounts.at(i))) {
                    m_mountpoint = mounts.at(i);
                    qDebug() << m_device;
                    return true;
                }
            }
            // detection based on player specific folders
            QStringList rootfolders = root.entryList(QDir::Dirs
                    | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
            if(rootfolders.contains("GBSYSTEM", Qt::CaseInsensitive))
            {
                // GBSYSTEM folder -> Gigabeat
                m_device = "gigabeatf";
                m_mountpoint = mounts.at(i);
                return true;
            }
#if defined(Q_OS_WIN32)
            // on windows, try to detect the drive letter of an Ipod
            if(rootfolders.contains("iPod_Control", Qt::CaseInsensitive))
            {
                // iPod_Control folder -> Ipod found
                // detecting of the Ipod type is done below using ipodpatcher
                m_mountpoint = mounts.at(i);
            }
#endif
        }

    }

    int n;
    // try ipodpatcher
    // initialize sector buffer. Needed.
    ipod_sectorbuf = NULL;
    ipod_alloc_buffer(&ipod_sectorbuf, BUFFER_SIZE);
    struct ipod_t ipod;
    n = ipod_scan(&ipod);
    if(n == 1) {
        qDebug() << "Ipod found:" << ipod.modelstr << "at" << ipod.diskname;
        m_device = ipod.targetname;
        m_mountpoint = resolveMountPoint(ipod.diskname);
        return true;
    }
    else {
        qDebug() << "ipodpatcher: no Ipod found." << n;
    }
    free(ipod_sectorbuf);
    ipod_sectorbuf = NULL;

    // try sansapatcher
    // initialize sector buffer. Needed.
    sansa_sectorbuf = NULL;
    sansa_alloc_buffer(&sansa_sectorbuf, BUFFER_SIZE);
    struct sansa_t sansa;
    n = sansa_scan(&sansa);
    if(n == 1) {
        qDebug() << "Sansa found:" << sansa.targetname << "at" << sansa.diskname;
        m_device = QString("sansa%1").arg(sansa.targetname);
        m_mountpoint = resolveMountPoint(sansa.diskname);
        return true;
    }
    else {
        qDebug() << "sansapatcher: no Sansa found." << n;
    }
    free(sansa_sectorbuf);
    sansa_sectorbuf = NULL;

    if(m_mountpoint.isEmpty() && m_device.isEmpty()
            && m_errdev.isEmpty() && m_incompat.isEmpty())
        return false;
    return true;
}


QStringList Autodetection::mountpoints()
{
    QStringList tempList;
#if defined(Q_OS_WIN32)
    QFileInfoList list = QDir::drives();
    for(int i=0; i<list.size();i++)
    {
        tempList << list.at(i).absolutePath();
    }

#elif defined(Q_OS_MACX) || defined(Q_OS_OPENBSD)
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

#if defined(Q_OS_OPENBSD)
    int num;
    struct statfs *mntinf;

    num = getmntinfo(&mntinf, MNT_WAIT);
    while(num--) {
        if(QString(mntinf->f_mntfromname).startsWith(device)
           && QString(mntinf->f_fstypename).contains("msdos", Qt::CaseInsensitive))
            return QString(mntinf->f_mntonname);
        mntinf++;
    }
#endif

#if defined(Q_OS_WIN32)
    QString result;
    unsigned int driveno = device.replace(QRegExp("^.*([0-9]+)"), "\\1").toInt();

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
            //qDebug() << "error getting extents for" << uncpath;
            continue;
        }
        // get the extents
        if(DeviceIoControl(h, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                    NULL, 0, extents, sizeof(buffer), &written, NULL)) {
            for(unsigned int a = 0; a < extents->NumberOfDiskExtents; a++) {
                qDebug() << "Disk:" << extents->Extents[a].DiskNumber;
                if(extents->Extents[a].DiskNumber == driveno) {
                    result = letter;
                    qDebug("drive found for volume %i: %c", driveno, letter);
                    break;
                }
            }

        }

    }
    if(!result.isEmpty())
        return result + ":/";
#endif
    return QString("");
}


/** @brief detect devices based on usb pid / vid.
 *  @return true upon success, false otherwise.
 */
bool Autodetection::detectUsb()
{
    // usbids holds the mapping in the form
    // ((VID<<16)|(PID)), targetname
    // the ini file needs to hold the IDs as hex values.
    QMap<int, QString> usbids = settings->usbIdMap();
    QMap<int, QString> usberror = settings->usbIdErrorMap();
    QMap<int, QString> usbincompat = settings->usbIdIncompatMap();

    // usb pid detection
    QList<uint32_t> attached;
    attached = Detect::listUsbIds();

    int i = attached.size();
    while(i--) {
        if(usbids.contains(attached.at(i))) {
            m_device = usbids.value(attached.at(i));
            qDebug() << "[USB] detected supported player" << m_device;
            return true;
        }
        if(usberror.contains(attached.at(i))) {
            m_errdev = usberror.value(attached.at(i));
            qDebug() << "[USB] detected problem with player" << m_errdev;
            return true;
        }
        if(usbincompat.contains(attached.at(i))) {
            m_incompat = usbincompat.value(attached.at(i));
            qDebug() << "[USB] detected incompatible player" << m_incompat;
            return true;
        }
    }
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

