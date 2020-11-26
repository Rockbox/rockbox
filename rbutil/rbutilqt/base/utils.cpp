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

#include "utils.h"
#include "rockboxinfo.h"
#include "system.h"
#include "rbsettings.h"
#include "systeminfo.h"
#include "Logger.h"

#if !defined(_UNICODE)
#define _UNICODE
#endif

#include <QtCore>
#include <QDebug>
#include <cstdlib>
#include <stdio.h>

#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
#include <sys/statvfs.h>
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
#include <stdio.h>
#endif
#if defined(Q_OS_LINUX)
#include <mntent.h>
#endif
#if defined(Q_OS_MACX) || defined(Q_OS_OPENBSD)
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#endif
#if defined(Q_OS_WIN32)
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <setupapi.h>
#include <winioctl.h>
#include <tlhelp32.h>
#endif
#if defined(Q_OS_MACX)
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <IOKit/IOKitLib.h>
#endif

// recursive function to delete a dir with files
bool Utils::recursiveRmdir( const QString &dirName )
{
    QString dirN = dirName;
    QDir dir(dirN);
    // make list of entries in directory
    QStringList list = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
    QFileInfo fileInfo;
    QString curItem;
    for(int i = 0; i < list.size(); i++){ // loop through all items of list
        QString name = list.at(i);
        curItem = dirN + "/" + name;
        fileInfo.setFile(curItem);
        if(fileInfo.isDir())    // is directory
            recursiveRmdir(curItem);    // call recRmdir() recursively for
                                        // deleting subdirectory
        else                    // is file
            QFile::remove(curItem);     // ok, delete file
    }
    dir.cdUp();
    return dir.rmdir(dirN); // delete empty dir and return if (now empty)
                            // dir-removing was successfull
}


//! @brief resolves the given path, ignoring case.
//! @param path absolute path to resolve.
//! @return returns exact casing of path, empty string if path not found.
QString Utils::resolvePathCase(QString path)
{
    int start;
    QString realpath;
#if QT_VERSION >= 0x050e00
    QStringList elems = path.split("/", Qt::SkipEmptyParts);
#else
    QStringList elems = path.split("/", QString::SkipEmptyParts);
#endif

    if(path.isEmpty())
        return QString();
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
    LOG_INFO() << "resolving path" << path << "->" << realpath;
    return realpath;
}


QString Utils::filesystemType(QString path)
{
#if defined(Q_OS_LINUX)
    FILE *mn = setmntent("/etc/mtab", "r");
    if(!mn)
        return QString("");

    struct mntent *ent;
    while((ent = getmntent(mn))) {
        if(QString(ent->mnt_dir) == path) {
            endmntent(mn);
            LOG_INFO() << "device type is" << ent->mnt_type;
            return QString(ent->mnt_type);
        }
    }
    endmntent(mn);
#endif

#if defined(Q_OS_MACX) || defined(Q_OS_OPENBSD)
    int num;
    struct statfs *mntinf;

    num = getmntinfo(&mntinf, MNT_WAIT);
    while(num--) {
        if(QString(mntinf->f_mntonname) == path) {
            LOG_INFO() << "device type is" << mntinf->f_fstypename;
            return QString(mntinf->f_fstypename);
        }
        mntinf++;
    }
#endif

#if defined(Q_OS_WIN32)
    wchar_t t[64];
    memset(t, 0, 32);
    if(GetVolumeInformationW((LPCWSTR)path.utf16(),
                             NULL, 0, NULL, NULL, NULL, t, 64)) {
        LOG_INFO() << "device type is" << t;
        return QString::fromWCharArray(t);
    }
#endif
    return QString("-");
}


