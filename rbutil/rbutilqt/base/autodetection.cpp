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


#include "system.h"
#include "utils.h"
#include "rockboxinfo.h"

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
    QStringList mounts = Utils::mountpoints();

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
        // since resolveMountPoint is doing exact matches we need to select
        // the correct partition.
        QString mp(ipod.diskname);
#ifdef Q_OS_LINUX
        mp.append("2");
#endif
#ifdef Q_OS_MACX
        mp.append("s2");
#endif
        m_mountpoint = Utils::resolveMountPoint(mp);
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
        QString mp(sansa.diskname);
#ifdef Q_OS_LINUX
        mp.append("1");
#endif
#ifdef Q_OS_MACX
        mp.append("s1");
#endif
        m_mountpoint = Utils::resolveMountPoint(mp);
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

