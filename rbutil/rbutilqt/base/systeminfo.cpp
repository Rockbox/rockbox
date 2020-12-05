/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2010 by Dominik Wenger
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "systeminfo.h"
#include "rbsettings.h"

#include <QSettings>
#include "Logger.h"

// device settings

//! pointer to setting object to nullptr
QSettings* SystemInfo::systemInfos = nullptr;

void SystemInfo::ensureSystemInfoExists()
{
    //check and create settings object
    if(systemInfos == nullptr)
    {
        // only use built-in rbutil.ini
        systemInfos = new QSettings(":/ini/rbutil.ini", QSettings::IniFormat);
    }
}


QMap<int, QStringList> SystemInfo::usbIdMap(enum MapType type)
{
    ensureSystemInfoExists();

    QMap<int, QStringList> map;
    // get a list of ID -> target name
    QStringList platforms;
    systemInfos->beginGroup("platforms");
    platforms = systemInfos->childKeys();
    systemInfos->endGroup();

    QString t;
    switch(type) {
        case MapDevice:
            t = "usbid";
            break;
        case MapError:
            t = "usberror";
            break;
        case MapIncompatible:
            t = "usbincompat";
            break;
    }

    for(int i = 0; i < platforms.size(); i++)
    {
        systemInfos->beginGroup("platforms");
        QString target = systemInfos->value(platforms.at(i)).toString();
        systemInfos->endGroup();
        systemInfos->beginGroup(target);
        QStringList ids = systemInfos->value(t).toStringList();
        int j = ids.size();
        while(j--) {
            QStringList l;
            int id = ids.at(j).toInt(nullptr, 16);
            if(id == 0) {
                continue;
            }
            if(map.contains(id)) {
                l = map.take(id);
            }
            l.append(target);
            map.insert(id, l);
        }
        systemInfos->endGroup();
    }
    return map;
}