QString Utils::filesystemName(QString path)
{
    QString name;
#if defined(Q_OS_WIN32)
    wchar_t volname[MAX_PATH+1];
    bool res = GetVolumeInformationW((LPTSTR)path.utf16(), volname, MAX_PATH+1,
            NULL, NULL, NULL, NULL, 0);
    if(res) {
        name = QString::fromWCharArray(volname);
    }
#endif
#if defined(Q_OS_MACX)
    // BSD label does not include folder.
    QString bsd = Utils::resolveDevicename(path).remove("/dev/");
    if(bsd.isEmpty()) {
        return name;
    }
    OSStatus result;
    ItemCount index = 1;

    do {
        FSVolumeRefNum volrefnum;
        HFSUniStr255 volname;

        result = FSGetVolumeInfo(kFSInvalidVolumeRefNum, index, &volrefnum,
                kFSVolInfoFSInfo, NULL, &volname, NULL);

        if(result == noErr) {
            GetVolParmsInfoBuffer volparms;
            /* PBHGetVolParmsSync() is not available for 64bit while
            FSGetVolumeParms() is available in 10.5+. Thus we need to use
            PBHGetVolParmsSync() for 10.4, and that also requires 10.4 to
            always use 32bit.
            Qt 4 supports 32bit on 10.6 Cocoa only.
            */
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 1050
            if(FSGetVolumeParms(volrefnum, &volparms, sizeof(volparms)) == noErr)
#else
            HParamBlockRec hpb;
            hpb.ioParam.ioNamePtr = NULL;
            hpb.ioParam.ioVRefNum = volrefnum;
            hpb.ioParam.ioBuffer = (Ptr)&volparms;
            hpb.ioParam.ioReqCount = sizeof(volparms);
            if(PBHGetVolParmsSync(&hpb) == noErr)
#endif
            {
                if(volparms.vMServerAdr == 0) {
                    if(bsd == (char*)volparms.vMDeviceID) {
                        name = QString::fromUtf16((const ushort*)volname.unicode,
                                                  (int)volname.length);
                        break;
                    }
                }
            }
        }
        index++;
    } while(result == noErr);
#endif

    LOG_INFO() << "Volume name of" << path << "is" << name;
    return name;
}


//! @brief figure the free disk space on a filesystem
//! @param path path on the filesystem to check
//! @return size in bytes
qulonglong Utils::filesystemFree(QString path)
{
    qulonglong size = filesystemSize(path, FilesystemFree);
    LOG_INFO() << "free disk space for" << path << size;
    return size;
}


qulonglong Utils::filesystemTotal(QString path)
{
    qulonglong size = filesystemSize(path, FilesystemTotal);
    LOG_INFO() << "total disk space for" << path << size;
    return size;
}


qulonglong Utils::filesystemSize(QString path, enum Utils::Size type)
{
    qulonglong size = 0;
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
    // the usage of statfs() is deprecated by the LSB so use statvfs().
    struct statvfs fs;
    int ret;

    ret = statvfs(qPrintable(path), &fs);

    if(ret == 0) {
        if(type == FilesystemFree) {
            size = (qulonglong)fs.f_frsize * (qulonglong)fs.f_bavail;
        }
        if(type == FilesystemTotal) {
            size = (qulonglong)fs.f_frsize * (qulonglong)fs.f_blocks;
        }
        if(type == FilesystemClusterSize) {
            size = (qulonglong)fs.f_frsize;
        }
    }
#endif
#if defined(Q_OS_WIN32)
    BOOL ret;
    ULARGE_INTEGER freeAvailBytes;
    ULARGE_INTEGER totalNumberBytes;

    ret = GetDiskFreeSpaceExW((LPCTSTR)path.utf16(), &freeAvailBytes,
            &totalNumberBytes, NULL);
    if(ret) {
        if(type == FilesystemFree) {
            size = freeAvailBytes.QuadPart;
        }
        if(type == FilesystemTotal) {
            size = totalNumberBytes.QuadPart;
        }
        if(type == FilesystemClusterSize) {
            DWORD sectorsPerCluster;
            DWORD bytesPerSector;
            DWORD freeClusters;
            DWORD totalClusters;
            ret = GetDiskFreeSpaceW((LPCTSTR)path.utf16(), &sectorsPerCluster,
                    &bytesPerSector, &freeClusters, &totalClusters);
            if(ret) {
                size = bytesPerSector * sectorsPerCluster;
            }
        }
    }
#endif
    return size;
}

