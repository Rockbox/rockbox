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

#ifdef UNICODE
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
    QString curItem, lstAt;
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
    QStringList elems = path.split("/", QString::SkipEmptyParts);

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
    qDebug() << "[Utils] resolving path" << path << "->" << realpath;
    return realpath;
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
            HParamBlockRec hpb;
            hpb.ioParam.ioNamePtr = NULL;
            hpb.ioParam.ioVRefNum = volrefnum;
            hpb.ioParam.ioBuffer = (Ptr)&volparms;
            hpb.ioParam.ioReqCount = sizeof(volparms);

            if(PBHGetVolParmsSync(&hpb) == noErr) {
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

    qDebug() << "[Utils] Volume name of" << path << "is" << name;
    return name;
}


//! @brief figure the free disk space on a filesystem
//! @param path path on the filesystem to check
//! @return size in bytes
qulonglong Utils::filesystemFree(QString path)
{
    qulonglong size = filesystemSize(path, FilesystemFree);
    qDebug() << "[Utils] free disk space for" << path << size;
    return size;
}


qulonglong Utils::filesystemTotal(QString path)
{
    qulonglong size = filesystemSize(path, FilesystemTotal);
    qDebug() << "[Utils] total disk space for" << path << size;
    return size;
}


qulonglong Utils::filesystemClusterSize(QString path)
{
    qulonglong size = filesystemSize(path, FilesystemClusterSize);
    qDebug() << "[Utils] cluster size for" << path << size;
    return size;
}


qulonglong Utils::filesystemSize(QString path, enum Utils::Size type)
{
    qlonglong size = 0;
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
    QString exepath;
    //try autodetect tts   
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX) || defined(Q_OS_OPENBSD)
    QStringList path = QString(getenv("PATH")).split(":", QString::SkipEmptyParts);
#elif defined(Q_OS_WIN)
    QStringList path = QString(getenv("PATH")).split(";", QString::SkipEmptyParts);
#endif
    qDebug() << "[Utils] system path:" << path;
    for(int i = 0; i < path.size(); i++)
    {
        QString executable = QDir::fromNativeSeparators(path.at(i)) + "/" + name;
#if defined(Q_OS_WIN)
        executable += ".exe";
        QStringList ex = executable.split("\"", QString::SkipEmptyParts);
        executable = ex.join("");
#endif
        if(QFileInfo(executable).isExecutable())
        {
            qDebug() << "[Utils] findExecutable: found" << executable;
            return QDir::toNativeSeparators(executable);
        }
    }
    qDebug() << "[Utils] findExecutable: could not find" << name;
    return "";
}


/** @brief checks different Enviroment things. Ask if user wants to continue.
 *  @param permission if it should check for permission
 *  @return string with error messages if problems occurred, empty strings if none.
 */
QString Utils::checkEnvironment(bool permission)
{
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
       SystemInfo::value(SystemInfo::CurConfigureModel).toString())
    {
        text += tr("<li>Target mismatch detected.<br/>"
                "Installed target: %1<br/>Selected target: %2.</li>")
            .arg(SystemInfo::platformValue(installed, SystemInfo::CurPlatformName).toString(),
                 SystemInfo::value(SystemInfo::CurPlatformName).toString());
    }

    if(!text.isEmpty())
        return tr("Problem detected:") + "<ul>" + text + "</ul>";
    else
        return text;
}
/** @brief Compare two version strings.
 *  @param s1 first version string
 *  @param s2 second version string
 *  @return 0 if strings identical, 1 if second is newer, -1 if first.
 */
int Utils::compareVersionStrings(QString s1, QString s2)
{
    qDebug() << "[Utils] comparing version strings" << s1 << "and" << s2;
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
        int vala = numa.toUInt(&ok1);
        int valb = numb.toUInt(&ok2);
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
    qDebug() << "[Utils] resolving device name" << path;
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
            qDebug() << "[Utils] device name is" << ent->mnt_fsname;
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
            qDebug() << "[Utils] device name is" << mntinf->f_mntfromname;
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
            qDebug() << "[Utils] resolving device name: volume spans multiple disks!";
            return "";
        }
        qDebug() << "[Utils] device name is" << extents->Extents[0].DiskNumber;
        return QString("%1").arg(extents->Extents[0].DiskNumber);
    }
