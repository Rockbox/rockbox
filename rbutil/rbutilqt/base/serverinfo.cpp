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

#include "serverinfo.h"
#include "rbsettings.h"
#include "systeminfo.h"

#if defined(Q_OS_LINUX)
#include <unistd.h>
#endif

// server infos
const static struct {
    ServerInfo::ServerInfos info;
    const char* name;
    const char* def;
} ServerInfoList[] = {
    { ServerInfo::CurReleaseVersion,    ":platform:/releaseversion", "" },
    { ServerInfo::CurStatus,            ":platform:/status",         "Unknown" },
    { ServerInfo::DailyRevision,        "dailyrev",                  "" },
    { ServerInfo::DailyDate,            "dailydate",                 "" },
    { ServerInfo::BleedingRevision,     "bleedingrev",               "" },
    { ServerInfo::BleedingDate,         "bleedingdate",              "" },
};  

QMap<QString, QVariant> ServerInfo::serverInfos;

void ServerInfo::readBuildInfo(QString file)
{
    QSettings info(file, QSettings::IniFormat);
    
    setValue(ServerInfo::DailyRevision,info.value("dailies/rev"));
    QDate date = QDate::fromString(info.value("dailies/date").toString(), "yyyyMMdd");
    setValue(ServerInfo::DailyDate,date.toString(Qt::ISODate));
   
    info.beginGroup("release");
    QStringList keys = info.allKeys();
    for(int i=0; i < keys.size(); i++)
    {
        setAllConfigPlatformValue(keys[i],ServerInfo::CurReleaseVersion,info.value(keys[i]));
    }
    info.endGroup();

    info.beginGroup("status");
    keys = info.allKeys();
    for(int i=0; i < keys.size(); i++)
    {
        switch(info.value(keys[i]).toInt())
        {
            case 1:
                ServerInfo::setAllConfigPlatformValue(keys[i],ServerInfo::CurStatus,tr("Unusable"));
                break;
            case 2:
                ServerInfo::setAllConfigPlatformValue(keys[i],ServerInfo::CurStatus,tr("Unstable"));
                break;
            case 3:
                ServerInfo::setAllConfigPlatformValue(keys[i],ServerInfo::CurStatus,tr("Stable"));
                break;
            default:    
                ServerInfo::setAllConfigPlatformValue(keys[i],ServerInfo::CurStatus,tr("Unknown"));
                break;
        }
    }
    info.endGroup();
}

void ServerInfo::readBleedingInfo(QString file)
{
    QSettings info(file, QSettings::IniFormat);
    
    setValue(ServerInfo::BleedingRevision,info.value("bleeding/rev"));
    QDateTime date = QDateTime::fromString(info.value("bleeding/timestamp").toString(), "yyyyMMddThhmmssZ");
    setValue(ServerInfo::BleedingDate,date.toString(Qt::ISODate));
}
        
QVariant ServerInfo::value(enum ServerInfos info)
{
    // locate info item
    int i = 0;
    while(ServerInfoList[i].info != info)
        i++;

    QString s = ServerInfoList[i].name;
    s.replace(":platform:", RbSettings::value(RbSettings::CurrentPlatform).toString());
    qDebug() << "[ServerIndo] GET:" << s << serverInfos.value(s, ServerInfoList[i].def).toString();
    return serverInfos.value(s, ServerInfoList[i].def);
}

void ServerInfo::setValue(enum ServerInfos setting, QVariant value)
{
   QString empty;
   return setPlatformValue(empty, setting, value);
}

void ServerInfo::setPlatformValue(QString platform, enum ServerInfos info, QVariant value)
{
    // locate setting item
    int i = 0;
    while(ServerInfoList[i].info != info)
        i++;

    QString s = ServerInfoList[i].name;
    s.replace(":platform:", platform);
    serverInfos.insert(s, value);
    qDebug() << "[ServerInfo] SET:" << s << serverInfos.value(s).toString();
}

void ServerInfo::setAllConfigPlatformValue(QString configplatform, ServerInfos info, QVariant value)
{
    // insert intp all platforms where configurename matches
    QStringList platforms = SystemInfo::platforms();
    for(int i =0; i < platforms.size(); i++)
    {
        if(SystemInfo::platformValue(platforms.at(i),SystemInfo::CurConfigureModel) == configplatform)
            setPlatformValue(platforms.at(i),info,value);
    }
} 

QVariant ServerInfo::platformValue(QString platform, enum ServerInfos info)
{
    // locate setting item
    int i = 0;
    while(ServerInfoList[i].info != info)
        i++;

    QString s = ServerInfoList[i].name;
    s.replace(":platform:", platform);
    QString d = ServerInfoList[i].def;
    d.replace(":platform:", platform);
    qDebug() << "[ServerInfo] GET" << s << serverInfos.value(s, d).toString();
    return serverInfos.value(s, d);
}