//! \brief searches for a Executable in the Environement Path
QString Utils::findExecutable(QString name)
{
    //try autodetect tts
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX) || defined(Q_OS_OPENBSD)
#if QT_VERSION >= 0x050e00
    QStringList path = QString(getenv("PATH")).split(":", Qt::SkipEmptyParts);
#else
    QStringList path = QString(getenv("PATH")).split(":", QString::SkipEmptyParts);
#endif
#elif defined(Q_OS_WIN)
#if QT_VERSION >= 0x050e00
    QStringList path = QString(getenv("PATH")).split(";", Qt::SkipEmptyParts);
#else
    QStringList path = QString(getenv("PATH")).split(";", QString::SkipEmptyParts);
#endif
#endif
    LOG_INFO() << "system path:" << path;
    for(int i = 0; i < path.size(); i++)
    {
        QString executable = QDir::fromNativeSeparators(path.at(i)) + "/" + name;
#if defined(Q_OS_WIN)
        executable += ".exe";
#if QT_VERSION >= 0x050e00
        QStringList ex = executable.split("\"", Qt::SkipEmptyParts);
#else
        QStringList ex = executable.split("\"", QString::SkipEmptyParts);
#endif
        executable = ex.join("");
#endif
        if(QFileInfo(executable).isExecutable())
        {
            LOG_INFO() << "findExecutable: found" << executable;
            return QDir::toNativeSeparators(executable);
        }
    }
    LOG_INFO() << "findExecutable: could not find" << name;
    return "";
}


/** @brief checks different Enviroment things. Ask if user wants to continue.
 *  @param permission if it should check for permission
 *  @return string with error messages if problems occurred, empty strings if none.
 */
QString Utils::checkEnvironment(bool permission)
{
    LOG_INFO() << "checking environment";
    QString text = "";

    // check permission
    if(permission)
    {
#if defined(Q_OS_WIN32)
        if(System::userPermissions() != System::ADMIN)
        {
            text += tr("<li>Permissions insufficient for bootloader "
                    "installation.\nAdministrator priviledges are necessary.</li>");
        }
#endif
    }

    // Check TargetId
    RockboxInfo rbinfo(RbSettings::value(RbSettings::Mountpoint).toString());
    QString installed = rbinfo.target();
    if(!installed.isEmpty() && installed !=
       SystemInfo::platformValue(SystemInfo::ConfigureModel).toString())
    {
        text += tr("<li>Target mismatch detected.<br/>"
                "Installed target: %1<br/>Selected target: %2.</li>")
            .arg(SystemInfo::platformValue(SystemInfo::Name, installed).toString(),
                 SystemInfo::platformValue(SystemInfo::Name).toString());
    }

    if(!text.isEmpty())
        return tr("Problem detected:") + "<ul>" + text + "</ul>";
    else
        return text;
}

/** @brief Trim version string from filename to version part only.
 *  @param s Version string
 *  @return Version part of string if found, input string on error.
 */
QString Utils::trimVersionString(QString s)
{
    QRegExp r = QRegExp(".*([\\d\\.]+\\d+[a-z]?).*");
    if(r.indexIn(s) != -1) {
        return r.cap(1);
    }
    return s;
}

/** @brief Compare two version strings.
 *  @param s1 first version string
 *  @param s2 second version string
 *  @return 0 if strings identical, 1 if second is newer, -1 if first.
 */
