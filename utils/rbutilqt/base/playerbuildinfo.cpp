/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2020 by Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "playerbuildinfo.h"
#include "rbsettings.h"
#include "Logger.h"

PlayerBuildInfo* PlayerBuildInfo::infoInstance = nullptr;

PlayerBuildInfo* PlayerBuildInfo::instance()
{
    if (infoInstance == nullptr) {
        infoInstance = new PlayerBuildInfo();
    }
    return infoInstance;
}

// server infos
const static struct {
    PlayerBuildInfo::BuildInfo item;
    const char* name;
} ServerInfoList[] = {
    { PlayerBuildInfo::BuildVoiceLangs,   "voices/:version:"    },
    { PlayerBuildInfo::BuildVersion,      ":build:/:target:"     },
    { PlayerBuildInfo::BuildUrl,          ":build:/build_url"   },
    { PlayerBuildInfo::BuildVoiceUrl,     ":build:/voice_url"   },
    { PlayerBuildInfo::BuildManualUrl,    ":build:/manual_url"  },
    { PlayerBuildInfo::BuildSourceUrl,    ":build:/source_url"  },
    { PlayerBuildInfo::BuildFontUrl,      ":build:/font_url"    },

    // other URLs -- those are not directly related to the build, but handled here.
    { PlayerBuildInfo::DoomUrl,           "other/doom_url"      },
    { PlayerBuildInfo::Duke3DUrl,         "other/duke3d_url"    },
    { PlayerBuildInfo::PuzzFontsUrl,      "other/puzzfonts_url" },
    { PlayerBuildInfo::QuakeUrl,          "other/quake_url"     },
    { PlayerBuildInfo::Wolf3DUrl,         "other/wolf3d_url"    },
    { PlayerBuildInfo::XRickUrl,          "other/xrick_url"     },
    { PlayerBuildInfo::XWorldUrl,         "other/xworld_url"    },
    { PlayerBuildInfo::MidiPatchsetUrl,   "other/patcheset_url" },
};

const static struct {
    PlayerBuildInfo::DeviceInfo item;
    const char* name;
} PlayerInfoList[] = {
    { PlayerBuildInfo::BuildStatus,        "status/:target:"           },
    { PlayerBuildInfo::DisplayName,        ":target:/name"             },
    { PlayerBuildInfo::BootloaderMethod,   ":target:/bootloadermethod" },
    { PlayerBuildInfo::BootloaderName,     ":target:/bootloadername"   },
    { PlayerBuildInfo::BootloaderFile,     ":target:/bootloaderfile"   },
    { PlayerBuildInfo::BootloaderFilter,   ":target:/bootloaderfilter" },
    { PlayerBuildInfo::Encoder,            ":target:/encoder"          },
    { PlayerBuildInfo::Brand,              ":target:/brand"            },
    { PlayerBuildInfo::PlayerPicture,      ":target:/playerpic"        },
    { PlayerBuildInfo::ThemeName,          ":target:/themename"        },
    { PlayerBuildInfo::TargetNamesAll,     "_targets/all"              },
    { PlayerBuildInfo::TargetNamesEnabled, "_targets/enabled"          },
    { PlayerBuildInfo::LanguageInfo,       "languages/:target:"        },
    { PlayerBuildInfo::LanguageList,       "_languages/list"           },
    { PlayerBuildInfo::UsbIdErrorList,     "_usb/error"                },
    { PlayerBuildInfo::UsbIdTargetList,    "_usb/target"               },
};

const static struct {
    PlayerBuildInfo::SystemUrl item;
    const char* name;
} PlayerSystemUrls[] = {
    { PlayerBuildInfo::BootloaderUrl,     "bootloader/download_url" },
    { PlayerBuildInfo::BuildInfoUrl,      "build_info_url"          },
    { PlayerBuildInfo::GenlangUrl,        "genlang_url"             },
    { PlayerBuildInfo::ThemesUrl,         "themes_url"              },
    { PlayerBuildInfo::ThemesInfoUrl,     "themes_info_url"         },
    { PlayerBuildInfo::RbutilUrl,         "rbutil_url"              },
};

