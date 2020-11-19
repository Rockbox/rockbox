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

#include <QtCore>
#include "autodetection.h"
#include "rbsettings.h"
#include "systeminfo.h"

#include "../ipodpatcher/ipodpatcher.h"
#include "../sansapatcher/sansapatcher.h"


#include "system.h"
#include "utils.h"
#include "rockboxinfo.h"
#include "Logger.h"

Autodetection::Autodetection(QObject* parent): QObject(parent)
{
}


bool Autodetection::detect(void)
{
    QMap<PlayerStatus, QString> states;
    states[PlayerOk] = "Ok";
    states[PlayerAmbiguous] = "Ambiguous";
    states[PlayerError] = "Error";
    states[PlayerIncompatible] = "Incompatible";
    states[PlayerMtpMode] = "MtpMode";

    // clear detection state
    m_detected.clear();

    detectUsb();
    mergeMounted();
    mergePatcher();
    // if any entry with usbdevices containing a value is left that entry
    // hasn't been merged later. This indicates a problem during detection
    // (ambiguous player but refining it failed). In this case create an entry
    // for eacho of those so the user can select.
    for(int i = 0; i < m_detected.size(); ++i) {
        int j = m_detected.at(i).usbdevices.size();
        if(j > 0) {
            struct Detected entry = m_detected.takeAt(i);
            while(j--) {
                struct Detected d;
                d.device = entry.usbdevices.at(j);
                d.mountpoint = entry.mountpoint;
                d.status = PlayerAmbiguous;
                m_detected.append(d);
            }
        }
    }
    for(int i = 0; i < m_detected.size(); ++i) {
        LOG_INFO() << "Detected player:" << m_detected.at(i).device
                   << "at" << m_detected.at(i).mountpoint << states[m_detected.at(i).status];
    }

    return m_detected.size() > 0;
}


/** @brief detect devices based on usb pid / vid.
 */
void Autodetection::detectUsb()
{
    // usbids holds the mapping in the form
    // ((VID<<16)|(PID)), targetname
    // the ini file needs to hold the IDs as hex values.
    QMap<int, QStringList> usbids = SystemInfo::usbIdMap(SystemInfo::MapDevice);
    QMap<int, QStringList> usberror = SystemInfo::usbIdMap(SystemInfo::MapError);

    // usb pid detection
    QList<uint32_t> attached;
    attached = System::listUsbIds();

    int i = attached.size();
    while(i--) {
        if(usbids.contains(attached.at(i))) {
            // we found a USB device that might be ambiguous.
            struct Detected d;
            d.status = PlayerOk;
            d.usbdevices = usbids.value(attached.at(i));
            m_detected.append(d);
            LOG_INFO() << "[USB] detected supported player" << d.usbdevices;
        }
        if(usberror.contains(attached.at(i))) {
            struct Detected d;
            d.status = PlayerMtpMode;
            d.device = usberror.value(attached.at(i)).at(0);
            m_detected.append(d);
            LOG_WARNING() << "[USB] detected problem with player" << d.device;
        }
        QString idstring = QString("%1").arg(attached.at(i), 8, 16, QChar('0'));
        if(!SystemInfo::platformValue(SystemInfo::Name, idstring).toString().isEmpty()) {
            struct Detected d;
            d.status = PlayerIncompatible;
            d.device = idstring;
            m_detected.append(d);
            LOG_WARNING() << "[USB] detected incompatible player" << d.device;
        }
    }
}


