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

#include "rbsettings.h"

#include <QSettings>

#if defined(Q_OS_LINUX)
#include <unistd.h>
#endif

// device settings
const static struct {
    RbSettings::SystemSettings setting;
    const char* name;
    const char* def;
} SystemSettingsList[] = {
    { RbSettings::ManualUrl,            "manual_url",           "" },
    { RbSettings::BleedingUrl,          "bleeding_url",         "" },
    { RbSettings::BootloaderUrl,        "bootloader_url",       "" },
    { RbSettings::BootloaderInfoUrl,    "bootloader_info_url",  "" },
    { RbSettings::FontUrl,              "font_url",             "" },
    { RbSettings::VoiceUrl,             "voice_url",            "" },
    { RbSettings::DoomUrl,              "doom_url",             "" },
    { RbSettings::ReleaseUrl,           "release_url",          "" },
    { RbSettings::DailyUrl,             "daily_url",            "" },
    { RbSettings::ServerConfUrl,        "server_conf_url",      "" },
    { RbSettings::GenlangUrl,           "genlang_url",          "" },
    { RbSettings::ThemesUrl,            "themes_url",           "" },
    { RbSettings::BleedingInfo,         "bleeding_info",        "" },
    { RbSettings::CurPlatformName,      ":platform:/name",      "" },
    { RbSettings::CurManual,            ":platform:/manual",    "rockbox-:platform:" },
    { RbSettings::CurBootloaderMethod,  ":platform:/bootloadermethod", "none" },
    { RbSettings::CurBootloaderName,    ":platform:/bootloadername", "" },
    { RbSettings::CurBootloaderFile,    ":platform:/bootloaderfile", "" },
    { RbSettings::CurEncoder,           ":platform:/encoder",   "" },
    { RbSettings::CurResolution,        ":platform:/resolution", "" },
    { RbSettings::CurBrand,             ":platform:/brand",     "" },
    { RbSettings::CurName,              ":platform:/name",      "" },
    { RbSettings::CurBuildserverModel,  ":platform:/buildserver_modelname", "" },
    { RbSettings::CurConfigureModel,    ":platform:/configure_modelname", "" },
    { RbSettings::CurTargetId,          ":platform:/targetid",  "" },
};

// user settings
const static struct {
    RbSettings::UserSettings setting;
    const char* name;
    const char* def;
} UserSettingsList[] = {
    { RbSettings::RbutilVersion,        "rbutil_version",       "" },
    { RbSettings::CurrentPlatform,      "platform",             "" },
    { RbSettings::Mountpoint,           "mountpoint",           "" },
    { RbSettings::CachePath,            "cachepath",            "" },
    { RbSettings::Build,                "build",                "" },
    { RbSettings::ProxyType,            "proxytype",            "" },
    { RbSettings::Proxy,                "proxy",                "" },
    { RbSettings::OfPath,               "ofpath",               "" },
    { RbSettings::Platform,             "platform",             "" },
    { RbSettings::Language,             "lang",                 "" },
    { RbSettings::Tts,                  "tts",                  "" },
    { RbSettings::LastTalkedFolder,     "last_talked_folder",   "" },
    { RbSettings::VoiceLanguage,        "voicelanguage",        "" },
    { RbSettings::TtsLanguage,          ":tts:/language",       "" },
    { RbSettings::TtsOptions,           ":tts:/options",        "" },
    { RbSettings::TtsPath,              ":tts:/path",           "" },
    { RbSettings::TtsVoice,             ":tts:/voice",          "" },
    { RbSettings::EncoderPath,          ":encoder:/encoderpath",        "" },
    { RbSettings::EncoderOptions,       ":encoder:/encoderoptions",     "" },
    { RbSettings::CacheOffline,         "offline",              "false" },
    { RbSettings::CacheDisabled,        "cachedisable",         "false" },
    { RbSettings::TtsUseSapi4,          "sapi/useSapi4",        "false" },
    { RbSettings::EncoderNarrowBand,    ":encoder:/narrowband", "false" },
    { RbSettings::WavtrimThreshold,     "wavtrimthreshold",     "500"},
    { RbSettings::TtsSpeed,             ":tts:/speed",          "0" },
    { RbSettings::EncoderComplexity,    ":encoder:/complexity", "10" },
    { RbSettings::EncoderQuality,       ":encoder:/quality",    "8.0" },
    { RbSettings::EncoderVolume,        ":encoder:/volume",     "1.0" },
};