PlayerBuildInfo::PlayerBuildInfo() :
     serverInfo(nullptr),
     playerInfo(":/ini/rbutil.ini", QSettings::IniFormat)
{

}

void PlayerBuildInfo::setBuildInfo(QString file)
{
    if (serverInfo)
        delete serverInfo;
    LOG_INFO() << "updated:" << file;
    serverInfo = new QSettings(file, QSettings::IniFormat);
}

QVariant PlayerBuildInfo::value(BuildInfo item, BuildType type)
{
    // locate setting item in server info file
    int i = 0;
    while(ServerInfoList[i].item != item)
        i++;

    // split of variant for target.
    // we can have an optional variant part in the target string.
    // For build info we don't use that.
    QString target = RbSettings::value(RbSettings::CurrentPlatform).toString().split('.').at(0);

    QString serverinfo = ServerInfoList[i].name;
    serverinfo.replace(":target:", target);
    QString buildtypename;
    switch(type) {
    case TypeRelease:
        buildtypename = "release";
        break;
    case TypeCandidate:
        buildtypename = "release-candidate";
        break;
    case TypeDaily:
        buildtypename = "daily";
        break;
    case TypeDevel:
        buildtypename = "development";
        // manual and fonts don't exist for development builds. We do have an
        // URL configured, but need to get the daily version instead.
        if(item == BuildManualUrl || item == BuildFontUrl) {
            LOG_INFO() << "falling back to daily build for this info value";
            buildtypename = "daily";
        }
        break;
    }

    QVariant result;
    if (!serverInfo)
        return QString();
    QStringList version = serverInfo->value(buildtypename + "/" + target, "").toStringList();
    serverinfo.replace(":build:", buildtypename);
    serverinfo.replace(":version:", version.at(0));

    // get value from server build-info
    // we need to get a version string, otherwise the data is invalid.
    // For invalid data return an empty string.
    if(version.at(0).isEmpty()) {
        LOG_INFO() << serverinfo << "(version invalid)";
        return QString();
    }
    if(!serverinfo.isEmpty())
        result = serverInfo->value(serverinfo);

    // depending on the actual value we need more replacements.
    switch(item) {
    case BuildVersion:
        result = result.toStringList().at(0);
        break;

    case BuildUrl:
        if(version.size() > 1) {
            // version info has an URL appended. Takes precendence.
            result = version.at(1);
        }
        break;

    case BuildVoiceLangs:
        if (type == TypeDaily)
            serverinfo = "voices/daily";
        result = serverInfo->value(serverinfo);
        break;

    case BuildManualUrl:
    {
        // special case: if playerInfo has a non-empty manualname entry for the
        // target, use that as target for the manual name.
        QString manualtarget = playerInfo.value(target + "/manualname", "").toString();
        if(!manualtarget.isEmpty())
            target = manualtarget;
        break;
    }

    default:
        break;
    }
    // if the value is a string we can replace some patterns.
    // if we cannot convert it (f.e. for a QStringList) we leave as-is, since
    // the conversion would return an empty type.
#if QT_VERSION < 0x060000
    if (result.type() == QVariant::String)
#else
    if (result.metaType().id() == QMetaType::QString)
#endif
        result = result.toString()
                    .replace("%TARGET%", target)
                    .replace("%VERSION%", version.at(0));

    LOG_INFO() << "B:" << serverinfo << result;
    return result;
}

