/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2010 by Dominik Wenger
 *   $Id$
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

#if defined(Q_OS_LINUX)
#include <unistd.h>
#endif


// device settings
const static struct {
    SystemInfo::SystemInfos info;
    const char* name;
    const char* def;
} SystemInfosList[] = {
    { SystemInfo::ManualUrl,            "manual_url",           "" },
    { SystemInfo::BleedingUrl,          "bleeding_url",         "" },
    { SystemInfo::BootloaderUrl,        "bootloader_url",       "" },
    { SystemInfo::BootloaderInfoUrl,    "bootloader_info_url",  "" },
    { SystemInfo::FontUrl,              "font_url",             "" },
    { SystemInfo::VoiceUrl,             "voice_url",            "" },
    { SystemInfo::DoomUrl,              "doom_url",             "" },
    { SystemInfo::ReleaseUrl,           "release_url",          "" },
    { SystemInfo::DailyUrl,             "daily_url",            "" },
    { SystemInfo::ServerConfUrl,        "server_conf_url",      "" },
    { SystemInfo::GenlangUrl,           "genlang_url",          "" },
    { SystemInfo::ThemesUrl,            "themes_url",           "" },
    { SystemInfo::RbutilUrl,            "rbutil_url",           "" },
    { SystemInfo::BleedingInfo,         "bleeding_info",        "" },
    { SystemInfo::CurPlatformName,      ":platform:/name",      "" },
    { SystemInfo::CurManual,            ":platform:/manualname","rockbox-:platform:" },
    { SystemInfo::CurBootloaderMethod,  ":platform:/bootloadermethod", "none" },
    { SystemInfo::CurBootloaderName,    ":platform:/bootloadername", "" },
    { SystemInfo::CurBootloaderFile,    ":platform:/bootloaderfile", "" },
    { SystemInfo::CurEncoder,           ":platform:/encoder",   "" },
    { SystemInfo::CurBrand,             ":platform:/brand",     "" },
    { SystemInfo::CurName,              ":platform:/name",      "" },
    { SystemInfo::CurBuildserverModel,  ":platform:/buildserver_modelname", "" },
    { SystemInfo::CurConfigureModel,    ":platform:/configure_modelname", "" },
};

//! pointer to setting object to NULL
QSettings* SystemInfo::systemInfos = NULL;

void SystemInfo::ensureSystemInfoExists()
{
    //check and create settings object
    if(systemInfos == NULL)
    {
        // only use built-in rbutil.ini
        systemInfos = new QSettings(":/ini/rbutil.ini", QSettings::IniFormat, 0);
    }
}


QVariant SystemInfo::value(enum SystemInfos info)
{
    ensureSystemInfoExists();

    // locate setting item
    int i = 0;
    while(SystemInfosList[i].info != info)
        i++;
    QString platform = RbSettings::value(RbSettings::CurrentPlatform).toString();
    QString s = SystemInfosList[i].name;
    s.replace(":platform:", platform);
    QString d = SystemInfosList[i].def;
    d.replace(":platform:", platform);
    qDebug() << "[SystemInfo] GET:" << s << systemInfos->value(s, d).toString();
    return systemInfos->value(s, d);
}

QVariant SystemInfo::platformValue(QString platform, enum SystemInfos info)
{
    ensureSystemInfoExists();

    // locate setting item
    int i = 0;
    while(SystemInfosList[i].info != info)
        i++;

    QString s = SystemInfosList[i].name;
    s.replace(":platform:", platform);
    QString d = SystemInfosList[i].def;
    d.replace(":platform:", platform);
    qDebug() << "[SystemInfo] GET P:" << s << systemInfos->value(s, d).toString();
    return systemInfos->value(s, d);
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
        // only add target if its not disabled
        if(type != PlatformAllDisabled
                && systemInfos->value(target+"/status").toString() == "disabled")
            continue;
        // report only base targets when PlatformBase is requested
        if(type == PlatformBase && target.contains('.'))
            continue;
        // report only matching target if PlatformVariant is requested
        if(type == PlatformVariant && !target.startsWith(variant))
            continue;
        result.append(target);
    }
    return result;
}

QStringList SystemInfo::languages()
{
    ensureSystemInfoExists();

    QStringList result;
    systemInfos->beginGroup("languages");
    QStringList a = systemInfos->childKeys();
    for(int i = 0; i < a.size(); i++)
    {
        result.append(systemInfos->value(a.at(i), "null").toString());
    }
    systemInfos->endGroup();
    return result;
}


QString SystemInfo::name(QString platform)
{
    ensureSystemInfoExists();
    return systemInfos->value(platform + "/name").toString();
}

QString SystemInfo::brand(QString platform)
{
    ensureSystemInfoExists();
    return systemInfos->value(platform + "/brand").toString();
}

QMap<int, QString> SystemInfo::usbIdMap(enum MapType type)
{
    ensureSystemInfoExists();

    QMap<int, QString> map;
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
        while(j--)
            map.insert(ids.at(j).toInt(0, 16), target);

        systemInfos->endGroup();
    }
    return map;
}