void RbSettings::open()
{
    // only use built-in rbutil.ini
    systemSettings = new QSettings(":/ini/rbutil.ini", QSettings::IniFormat, 0);
     // portable installation:
    // check for a configuration file in the program folder.
    QFileInfo config;
    config.setFile(QCoreApplication::instance()->applicationDirPath()
            + "/RockboxUtility.ini");
    if(config.isFile())
    {
        userSettings = new QSettings(QCoreApplication::instance()->applicationDirPath()
                + "/RockboxUtility.ini", QSettings::IniFormat, this);
        qDebug() << "config: portable";
    }
    else
    {
        userSettings = new QSettings(QSettings::IniFormat,
            QSettings::UserScope, "rockbox.org", "RockboxUtility",this);
        qDebug() << "config: system";
    }
}

void RbSettings::sync()
{
    userSettings->sync();
#if defined(Q_OS_LINUX)
    // when using sudo it runs rbutil with uid 0 but unfortunately without a
    // proper login shell, thus the configuration file gets placed in the
    // calling users $HOME. This in turn will cause issues if trying to
    // run rbutil later as user. Try to detect this case via the environment
    // variable SUDO_UID and SUDO_GID and if set chown the user config file.
    if(getuid() == 0)
    {
        char* realuser = getenv("SUDO_UID");
        char* realgroup = getenv("SUDO_GID");
        if(realuser != NULL && realgroup != NULL)
        {
            int realuid = atoi(realuser);
            int realgid = atoi(realgroup);
            // chown is attribute warn_unused_result, but in case this fails
            // we can't do anything useful about it. Notifying the user
            // is somewhat pointless. Add hack to suppress compiler warning.
            if(chown(qPrintable(userSettings->fileName()), realuid, realgid));
        }
    }
#endif
}


QVariant RbSettings::value(enum SystemSettings setting)
{
    // locate setting item
    int i = 0;
    while(SystemSettingsList[i].setting != setting)
        i++;

    QString s = constructSettingPath(SystemSettingsList[i].name);
    QString d = SystemSettingsList[i].def;
    d.replace(":platform:", userSettings->value("platform").toString());
    qDebug() << "[Settings] GET S:" << s << systemSettings->value(s, d).toString();
    return systemSettings->value(s, d);
}


QString RbSettings::userSettingFilename()
{
    return userSettings->fileName();
}


QVariant RbSettings::subValue(QString& sub, enum UserSettings setting)
{
    // locate setting item
    int i = 0;
    while(UserSettingsList[i].setting != setting)
        i++;

    QString s = constructSettingPath(UserSettingsList[i].name, sub);
    qDebug() << "[Settings] GET U:" << s << userSettings->value(s).toString();
    return userSettings->value(s, UserSettingsList[i].def);
}


void RbSettings::setSubValue(QString& sub, enum UserSettings setting, QVariant value)
{
    // locate setting item
    int i = 0;
    while(UserSettingsList[i].setting != setting)
        i++;

    QString s = constructSettingPath(UserSettingsList[i].name, sub);
    qDebug() << "[Settings] SET U:" << s << userSettings->value(s).toString();
    userSettings->setValue(s, value);
}



QStringList RbSettings::platforms()
{
    QStringList result;
    systemSettings->beginGroup("platforms");
    QStringList a = systemSettings->childKeys();
    for(int i = 0; i < a.size(); i++)
    {
        result.append(systemSettings->value(a.at(i), "null").toString());
    }
    systemSettings->endGroup();
    return result;
}

QStringList RbSettings::languages()
{
    QStringList result;
    systemSettings->beginGroup("languages");
    QStringList a = systemSettings->childKeys();
    for(int i = 0; i < a.size(); i++)
    {
        result.append(systemSettings->value(a.at(i), "null").toString());
    }
    systemSettings->endGroup();
    return result;
}

QString RbSettings::name(QString platform)
{
    return systemSettings->value(platform + "/name").toString();
}

QString RbSettings::brand(QString platform)
{
    return systemSettings->value(platform + "/brand").toString();
}

QMap<int, QString> RbSettings::usbIdMap(enum MapType type)
{
    QMap<int, QString> map;
    // get a list of ID -> target name
    QStringList platforms;
    systemSettings->beginGroup("platforms");
    platforms = systemSettings->childKeys();
    systemSettings->endGroup();

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
        systemSettings->beginGroup("platforms");
        QString target = systemSettings->value(platforms.at(i)).toString();
        systemSettings->endGroup();
        systemSettings->beginGroup(target);
        QStringList ids = systemSettings->value(t).toStringList();
        int j = ids.size();
        while(j--)
            map.insert(ids.at(j).toInt(0, 16), target);

        systemSettings->endGroup();
    }
    return map;
}


QString RbSettings::constructSettingPath(QString path, QString substitute)
{
    QString platform = userSettings->value("platform").toString();
    if(!substitute.isEmpty()) {
        path.replace(":tts:", substitute);
        path.replace(":encoder:", substitute);
    }
    else {
        path.replace(":tts:", userSettings->value("tts").toString());
        path.replace(":encoder:", systemSettings->value(platform + "/encoder").toString());
    }
    path.replace(":platform:", platform);

    return path;
}

