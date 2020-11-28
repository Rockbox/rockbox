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


QStringList SystemInfo::platforms(enum SystemInfo::PlatformType type, QString variant)
{
    ensureSystemInfoExists();

    QStringList result;
    systemInfos->beginGroup("platforms");
    QStringList a = systemInfos->childKeys();
    systemInfos->endGroup();
    for(int i = 0; i < a.size(); i++)
    {
        QString target = systemInfos->value("platforms/"+a.at(i), "null").toString();
        QRegExp regex("\\..*$");
        QString targetbase = target;
        targetbase.remove(regex);
        // only add target if its not disabled unless Platform*Disabled requested
        if(type != PlatformAllDisabled && type != PlatformBaseDisabled
                && type != PlatformVariantDisabled
                && systemInfos->value(target+"/status").toString() == "disabled")
            continue;
        // report only matching target if PlatformVariant* is requested
        if((type == PlatformVariant || type == PlatformVariantDisabled)
                && (targetbase != variant))
            continue;
        // report only base targets when PlatformBase* is requested
        if((type == PlatformBase || type == PlatformBaseDisabled))
            result.append(targetbase);
        else
            result.append(target);
    }
    result.removeDuplicates();
    return result;
}

QMap<QString, QStringList> SystemInfo::languages(bool namesOnly)
{
    ensureSystemInfoExists();

    QMap<QString, QStringList> result;
    systemInfos->beginGroup("languages");
    QStringList a = systemInfos->childKeys();
    for(int i = 0; i < a.size(); i++)
    {
        QStringList data = systemInfos->value(a.at(i), "null").toStringList();
        if(namesOnly)
            result.insert(data.at(0), QStringList(data.at(1)));
        else
            result.insert(a.at(i), data);
    }
    systemInfos->endGroup();
    return result;
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


