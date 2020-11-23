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

#include "serverinfo.h"
#include "rbsettings.h"
#include "systeminfo.h"
#include "Logger.h"

ServerInfo* ServerInfo::infoInstance = nullptr;

ServerInfo* ServerInfo::instance()
{
    if (infoInstance == nullptr) {
        infoInstance = new ServerInfo();
    }
    return infoInstance;
}

// server infos
const static struct {
    ServerInfo::ServerInfos info;
    const char* name;
    const char* def;
} ServerInfoList[] = {
    { ServerInfo::CurReleaseVersion,    "release/:platform:",           "" },
    { ServerInfo::CurReleaseUrl,        "release/:platform:",           "" },
    { ServerInfo::RelCandidateVersion,  "release-candidate/:platform:", "" },
    { ServerInfo::RelCandidateUrl,      "release-candidate/:platform:", "" },
    { ServerInfo::DailyVersion,         "daily/:platform:",             "" },
    { ServerInfo::DailyUrl,             "daily/:platform:",             "" },
    { ServerInfo::CurStatus,            "status/:platform:",            "-1" },
    { ServerInfo::BleedingRevision,     "bleeding/rev",                 "" },
    { ServerInfo::BleedingDate,         "bleeding/timestamp",           "" },
    { ServerInfo::CurDevelUrl,          "",                             "" },
};

void ServerInfo::readBuildInfo(QString file)
{
    if (serverSettings)
        delete serverSettings;
    serverSettings = new QSettings(file, QSettings::IniFormat);
}


QVariant ServerInfo::platformValue(enum ServerInfos info, QString platform)
{
    // locate setting item in server info file
    int i = 0;
    while(ServerInfoList[i].info != info)
        i++;

    // replace setting name
    if(platform.isEmpty())
        platform = RbSettings::value(RbSettings::CurrentPlatform).toString();

    // split of variant for platform.
    // we can have an optional variant part in the platform string.
    // For build info we don't use that.
    platform = platform.split('.').at(0);

    QString s = ServerInfoList[i].name;
    s.replace(":platform:", platform);

    // get value
    QVariant value = QString();
    if(!s.isEmpty() && serverSettings)
        value = serverSettings->value(s, ServerInfoList[i].def);

    // depending on the actual value we need more replacements.
    switch(info) {
    case CurReleaseVersion:
    case RelCandidateVersion:
    case DailyVersion:
        value = value.toStringList().at(0);
        break;
    case CurReleaseUrl:
    case RelCandidateUrl:
    case DailyUrl:
        {
            QString version = value.toStringList().at(0);
            if(value.toStringList().size() > 1)
                value = value.toStringList().at(1);
            else if(!version.isEmpty() && info == CurReleaseUrl)
                value = SystemInfo::value(SystemInfo::BuildUrl,
                                          SystemInfo::BuildRelease).toString()
                    .replace("%MODEL%", platform)
                    .replace("%RELVERSION%", version);
            else if(!version.isEmpty() && info == RelCandidateUrl)
                value = SystemInfo::value(SystemInfo::BuildUrl,
                                          SystemInfo::BuildCandidate).toString()
                    .replace("%MODEL%", platform)
                    .replace("%RELVERSION%", version);
            else if(!version.isEmpty() && info == DailyUrl)
                value = SystemInfo::value(SystemInfo::BuildUrl,
                                          SystemInfo::BuildDaily).toString()
                    .replace("%MODEL%", platform)
                    .replace("%RELVERSION%", version);
        }
        break;
    case CurDevelUrl:
        value = SystemInfo::value(SystemInfo::BuildUrl,
                                  SystemInfo::BuildCurrent).toString()
                .replace("%MODEL%", platform);
        break;
    case BleedingDate:
        // TODO: get rid of this, it's location specific.
        value = QDateTime::fromString(value.toString(),
                                      "yyyyMMddThhmmssZ").toString(Qt::ISODate);
        break;

    default:
        break;
    }

    LOG_INFO() << "Server:" << value;
    return value;
}

QString ServerInfo::statusAsString(QString platform)
{
    QString value;
    switch(platformValue(CurStatus, platform).toInt())
    {
    case STATUS_RETIRED:
        value = tr("Stable (Retired)");
        break;
    case STATUS_UNUSABLE:
        value = tr("Unusable");
        break;
    case STATUS_UNSTABLE:
        value = tr("Unstable");
        break;
    case STATUS_STABLE:
        value = tr("Stable");
        break;
    default:
        value = tr("Unknown");
        break;
    }

    return value;
}
