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
#include "rbsettings.h"
#include "systeminfo.h"

#include "../ipodpatcher/ipodpatcher.h"
#include "../sansapatcher/sansapatcher.h"

#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
#include <stdio.h>
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

#include "system.h"
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
        qDebug() << "[Autodetect] paths to check:" << mounts;
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
                    qDebug() << "[Autodetect] rbutil.log detected:" << m_device << m_mountpoint;
                    return true;
                }
            }

            // check rockbox-info.txt afterwards.
            RockboxInfo info(mounts.at(i));
            if(info.success())
            {
                if(m_device.isEmpty())
                {
                    m_device = info.target();
                    // special case for video64mb. This is a workaround, and
                    // should get replaced when autodetection is reworked.
                    if(m_device == "ipodvideo" && info.ram() == 64)
                    {
                        m_device = "ipodvideo64mb";
                    }
                }
                m_mountpoint = mounts.at(i);
                qDebug() << "[Autodetect] rockbox-info.txt detected:"
                         << m_device << m_mountpoint;
                return true;
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
                qDebug() << "[Autodetect] ajbrec.ajz found. Trying detectAjbrec()";
                if(detectAjbrec(mounts.at(i))) {
                    m_mountpoint = mounts.at(i);
                    qDebug() << "[Autodetect]" << m_device;
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
        qDebug() << "[Autodetect] Ipod found:" << ipod.modelstr << "at" << ipod.diskname;
        // if the found ipod is a macpod also notice it as device with problem.
        if(ipod.macpod)
            m_errdev = ipod.targetname;
        m_device = ipod.targetname;
        m_mountpoint = resolveMountPoint(ipod.diskname);
        return true;
    }
    else {
        qDebug() << "[Autodetect] ipodpatcher: no Ipod found." << n;
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
        qDebug() << "[Autodetect] Sansa found:" << sansa.targetname << "at" << sansa.diskname;
        m_device = QString("sansa%1").arg(sansa.targetname);
        m_mountpoint = resolveMountPoint(sansa.diskname);
        return true;
    }
    else {
        qDebug() << "[Autodetect] sansapatcher: no Sansa found." << n;
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


/** resolve device name to mount point / drive letter
 *  @param device device name / disk number
 *  @return mount point / drive letter
 */
QString Autodetection::resolveMountPoint(QString device)
{
    qDebug() << "[Autodetect] resolving mountpoint:" << device;

#if defined(Q_OS_LINUX)
    FILE *mn = setmntent("/etc/mtab", "r");
    if(!mn)
        return QString("");

    struct mntent *ent;
    while((ent = getmntent(mn))) {
        // Check for valid filesystem. Allow hfs too, as an Ipod might be a
        // MacPod.
        if(QString(ent->mnt_fsname) == device
           && (QString(ent->mnt_type).contains("vfat", Qt::CaseInsensitive)
               || QString(ent->mnt_type).contains("hfs", Qt::CaseInsensitive))) {
            endmntent(mn);
            qDebug() << "[Autodetect] resolved mountpoint is:" << ent->mnt_dir;
            return QString(ent->mnt_dir);
        }
    }
    endmntent(mn);

#endif

#if defined(Q_OS_MACX) || defined(Q_OS_OPENBSD)
    int num;
    struct statfs *mntinf;

    num = getmntinfo(&mntinf, MNT_WAIT);
    while(num--) {
        // Check for valid filesystem. Allow hfs too, as an Ipod might be a
        // MacPod.
        if(QString(mntinf->f_mntfromname) == device
           && (QString(mntinf->f_fstypename).contains("msdos", Qt::CaseInsensitive)
                || QString(mntinf->f_fstypename).contains("hfs", Qt::CaseInsensitive))) {
            qDebug() << "[Autodetect] resolved mountpoint is:" << mntinf->f_mntonname;
            return QString(mntinf->f_mntonname);
        }
        mntinf++;
    }
#endif

#if defined(Q_OS_WIN32)
    QString result;
    unsigned int driveno = device.replace(QRegExp("^.*([0-9]+)"), "\\1").toInt();

    int letter;
    for(letter = 'A'; letter <= 'Z'; letter++) {
        if(resolveDevicename(QString(letter)).toUInt() == driveno) {
            result = letter;
            break;
        }
    }
    qDebug() << "[Autodetect] resolved mountpoint is:" << result;
    if(!result.isEmpty())
        return result + ":/";
#endif
    return QString("");
}


/** Resolve mountpoint to devicename / disk number
 *  @param path mountpoint path / drive letter
 *  @return devicename / disk number
 */
QString Autodetection::resolveDevicename(QString path)
{
    qDebug() << "[Autodetect] resolving device name" << path;
#if defined(Q_OS_LINUX)
    FILE *mn = setmntent("/etc/mtab", "r");
    if(!mn)
        return QString("");

    struct mntent *ent;
    while((ent = getmntent(mn))) {
        // check for valid filesystem type.
        // Linux can handle hfs (and hfsplus), so consider it a valid file
        // system. Otherwise resolving the device name would fail, which in
        // turn would make it impossible to warn about a MacPod.
        if(QString(ent->mnt_dir) == path
           && (QString(ent->mnt_type).contains("vfat", Qt::CaseInsensitive)
            || QString(ent->mnt_type).contains("hfs", Qt::CaseInsensitive))) {
            endmntent(mn);
            qDebug() << "[Autodetect] device name is" << ent->mnt_fsname;
            return QString(ent->mnt_fsname);
        }
    }
    endmntent(mn);

#endif

#if defined(Q_OS_MACX) || defined(Q_OS_OPENBSD)
    int num;
    struct statfs *mntinf;

    num = getmntinfo(&mntinf, MNT_WAIT);
    while(num--) {
        // check for valid filesystem type. OS X can handle hfs (hfs+ is
        // treated as hfs), BSD should be the same.
        if(QString(mntinf->f_mntonname) == path
           && (QString(mntinf->f_fstypename).contains("msdos", Qt::CaseInsensitive)
            || QString(mntinf->f_fstypename).contains("hfs", Qt::CaseInsensitive))) {
            qDebug() << "[Autodetect] device name is" << mntinf->f_mntfromname;
            return QString(mntinf->f_mntfromname);
        }
        mntinf++;
    }
#endif

#if defined(Q_OS_WIN32)
    DWORD written;
    HANDLE h;
    TCHAR uncpath[MAX_PATH];
    UCHAR buffer[0x400];
    PVOLUME_DISK_EXTENTS extents = (PVOLUME_DISK_EXTENTS)buffer;

    _stprintf(uncpath, _TEXT("\\\\.\\%c:"), path.toAscii().at(0));
    h = CreateFile(uncpath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, NULL);
    if(h == INVALID_HANDLE_VALUE) {
        //qDebug() << "error getting extents for" << uncpath;
        return "";
    }
    // get the extents
    if(DeviceIoControl(h, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                NULL, 0, extents, sizeof(buffer), &written, NULL)) {
        if(extents->NumberOfDiskExtents > 1) {
            qDebug() << "[Autodetect] resolving device name: volume spans multiple disks!";
            return "";
        }
        qDebug() << "[Autodetect] device name is" << extents->Extents[0].DiskNumber;
        return QString("%1").arg(extents->Extents[0].DiskNumber);
    }
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
    QMap<int, QString> usbids = SystemInfo::usbIdMap(SystemInfo::MapDevice);
    QMap<int, QString> usberror = SystemInfo::usbIdMap(SystemInfo::MapError);
    QMap<int, QString> usbincompat = SystemInfo::usbIdMap(SystemInfo::MapIncompatible);

    // usb pid detection
    QList<uint32_t> attached;
    attached = System::listUsbIds();

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
        QString idstring = QString("%1").arg(attached.at(i), 8, 16, QChar('0'));
        if(!SystemInfo::platformValue(idstring, SystemInfo::CurName).toString().isEmpty()) {
            m_incompat = idstring;
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
    qDebug() << "[Autodetect] ABJREC possible bin length:" << len
             << "file len:" << f.size();
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

