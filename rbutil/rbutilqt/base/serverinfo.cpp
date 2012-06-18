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
    { ServerInfo::CurReleaseUrl,        ":platform:/releaseurl",     "" },
    { ServerInfo::RelCandidateVersion,  ":platform:/rcversion",      "" },
    { ServerInfo::RelCandidateUrl,      ":platform:/rcurl",          "" },
    { ServerInfo::CurStatus,            ":platform:/status",         "Unknown" },
    { ServerInfo::ManualPdfUrl,         ":platform:/manual_pdf",     "" },
    { ServerInfo::ManualHtmlUrl,        ":platform:/manual_html",    "" },
    { ServerInfo::ManualZipUrl,         ":platform:/manual_zip",     "" },
    { ServerInfo::BleedingRevision,     "bleedingrev",               "" },
    { ServerInfo::BleedingDate,         "bleedingdate",              "" },
    { ServerInfo::CurDevelUrl,          ":platform:/develurl",       "" },
};

QMap<QString, QVariant> ServerInfo::serverInfos;

void ServerInfo::readBuildInfo(QString file)
{
    QString releaseBaseUrl = SystemInfo::value(SystemInfo::ReleaseUrl).toString();
    QString develBaseUrl = SystemInfo::value(SystemInfo::BleedingUrl).toString();
    QString manualBaseUrl = SystemInfo::value(SystemInfo::ManualUrl).toString();

    QSettings info(file, QSettings::IniFormat);

    QString developmentRevision = info.value("bleeding/rev").toString();
    setValue(ServerInfo::BleedingRevision, developmentRevision);
    QDateTime date = QDateTime::fromString(info.value("bleeding/timestamp").toString(), "yyyyMMddThhmmssZ");
    setValue(ServerInfo::BleedingDate, date.toString(Qt::ISODate));

    info.beginGroup("release");
    QStringList releasekeys = info.allKeys();
    info.endGroup();
    info.beginGroup("release-candidate");
    QStringList rckeys = info.allKeys();
    info.endGroup();

    // get base platforms, handle variants with platforms in the loop
    QStringList platforms = SystemInfo::platforms(SystemInfo::PlatformBaseDisabled);
    for(int i = 0; i < platforms.size(); i++)
    {
        // check if there are rbutil-variants of the current platform and handle
        // them the same time.
        QStringList variants;
        variants = SystemInfo::platforms(SystemInfo::PlatformVariantDisabled, platforms.at(i));
        QString releaseVersion;
        QString releaseUrl;
        QString relCandidateVersion;
        QString relCandidateUrl;
        // support two formats for "release" sections:
        // - <target>=<version>. In this case the URL is constructed.
        // - <target>=<version>,<url>.
        info.beginGroup("release");
        if(releasekeys.contains(platforms.at(i))) {
            QStringList entry = info.value(platforms.at(i)).toStringList();
            releaseVersion = entry.at(0);
            if(entry.size() > 1) {
                releaseUrl = entry.at(1);
            }
            else if(!releaseVersion.isEmpty()) {
                // construct release download URL
                releaseUrl = releaseBaseUrl;
                releaseUrl.replace("%MODEL%", platforms.at(i));
                releaseUrl.replace("%RELVERSION%", releaseVersion);
            }
        }
        info.endGroup();
        // "release-candidate" section currently only support the 2nd format.
        info.beginGroup("release-candidate");
        if(rckeys.contains(platforms.at(i))) {
            QStringList entry = info.value(platforms.at(i)).toStringList();
            if(entry.size() > 1) {
                relCandidateVersion = entry.at(0);
                relCandidateUrl = entry.at(1);
            }
        }
        info.endGroup();

        // "bleeding" section (development) does not provide individual
        // information but only a global revision value.
        QString develUrl = develBaseUrl;
        develUrl.replace("%MODEL%", platforms.at(i));
        develUrl.replace("%RELVERSION%", developmentRevision);

        info.beginGroup("status");
        QString status = tr("Unknown");
        switch(info.value(platforms.at(i)).toInt())
        {
            case 1:
                status = tr("Unusable");
                break;
            case 2:
                status = tr("Unstable");
                break;
            case 3:
                status = tr("Stable");
                break;
            default:
                break;
        }
        info.endGroup();

        // manual URLs
        QString manualPdfUrl = manualBaseUrl;
        QString manualHtmlUrl = manualBaseUrl;
        QString manualZipUrl = manualBaseUrl;

        QString buildservermodel = SystemInfo::platformValue(platforms.at(i),
                SystemInfo::CurBuildserverModel).toString();
        QString modelman = SystemInfo::platformValue(platforms.at(i),
                SystemInfo::CurManual).toString();
        QString manualBaseName = "rockbox-";

        if(modelman.isEmpty()) manualBaseName += buildservermodel;
        else manualBaseName += modelman;

        manualPdfUrl.replace("%EXTENSION%", "pdf");
        manualPdfUrl.replace("%MANUALBASENAME%", manualBaseName);
        manualHtmlUrl.replace("%EXTENSION%", "html");
        manualHtmlUrl.replace("%MANUALBASENAME%", manualBaseName + "/rockbox-build");
        manualZipUrl.replace("%EXTENSION%", "zip");
        manualZipUrl.replace("%MANUALBASENAME%", manualBaseName + "-html");

        // set variants (if any)
        for(int j = 0; j < variants.size(); ++j) {
            setPlatformValue(variants.at(j), ServerInfo::CurStatus, status);
            if(!releaseUrl.isEmpty()) {
                setPlatformValue(variants.at(j), ServerInfo::CurReleaseVersion, releaseVersion);
                setPlatformValue(variants.at(j), ServerInfo::CurReleaseUrl, releaseUrl);
            }
            if(!relCandidateUrl.isEmpty()) {
                setPlatformValue(variants.at(j), ServerInfo::RelCandidateVersion, relCandidateVersion);
                setPlatformValue(variants.at(j), ServerInfo::RelCandidateUrl, relCandidateUrl);
            }
            setPlatformValue(variants.at(j), ServerInfo::CurDevelUrl, develUrl);

            setPlatformValue(variants.at(j), ServerInfo::ManualPdfUrl, manualPdfUrl);
            setPlatformValue(variants.at(j), ServerInfo::ManualHtmlUrl, manualHtmlUrl);
            setPlatformValue(variants.at(j), ServerInfo::ManualZipUrl, manualZipUrl);
        }
    }
}


QVariant ServerInfo::value(enum ServerInfos info)
{
    // locate info item
    int i = 0;
    while(ServerInfoList[i].info != info)
        i++;

    QString s = ServerInfoList[i].name;
    s.replace(":platform:", RbSettings::value(RbSettings::CurrentPlatform).toString());
    qDebug() << "[ServerInfo] GET:" << s << serverInfos.value(s, ServerInfoList[i].def).toString();
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
    qDebug() << "[ServerInfo] GET:" << s << serverInfos.value(s, d).toString();
    return serverInfos.value(s, d);
}