int Utils::compareVersionStrings(QString s1, QString s2)
{
    LOG_INFO() << "comparing version strings" << s1 << "and" << s2;
    QString a = s1.trimmed();
    QString b = s2.trimmed();
    // if strings are identical return 0.
    if(a.isEmpty())
        return 1;
    if(b.isEmpty())
        return -1;

    while(!a.isEmpty() || !b.isEmpty()) {
        // trim all leading non-digits and non-dots (dots are removed afterwards)
        a.remove(QRegExp("^[^\\d\\.]*"));
        b.remove(QRegExp("^[^\\d\\.]*"));

        // trim all trailing non-digits for conversion (QString::toInt()
        // requires this). Copy strings first as replace() changes the string.
        QString numa = a;
        QString numb = b;
        numa.remove(QRegExp("\\D+.*$"));
        numb.remove(QRegExp("\\D+.*$"));

        // convert to number
        bool ok1, ok2;
        unsigned int vala = numa.toUInt(&ok1);
        unsigned int valb = numb.toUInt(&ok2);
        // if none of the numbers converted successfully we're at trailing garbage.
        if(!ok1 && !ok2)
            break;
        if(!ok1)
            return 1;
        if(!ok2)
            return -1;

        // if numbers mismatch we have a decision.
        if(vala != valb)
            return (vala > valb) ? -1 : 1;

        // trim leading digits.
        a.remove(QRegExp("^\\d*"));
        b.remove(QRegExp("^\\d*"));

        // If only one of the following characters is a dot that one is
        // "greater" then anything else. Make sure it's followed by a number,
        // Otherwise it might be the end of the string or suffix.  Do this
        // before version addon characters check to avoid stopping too early.
        bool adot = a.contains(QRegExp("^[a-zA-Z]*\\.[0-9]"));
        bool bdot = b.contains(QRegExp("^[a-zA-Z]*\\.[0-9]"));
        if(adot && !bdot)
            return -1;
        if(!adot && bdot)
            return 1;
        // if number is immediately followed by a character consider it as
        // version addon (like 1.2.3b). In this case compare characters and end
        // (version numbers like 1.2b.3 aren't handled).
        QChar ltra;
        QChar ltrb;
        if(a.contains(QRegExp("^[a-zA-Z]")))
            ltra = a.at(0);
        if(b.contains(QRegExp("^[a-zA-Z]")))
            ltrb = b.at(0);
        if(ltra != ltrb)
            return (ltra < ltrb) ? 1 : -1;

        // both are identical or no addon characters, ignore.
        // remove modifiers and following dot.
        a.remove(QRegExp("^[a-zA-Z]*\\."));
        b.remove(QRegExp("^[a-zA-Z]*\\."));
    }

    // no differences found.
    return 0;
}


/** Resolve mountpoint to devicename / disk number
 *  @param path mountpoint path / drive letter
 *  @return devicename / disk number
 */
QString Utils::resolveDevicename(QString path)
{
    LOG_INFO() << "resolving device name" << path;
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
            LOG_INFO() << "device name is" << ent->mnt_fsname;
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
            LOG_INFO() << "device name is" << mntinf->f_mntfromname;
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

    _stprintf(uncpath, _TEXT("\\\\.\\%c:"), path.toLatin1().at(0));
    h = CreateFile(uncpath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, NULL);
    if(h == INVALID_HANDLE_VALUE) {
        //LOG_INFO() << "error getting extents for" << uncpath;
        return "";
    }
    // get the extents
    if(DeviceIoControl(h, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                NULL, 0, extents, sizeof(buffer), &written, NULL)) {
        if(extents->NumberOfDiskExtents == 1) {
            CloseHandle(h);
            LOG_INFO() << "device name is" << extents->Extents[0].DiskNumber;
            return QString("%1").arg(extents->Extents[0].DiskNumber);
        }
        LOG_INFO() << "resolving device name: volume spans multiple disks!";
    }
    CloseHandle(h);
#endif
    return QString("");

}


/** resolve device name to mount point / drive letter
 *  @param device device name / disk number
 *  @return mount point / drive letter
 */