#endif
    return QString("");

}


/** resolve device name to mount point / drive letter
 *  @param device device name / disk number
 *  @return mount point / drive letter
 */
QString Utils::resolveMountPoint(QString device)
{
    qDebug() << "[Utils] resolving mountpoint:" << device;

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
                qDebug() << "[Utils] resolved mountpoint is:" << ent->mnt_dir;
                result = QString(ent->mnt_dir);
            }
            else {
                qDebug() << "[Utils] mountpoint is wrong filesystem!";
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
                qDebug() << "[Utils] resolved mountpoint is:" << mntinf->f_mntonname;
                return QString(mntinf->f_mntonname);
            }
            else {
                qDebug() << "[Utils] mountpoint is wrong filesystem!";
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
            qDebug() << "[Utils] resolved mountpoint is:" << result;
            break;
        }
    }
    if(!result.isEmpty())
        return result + ":/";
#endif
    qDebug() << "[Utils] resolving mountpoint failed!";
    return QString("");
}


QStringList Utils::mountpoints()
{
    QStringList tempList;
#if defined(Q_OS_WIN32)
    QFileInfoList list = QDir::drives();
    for(int i=0; i<list.size();i++)
    {
        tempList << list.at(i).absolutePath();
        qDebug() << "[Utils] Mounted on" << list.at(i).absolutePath();
    }

#elif defined(Q_OS_MACX) || defined(Q_OS_OPENBSD)
    int num;
    struct statfs *mntinf;

    num = getmntinfo(&mntinf, MNT_WAIT);
    while(num--) {
        tempList << QString(mntinf->f_mntonname);
        qDebug() << "[Utils] Mounted on" << mntinf->f_mntonname
                 << "is" << mntinf->f_mntfromname << "type" << mntinf->f_fstypename;
        mntinf++;
    }
#elif defined(Q_OS_LINUX)

    FILE *mn = setmntent("/etc/mtab", "r");
    if(!mn)
        return QStringList("");

    struct mntent *ent;
    while((ent = getmntent(mn))) {
        tempList << QString(ent->mnt_dir);
        qDebug() << "[Utils] Mounted on" << ent->mnt_dir
                 << "is" << ent->mnt_fsname << "type" << ent->mnt_type;
    }
    endmntent(mn);

#else
#error Unknown Platform
#endif
    return tempList;
}


/** Check if a process with a given name is running
 *  @param names list of names to check
 *  @return list of detected processes.
 */
QStringList Utils::findRunningProcess(QStringList names)
{
    QStringList processlist;
    QStringList found;
#if defined(Q_OS_WIN32)
    HANDLE hdl;
    PROCESSENTRY32 entry;
    bool result;

    hdl = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(hdl == INVALID_HANDLE_VALUE) {
        qDebug() << "[Utils] CreateToolhelp32Snapshot failed.";
        return found;
    }
    entry.dwSize = sizeof(PROCESSENTRY32);
    entry.szExeFile[0] = '\0';
    if(!Process32First(hdl, &entry)) {
        qDebug() << "[Utils] Process32First failed.";
        return found;
    }

    processlist.append(QString::fromWCharArray(entry.szExeFile));
    do {
        entry.dwSize = sizeof(PROCESSENTRY32);
        entry.szExeFile[0] = '\0';
        result = Process32Next(hdl, &entry);
        if(result) {
            processlist.append(QString::fromWCharArray(entry.szExeFile));
        }
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
                QString process = QString::fromUtf8(&buf[i]);
                if(!processlist.contains(process)) {
                    processlist.append(process);
                }

            }
        }
    } while(err == noErr);
#endif
    // check for given names in list of processes
    for(int i = 0; i < names.size(); ++i) {
#if defined(Q_OS_WIN32)
        // the process name might be truncated. Allow the extension to be partial.
        int index = processlist.indexOf(QRegExp(names.at(i) + "(\\.(e(x(e?)?)?)?)?"));
#else
        int index = processlist.indexOf(names.at(i));
#endif
        if(index != -1) {
            found.append(processlist.at(index));
        }
    }
    qDebug() << "[Utils] Found listed processes running:" << found;
    return found;
}
