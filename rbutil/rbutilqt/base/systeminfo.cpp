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
const static struct {
    SystemInfo::SystemInfos info;
    const char* name;
} SystemInfosList[] = {
    { SystemInfo::ManualUrl,            ":build:/manual_url"      },
    { SystemInfo::BuildUrl,             ":build:/build_url"       },
    { SystemInfo::FontUrl,              ":build:/font_url"        },
    { SystemInfo::VoiceUrl,             ":build:/voice_url"       },
    { SystemInfo::BootloaderUrl,        "bootloader/download_url" },
    { SystemInfo::BootloaderInfoUrl,    "bootloader/info_url"     },
    { SystemInfo::DoomUrl,              "doom_url"                },
    { SystemInfo::Duke3DUrl,            "duke3d_url"              },
    { SystemInfo::PuzzFontsUrl,         "puzzfonts_url"           },
    { SystemInfo::QuakeUrl,             "quake_url"               },
    { SystemInfo::Wolf3DUrl,            "wolf3d_url"              },
    { SystemInfo::XWorldUrl,            "xworld_url"              },
    { SystemInfo::BuildInfoUrl,         "build_info_url"          },
    { SystemInfo::GenlangUrl,           "genlang_url"             },
    { SystemInfo::ThemesUrl,            "themes_url"              },
    { SystemInfo::ThemesInfoUrl,        "themes_info_url"         },
    { SystemInfo::RbutilUrl,            "rbutil_url"              },
};

const static struct {
    SystemInfo::PlatformInfo info;
    const char* name;
    const char* def;
} PlatformInfosList[] = {
    { SystemInfo::Manual,           ":platform:/manualname",        ":platform:" },
    { SystemInfo::BootloaderMethod, ":platform:/bootloadermethod",  "none" },
    { SystemInfo::BootloaderName,   ":platform:/bootloadername",    "" },
    { SystemInfo::BootloaderFile,   ":platform:/bootloaderfile",    "" },
    { SystemInfo::BootloaderFilter, ":platform:/bootloaderfilter",  "" },
    { SystemInfo::Encoder,          ":platform:/encoder",           "" },
    { SystemInfo::Brand,            ":platform:/brand",             "" },
    { SystemInfo::Name,             ":platform:/name",              "" },
    { SystemInfo::ConfigureModel,   ":platform:/configure_modelname", "" },
    { SystemInfo::PlayerPicture,    ":platform:/playerpic",         "" },
};

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


QVariant SystemInfo::value(enum SystemInfos info, BuildType type)
{
    ensureSystemInfoExists();

    // locate setting item
    int i = 0;
    while(SystemInfosList[i].info != info)
        i++;
    QString s = SystemInfosList[i].name;
    switch(type) {
    case BuildDaily:
        s.replace(":build:", "daily");
        break;
    case BuildCurrent:
        s.replace(":build:", "development");
        break;
    case BuildCandidate:
        s.replace(":build:", "release-candidate");
        break;
    case BuildRelease:
        s.replace(":build:", "release");
        break;
    }
    LOG_INFO() << "GET:" << s << systemInfos->value(s).toString();
    return systemInfos->value(s);
}

QVariant SystemInfo::platformValue(enum PlatformInfo info, QString platform)
{
    ensureSystemInfoExists();

    // locate setting item
    int i = 0;
    while(PlatformInfosList[i].info != info)
        i++;

    if (platform.isEmpty())
        platform = RbSettings::value(RbSettings::CurrentPlatform).toString();

    QString s = PlatformInfosList[i].name;
    s.replace(":platform:", platform);
    QString d = PlatformInfosList[i].def;
    d.replace(":platform:", platform);
    LOG_INFO() << "GET P:" << s << systemInfos->value(s, d).toString();
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


