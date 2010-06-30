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

#if defined(Q_OS_WIN32)
#include <windows.h>
#include <tchar.h>
#include <winioctl.h>
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
#include <sys/statvfs.h>
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
    qDebug() << "[Utils] resolving path" << path << "->" << realpath;
    return realpath;
}


//! @brief figure the free disk space on a filesystem
//! @param path path on the filesystem to check
//! @return size in bytes
qulonglong Utils::filesystemFree(QString path)
{
    qlonglong size = 0;
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX) 
    // the usage of statfs() is deprecated by the LSB so use statvfs().
    struct statvfs fs;
    int ret;

    ret = statvfs(qPrintable(path), &fs);

    if(ret == 0)
        size = (qulonglong)fs.f_frsize * (qulonglong)fs.f_bavail;
#endif
#if defined(Q_OS_WIN32)
    BOOL ret;
    ULARGE_INTEGER freeAvailBytes;

    ret = GetDiskFreeSpaceExW((LPCTSTR)path.utf16(), &freeAvailBytes, NULL, NULL);
    if(ret)
        size = freeAvailBytes.QuadPart;
#endif
    qDebug() << "[Utils] Filesystem free:" << path << size;
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
        qDebug() << "[Utils] executable:" << executable;
        if(QFileInfo(executable).isExecutable())
        {
            return QDir::toNativeSeparators(executable);
        }
    }
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
        text += tr("<li>Target mismatch detected.\n"
                "Installed target: %1, selected target: %2.</li>")
            .arg(installed, SystemInfo::value(SystemInfo::CurPlatformName).toString());
            // FIXME: replace installed by human-friendly name
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