QString Utils::resolveMountPoint(QString device)
{
    LOG_INFO() << "resolving mountpoint:" << device;

#if defined(Q_OS_LINUX)
    FILE *mn = setmntent("/etc/mtab", "r");
    if(!mn)
        return QString("");

    struct mntent *ent;
    while((ent = getmntent(mn))) {
        // Check for valid filesystem. Allow hfs too, as an Ipod might be a
        // MacPod.
        if(QString(ent->mnt_fsname) == device) {
            QString result;
            if(QString(ent->mnt_type).contains("vfat", Qt::CaseInsensitive)
                    || QString(ent->mnt_type).contains("hfs", Qt::CaseInsensitive)) {
                LOG_INFO() << "resolved mountpoint is:" << ent->mnt_dir;
                result = QString(ent->mnt_dir);
            }
            else {
                LOG_INFO() << "mountpoint is wrong filesystem!";
            }
            endmntent(mn);
            return result;
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
        if(QString(mntinf->f_mntfromname) == device) {
            if(QString(mntinf->f_fstypename).contains("msdos", Qt::CaseInsensitive)
                || QString(mntinf->f_fstypename).contains("hfs", Qt::CaseInsensitive)) {
                LOG_INFO() << "resolved mountpoint is:" << mntinf->f_mntonname;
                return QString(mntinf->f_mntonname);
            }
            else {
                LOG_INFO() << "mountpoint is wrong filesystem!";
                return QString();
            }
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
            LOG_INFO() << "resolved mountpoint is:" << result;
            break;
        }
    }
    if(!result.isEmpty())
        return result + ":/";
#endif
    LOG_INFO() << "resolving mountpoint failed!";
    return QString("");
}


QStringList Utils::mountpoints(enum MountpointsFilter type)
{
    QStringList supported;
    QStringList tempList;
#if defined(Q_OS_WIN32)
    supported << "FAT32" << "FAT16" << "FAT12" << "FAT" << "HFS";
    QFileInfoList list = QDir::drives();
    for(int i=0; i<list.size();i++)
    {
        wchar_t t[32];
        memset(t, 0, 32);
        if(GetVolumeInformationW((LPCWSTR)list.at(i).absolutePath().utf16(),
                NULL, 0, NULL, NULL, NULL, t, 32) == 0) {
            // on error empty retrieved type -- don't rely on
            // GetVolumeInformation not changing it.
            memset(t, 0, sizeof(t));
        }

        QString fstype = QString::fromWCharArray(t);
        if(type == MountpointsAll || supported.contains(fstype)) {
            tempList << list.at(i).absolutePath();
            LOG_INFO() << "Added:" << list.at(i).absolutePath()
                     << "type" << fstype;
        }
        else {
            LOG_INFO() << "Ignored:" << list.at(i).absolutePath()
                     << "type" << fstype;
        }
    }

#elif defined(Q_OS_MACX) || defined(Q_OS_OPENBSD)
    supported << "vfat" << "msdos" << "hfs";
    int num;
    struct statfs *mntinf;

    num = getmntinfo(&mntinf, MNT_WAIT);
    while(num--) {
        if(type == MountpointsAll || supported.contains(mntinf->f_fstypename)) {
            tempList << QString(mntinf->f_mntonname);
            LOG_INFO() << "Added:" << mntinf->f_mntonname
                     << "is" << mntinf->f_mntfromname << "type" << mntinf->f_fstypename;
        }
        else {
            LOG_INFO() << "Ignored:" << mntinf->f_mntonname
                     << "is" << mntinf->f_mntfromname << "type" << mntinf->f_fstypename;
        }
        mntinf++;
    }
#elif defined(Q_OS_LINUX)
    supported << "vfat" << "msdos" << "hfsplus";
    FILE *mn = setmntent("/etc/mtab", "r");
    if(!mn)
        return QStringList("");

    struct mntent *ent;
    while((ent = getmntent(mn))) {
        if(type == MountpointsAll || supported.contains(ent->mnt_type)) {
            tempList << QString(ent->mnt_dir);
            LOG_INFO() << "Added:" << ent->mnt_dir
                     << "is" << ent->mnt_fsname << "type" << ent->mnt_type;
        }
        else {
            LOG_INFO() << "Ignored:" << ent->mnt_dir
                     << "is" << ent->mnt_fsname << "type" << ent->mnt_type;
        }
    }
    endmntent(mn);

#else
#error Unknown Platform
#endif
    return tempList;
}


/** Check if a process with a given name is running
 *  @param names list of names to filter on. All processes if empty list.
 *  @return list of processname, process ID pairs.
 */
QMap<QString, QList<int> > Utils::findRunningProcess(QStringList names)
{
    QMap<QString, QList<int> > processlist;
    QMap<QString, QList<int> > found;
#if defined(Q_OS_WIN32)
    HANDLE hdl;
    PROCESSENTRY32 entry;
    bool result;

    hdl = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(hdl == INVALID_HANDLE_VALUE) {
        LOG_ERROR() << "CreateToolhelp32Snapshot failed.";
        return found;
    }
    entry.dwSize = sizeof(PROCESSENTRY32);
    entry.szExeFile[0] = '\0';
    if(!Process32First(hdl, &entry)) {
        LOG_ERROR() << "Process32First failed.";
        return found;
    }

    do {
        int pid = entry.th32ProcessID;  // FIXME: DWORD vs int!
        QString name = QString::fromWCharArray(entry.szExeFile);
        if(processlist.find(name) == processlist.end()) {
            processlist.insert(name, QList<int>());
        }
        processlist[name].append(pid);
        entry.dwSize = sizeof(PROCESSENTRY32);
        entry.szExeFile[0] = '\0';
        result = Process32Next(hdl, &entry);
    } while(result);
    CloseHandle(hdl);
#endif
#if defined(Q_OS_MACX)
    ProcessSerialNumber psn = { 0, kNoProcess };
    OSErr err;
    do {
        pid_t pid;
        err = GetNextProcess(&psn);
        err = GetProcessPID(&psn, &pid);
        if(err == noErr) {
            char buf[32] = {0};
            ProcessInfoRec info;
            memset(&info, 0, sizeof(ProcessInfoRec));
            info.processName = (unsigned char*)buf;
            info.processInfoLength = sizeof(ProcessInfoRec);
            err = GetProcessInformation(&psn, &info);
            if(err == noErr) {
                // some processes start with nonprintable characters. Skip those.
                int i;
                for(i = 0; i < 32; i++) {
                    if(isprint(buf[i])) break;
                }
                // avoid adding duplicates.
                QString name = QString::fromUtf8(&buf[i]);
                if(processlist.find(name) == processlist.end()) {
                    processlist.insert(name, QList<int>());
                }
                processlist[name].append(pid);
            }
        }
    } while(err == noErr);
#endif
#if defined(Q_OS_LINUX)
    // not implemented for Linux!
#endif
    //  Filter for names (unless empty)
    if(names.size() > 0) {
        for(int i = 0; i < names.size(); ++i) {
            QStringList k(processlist.keys());
#if defined(Q_OS_WIN32)
            // the process name might be truncated. Allow the extension to be partial.
            int index = k.indexOf(QRegExp(names.at(i) + "(\\.(e(x(e?)?)?)?)?",
                                          Qt::CaseInsensitive));
#else
            int index = k.indexOf(names.at(i));
#endif
            if(index != -1) {
                found.insert(k[index], processlist[k[index]]);
            }
        }
    }
    else {
        found = processlist;
    }
    LOG_INFO() << "Looking for processes" << names << "found" << found;
    return found;
}


/** Suspends/resumes processes
 *  @param pidlist a list of PIDs to suspend/resume
 *  @param suspend processes are suspended if true, or resumed when false
 *  @return a list of PIDs successfully suspended/resumed
 */
QList<int> Utils::suspendProcess(QList<int> pidlist, bool suspend)
{
    QList<int> result;
#if defined(Q_OS_WIN32)
    // Enable debug privilege
    HANDLE hToken = NULL;
    LUID seDebugValue;
    TOKEN_PRIVILEGES tNext, tPrev;
    DWORD sPrev;
    if(LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &seDebugValue)) {
        if(OpenProcessToken(GetCurrentProcess(),
                    TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
            memset(&tNext, 0, sizeof(tNext));
            tNext.PrivilegeCount = 1;
            tNext.Privileges[0].Luid = seDebugValue;
            tNext.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            if(!AdjustTokenPrivileges(hToken, FALSE, &tNext, sizeof(tNext),
                                        &tPrev, &sPrev) || GetLastError() != 0) {
                CloseHandle(hToken);
                hToken = NULL;
                LOG_ERROR() << "AdjustTokenPrivileges(next) error" << GetLastError();
            }
        }
        else {
            LOG_ERROR() << "OpenProcessToken error" << GetLastError();
        }
    }
    else {
        LOG_ERROR() << "LookupPrivilegeValue error" << GetLastError();
    }

    // Suspend/resume threads
    for(int i = 0; i < pidlist.size(); i++) {
        HANDLE hdl = INVALID_HANDLE_VALUE;
        THREADENTRY32 entry;
        int n_fails = 0;

        hdl = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if(hdl == INVALID_HANDLE_VALUE) {
            LOG_ERROR() << "CreateToolhelp32Snapshot error" << GetLastError();
            continue;
        }
        entry.dwSize = sizeof(THREADENTRY32);
        if(!Thread32First(hdl, &entry)) {
            LOG_ERROR() << "Process32First error" << GetLastError();
            CloseHandle(hdl);
            continue;
        }

        do {
            if(entry.th32OwnerProcessID != (DWORD)(pidlist[i]))
                continue;
            HANDLE thr = OpenThread(THREAD_SUSPEND_RESUME,
                                        FALSE, entry.th32ThreadID);
            if(!thr) {
                LOG_ERROR() << "OpenThread" << entry.th32ThreadID
                            << "error" << GetLastError();
                n_fails++;
                continue;
            }
            if(suspend) {
                // Execution of the specified thread is suspended and
                // the thread's suspend count is incremented.
                if(SuspendThread(thr) == (DWORD)(-1)) {
                    LOG_ERROR() << "SuspendThread" << entry.th32ThreadID
                                << "error" << GetLastError();
                    n_fails++;
                }
            }
            else {
                // Decrements a thread's suspend count. When the
                // suspend count is decremented to zero, the
                // execution of the thread is resumed.
                if(ResumeThread(thr) == (DWORD)(-1)) {
                    LOG_ERROR() << "ResumeThread" << entry.th32ThreadID
                                << "error" << GetLastError();
                    n_fails++;
                }
            }
            CloseHandle(thr);
        } while(Thread32Next(hdl, &entry));
        if (!n_fails)
            result.append(pidlist[i]);
        CloseHandle(hdl);
    }

    // Restore previous debug privilege
    if (hToken) {
        if(!AdjustTokenPrivileges(hToken, FALSE,
                    &tPrev, sPrev, NULL, NULL) || GetLastError() != 0) {
            LOG_ERROR() << "AdjustTokenPrivileges(prev) error" << GetLastError();
        }
        CloseHandle(hToken);
    }
#endif
#if defined(Q_OS_MACX)
    int signal = suspend ? SIGSTOP : SIGCONT;
    for(int i = 0; i < pidlist.size(); i++) {
        pid_t pid = pidlist[i];
        if(kill(pid, signal) != 0) {
            LOG_ERROR() << "kill signal" << signal
                        << "for PID" << pid << "error:" << errno;
        }
        else {
            result.append(pidlist[i]);
        }
    }
#endif
#if defined(Q_OS_LINUX)
    // not implemented for Linux!
#endif
    LOG_INFO() << (suspend ? "Suspending" : "Resuming")
               << "PIDs" << pidlist << "result" << result;
    return result;
}


/** Eject device from PC.
 *  Request the OS to eject the player.
 *  @param device mountpoint of the device
 *  @return true on success, fals otherwise.
 */
bool Utils::ejectDevice(QString device)
{
#if defined(Q_OS_WIN32)
    /* See http://support.microsoft.com/kb/165721 on the procedure to eject a
     * device. */
    bool success = false;
    int i;
    HANDLE hdl;
    DWORD bytesReturned;
    TCHAR volume[8];

    /* CreateFile */
    _stprintf(volume, _TEXT("\\\\.\\%c:"), device.toLatin1().at(0));
    hdl = CreateFile(volume, GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, 0, NULL);
    if(hdl == INVALID_HANDLE_VALUE)
        return false;

    /* lock volume to make sure no other application is accessing the volume.
     * Try up to 10 times. */
    for(i = 0; i < 10; i++) {
        if(DeviceIoControl(hdl, FSCTL_LOCK_VOLUME,
                    NULL, 0, NULL, 0, &bytesReturned, NULL))
            break;
        /* short break before retry */
        Sleep(100);
    }
    if(i < 10) {
        /* successfully locked, now dismount */
        if(DeviceIoControl(hdl, FSCTL_DISMOUNT_VOLUME,
                    NULL, 0, NULL, 0, &bytesReturned, NULL)) {
            /* make sure media can be removed. */
            PREVENT_MEDIA_REMOVAL pmr;
            pmr.PreventMediaRemoval = false;
            if(DeviceIoControl(hdl, IOCTL_STORAGE_MEDIA_REMOVAL,
                        &pmr, sizeof(PREVENT_MEDIA_REMOVAL),
                        NULL, 0, &bytesReturned, NULL)) {
                /* eject the media */
                if(DeviceIoControl(hdl, IOCTL_STORAGE_EJECT_MEDIA,
                            NULL, 0, NULL, 0, &bytesReturned, NULL))
                    success = true;
            }
        }
    }
    /* close handle */
    CloseHandle(hdl);
    return success;

#endif
#if defined(Q_OS_MACX)
    // FIXME: FSUnmountVolumeSync is deprecated starting with 10.8.
    // Use DADiskUnmount / DiskArbitration framework eventually.
    // BSD label does not include folder.
    QString bsd = Utils::resolveDevicename(device).remove("/dev/");
    OSStatus result;
    ItemCount index = 1;
    bool found = false;

    do {
        FSVolumeRefNum volrefnum;

        result = FSGetVolumeInfo(kFSInvalidVolumeRefNum, index, &volrefnum,
                kFSVolInfoFSInfo, NULL, NULL, NULL);
        if(result == noErr) {
            GetVolParmsInfoBuffer volparms;
            /* See above -- PBHGetVolParmsSync() is not available for 64bit,
             * and FSGetVolumeParms() on 10.5+ only. */
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 1050
            if(FSGetVolumeParms(volrefnum, &volparms, sizeof(volparms)) == noErr)
#else
            HParamBlockRec hpb;
            hpb.ioParam.ioNamePtr = NULL;
            hpb.ioParam.ioVRefNum = volrefnum;
            hpb.ioParam.ioBuffer = (Ptr)&volparms;
            hpb.ioParam.ioReqCount = sizeof(volparms);
            if(PBHGetVolParmsSync(&hpb) == noErr)
#endif
            {
                if(volparms.vMServerAdr == 0) {
                    if(bsd == (char*)volparms.vMDeviceID) {
                        pid_t dissenter;
                        result = FSUnmountVolumeSync(volrefnum, 0, &dissenter);
                        found = true;
                        break;
                    }
                }
            }
        }
        index++;
    } while(result == noErr);
    if(result == noErr && found)
        return true;

#endif
#if defined(Q_OS_LINUX)
    (void)device;
#endif
    return false;
}