QVariant PlayerBuildInfo::value(DeviceInfo item, QString target)
{
    // locate setting item in server info file
    int i = 0;
    while(PlayerInfoList[i].item != item)
        i++;

    // split of variant for target.
    // we can have an optional variant part in the target string.
    // For device info we use this.
    if (target.isEmpty())
        target = RbSettings::value(RbSettings::CurrentPlatform).toString();

    QVariant result = QString();

    QString s = PlayerInfoList[i].name;
    s.replace(":target:", target);

    switch(item) {
    case BuildStatus:
    {
        // build status is the only value that doesn't depend on the version
        // but the selected target instead.
        bool ok = false;
        if (serverInfo)
            result = serverInfo->value(s).toInt(&ok);
        if (!ok)
            result = -1;
        break;
    }
    case TargetNamesAll:
        // list of all internal target names. Doesn't depend on the passed target.
        result = targetNames(true);
        break;
    case TargetNamesEnabled:
        // list of all non-disabled target names. Doesn't depend on the passed target.
        result = targetNames(false);
        break;

    case LanguageList:
        // Return a map (language, display string).
        {
            // need to use (QString, QVariant) here, so we can put the map into
            // a QVariant by itself.
            QMap<QString, QVariant> m;

            playerInfo.beginGroup("languages");
            QStringList a = playerInfo.childKeys();

            for(int i = 0; i < a.size(); i++) {
                QStringList v = playerInfo.value(a.at(i)).toStringList();
                m[v.at(0)] = v.at(1);
            }
            playerInfo.endGroup();
            result = m;
        }
        break;

    default:
        result = playerInfo.value(s, "");
        break;
    }

    LOG_INFO() << "T:" << s << result;
    return result;
}

QVariant PlayerBuildInfo::value(DeviceInfo item, unsigned int match)
{
    QStringList result;
    int i = 0;
    while(PlayerInfoList[i].item != item)
        i++;
    QString s = PlayerInfoList[i].name;

    switch(item) {
    case UsbIdErrorList:
    {
        // go through all targets and find the one indicated by the usb id "target".
        // return list of matching players (since it could be more than one)
        QStringList targets = targetNames(true);
        for(int i = 0; i < targets.size(); i++) {
            QStringList usbids = playerInfo.value(targets.at(i) + "/usberror").toStringList();
            for(int j = 0; j < usbids.size(); j++) {
                if(usbids.at(j).toUInt(nullptr, 0) == match) {
                    result << targets.at(i);
                }
            }
        }
        break;
    }

    case UsbIdTargetList:
    {
        QStringList targets = targetNames(true);
        for(int i = 0; i < targets.size(); i++) {
            QStringList usbids = playerInfo.value(targets.at(i) + "/usbid").toStringList();
            for(int j = 0; j < usbids.size(); j++) {
                if(usbids.at(j).toUInt(nullptr, 0) == match) {
                    result << targets.at(i);
                }
            }
        }
        break;
    }

    default:
        break;
    }
    LOG_INFO() << "T:" << s << result;
    return result;
}

QVariant PlayerBuildInfo::value(SystemUrl item)
{
    // locate setting item in server info file
    int i = 0;
    while(PlayerSystemUrls[i].item != item)
        i++;

    QVariant result = playerInfo.value(PlayerSystemUrls[i].name);
    LOG_INFO() << "U:" << PlayerSystemUrls[i].name << result;
    return result;
}


QString PlayerBuildInfo::statusAsString(QString platform)
{
    QString result;
    switch(value(BuildStatus, platform.split('.').at(0)).toInt())
    {
    case STATUS_RETIRED:
        result = tr("Stable (Retired)");
        break;
    case STATUS_UNUSABLE:
        result = tr("Unusable");
        break;
    case STATUS_UNSTABLE:
        result = tr("Unstable");
        break;
    case STATUS_STABLE:
        result = tr("Stable");
        break;
    default:
        result = tr("Unknown");
        break;
    }

    return result;
}


QStringList PlayerBuildInfo::targetNames(bool all)
{
    QStringList result;
    playerInfo.beginGroup("platforms");
    QStringList a = playerInfo.childKeys();
    playerInfo.endGroup();
    for(int i = 0; i < a.size(); i++)
    {
        QString target = playerInfo.value("platforms/" + a.at(i), "null").toString();
        if(playerInfo.value(target + "/status").toString() != "disabled" || all) {
            result.append(target);
        }
    }
    return result;
}