// Merge players detected by checking mounted filesystems for known files:
// - rockbox-info.txt / rbutil.log
// - player specific files
void Autodetection::mergeMounted(void)
{
    QStringList mounts = Utils::mountpoints(Utils::MountpointsSupported);
    LOG_INFO() << "paths to check:" << mounts;

    for(int i = 0; i < mounts.size(); i++)
    {
        // do the file checking
        QDir dir(mounts.at(i));
        if(dir.exists())
        {
            // check logfile first.
            if(QFile(mounts.at(i) + "/.rockbox/rbutil.log").exists()) {
                QSettings log(mounts.at(i) + "/.rockbox/rbutil.log",
                              QSettings::IniFormat, this);
                if(!log.value("platform").toString().isEmpty()) {
                    struct Detected d;
                    d.device = log.value("platform").toString();
                    d.mountpoint = mounts.at(i);
                    d.status = PlayerOk;
                    updateDetectedDevice(d);
                    LOG_INFO() << "rbutil.log detected:"
                               << log.value("platform").toString() << mounts.at(i);
                }
            }

            // check rockbox-info.txt afterwards.
            RockboxInfo info(mounts.at(i));
            if(info.success())
            {
                struct Detected d;
                d.device = info.target();
                d.mountpoint = mounts.at(i);
                d.status = PlayerOk;
                updateDetectedDevice(d);
                LOG_INFO() << "rockbox-info.txt detected:"
                           << info.target() << mounts.at(i);
            }

            // check for some specific files in root folder
            QDir root(mounts.at(i));
            QStringList rootentries = root.entryList(QDir::Files);
            if(rootentries.contains("archos.mod", Qt::CaseInsensitive))
            {
                // archos.mod in root folder -> Archos Player
                struct Detected d;
                d.device = "player";
                d.mountpoint = mounts.at(i);
                d.status = PlayerOk;
                updateDetectedDevice(d);
            }
            if(rootentries.contains("ONDIOST.BIN", Qt::CaseInsensitive))
            {
                // ONDIOST.BIN in root -> Ondio FM
                struct Detected d;
                d.device = "ondiofm";
                d.mountpoint = mounts.at(i);
                d.status = PlayerOk;
                updateDetectedDevice(d);
            }
            if(rootentries.contains("ONDIOSP.BIN", Qt::CaseInsensitive))
            {
                // ONDIOSP.BIN in root -> Ondio SP
                struct Detected d;
                d.device = "ondiosp";
                d.mountpoint = mounts.at(i);
                d.status = PlayerOk;
                updateDetectedDevice(d);
            }
            if(rootentries.contains("ajbrec.ajz", Qt::CaseInsensitive))
            {
                LOG_INFO() << "ajbrec.ajz found. Trying detectAjbrec()";
                struct Detected d;
                d.device = detectAjbrec(mounts.at(i));
                d.mountpoint = mounts.at(i);
                d.status = PlayerOk;
                if(!d.device.isEmpty()) {
                    LOG_INFO() << d.device;
                    updateDetectedDevice(d);
                }
            }
            // detection based on player specific folders
            QStringList rootfolders = root.entryList(QDir::Dirs
                    | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
            if(rootfolders.contains("GBSYSTEM", Qt::CaseInsensitive))
            {
                // GBSYSTEM folder -> Gigabeat
                struct Detected d;
                d.device = "gigabeatf";
                d.mountpoint = mounts.at(i);
                updateDetectedDevice(d);
            }
        }
    }
#if 0
    // Ipods have a folder "iPod_Control" in the root.
    for(int i = 0; i < m_detected.size(); ++i) {
        struct Detected entry = m_detected.at(i);
        for(int j = 0; j < entry.usbdevices.size(); ++j) {
            // limit this to Ipods only.
            if(!entry.usbdevices.at(j).startsWith("ipod")
                    && !entry.device.startsWith("ipod")) {
                continue;
            }
            // look for iPod_Control on all supported volumes.
            for(int k = 0; k < mounts.size(); k++) {
                QDir root(mounts.at(k));
                QStringList rootfolders = root.entryList(QDir::Dirs
                        | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
                if(rootfolders.contains("iPod_Control", Qt::CaseInsensitive)) {
                    entry.mountpoint = mounts.at(k);
                    m_detected.takeAt(i);
                    m_detected.append(entry);
                }
            }
        }
    }
#endif

}


void Autodetection::mergePatcher(void)
{
    int n;
    // try ipodpatcher
    // initialize sector buffer. Needed.
    struct ipod_t ipod;
    ipod.sectorbuf = nullptr;
    ipod_alloc_buffer(&ipod, BUFFER_SIZE);
    n = ipod_scan(&ipod);
    // FIXME: handle more than one Ipod connected in ipodpatcher.
    if(n == 1) {
        LOG_INFO() << "Ipod found:" << ipod.modelstr << "at" << ipod.diskname;
        // since resolveMountPoint is doing exact matches we need to select
        // the correct partition.
        QString mp(ipod.diskname);
#ifdef Q_OS_LINUX
        mp.append("2");
#endif
#ifdef Q_OS_MACX
        mp.append("s2");
#endif
        struct Detected d;
        d.device = ipod.targetname;
        d.mountpoint = Utils::resolveMountPoint(mp);
        // if the found ipod is a macpod also notice it as device with problem.
        if(ipod.macpod)
            d.status = PlayerWrongFilesystem;
        else
            d.status = PlayerOk;
        updateDetectedDevice(d);
    }
    else {
        LOG_INFO() << "ipodpatcher: no Ipod found." << n;
    }
    ipod_dealloc_buffer(&ipod);

    // try sansapatcher
    // initialize sector buffer. Needed.
    struct sansa_t sansa;
    sansa_alloc_buffer(&sansa, BUFFER_SIZE);
    n = sansa_scan(&sansa);
    if(n == 1) {
        LOG_INFO() << "Sansa found:"
                   << sansa.targetname << "at" << sansa.diskname;
        QString mp(sansa.diskname);
#ifdef Q_OS_LINUX
        mp.append("1");
#endif
#ifdef Q_OS_MACX
        mp.append("s1");
#endif
        struct Detected d;
        d.device = QString("sansa%1").arg(sansa.targetname);
        d.mountpoint = Utils::resolveMountPoint(mp);
        d.status = PlayerOk;
        updateDetectedDevice(d);
    }
    else {
        LOG_INFO() << "sansapatcher: no Sansa found." << n;
    }
    sansa_dealloc_buffer(&sansa);
}


QString Autodetection::detectAjbrec(QString root)
{
    QFile f(root + "/ajbrec.ajz");
    char header[24];
    f.open(QIODevice::ReadOnly);
    if(!f.read(header, 24)) return QString();
    f.close();

    // check the header of the file.
    // recorder v1 had a 6 bytes sized header
    // recorder v2, FM, Ondio SP and FM have a 24 bytes header.

    // recorder v1 has the binary length in the first 4 bytes, so check
    // for them first.
    int len = (header[0]<<24) | (header[1]<<16) | (header[2]<<8) | header[3];
    LOG_INFO() << "abjrec.ajz possible bin length:" << len
               << "file len:" << f.size();
    if((f.size() - 6) == len)
        return "recorder";

    // size didn't match, now we need to assume we have a headerlength of 24.
    switch(header[11]) {
        case 2:
            return "recorderv2";
            break;

        case 4:
            return "fmrecorder";
            break;

        case 8:
            return "ondiofm";
            break;

        case 16:
            return "ondiosp";
            break;

        default:
            break;
    }
    return QString();
}


int Autodetection::findDetectedDevice(QString device)
{
    int i = m_detected.size();
    while(i--) {
        if(m_detected.at(i).usbdevices.contains(device))
            return i;
    }
    i = m_detected.size();
    while(i--) {
        if(m_detected.at(i).device == device)
            return i;
    }
    return -1;
}


void Autodetection::updateDetectedDevice(Detected& entry)
{
    int index = findDetectedDevice(entry.device);
    if(index < 0) {
        m_detected.append(entry);
    }
    else {
        m_detected.takeAt(index);
        m_detected.append(entry);
    }
}
