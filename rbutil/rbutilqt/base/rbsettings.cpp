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
    { RbSettings::RbutilUrl,            "rbutil_url",           "" },
    { RbSettings::BleedingInfo,         "bleeding_info",        "" },
    { RbSettings::CurPlatformName,      ":platform:/name",      "" },
    { RbSettings::CurManual,            ":platform:/manualname","rockbox-:platform:" },
    { RbSettings::CurBootloaderMethod,  ":platform:/bootloadermethod", "none" },
    { RbSettings::CurBootloaderName,    ":platform:/bootloadername", "" },
    { RbSettings::CurBootloaderFile,    ":platform:/bootloaderfile", "" },
    { RbSettings::CurEncoder,           ":platform:/encoder",   "" },
    { RbSettings::CurBrand,             ":platform:/brand",     "" },
    { RbSettings::CurName,              ":platform:/name",      "" },
    { RbSettings::CurBuildserverModel,  ":platform:/buildserver_modelname", "" },
    { RbSettings::CurConfigureModel,    ":platform:/configure_modelname", "" },
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

// server settings
const static struct {
    RbSettings::ServerSettings setting;
    const char* name;
    const char* def;
} ServerSettingsList[] = {
    { RbSettings::CurReleaseVersion,    ":platform:/releaseversion", "" },
    { RbSettings::CurStatus,            ":platform:/status", "" },
    { RbSettings::DailyRevision,        "dailyrev",             "" },
    { RbSettings::DailyDate,            "dailydate",            "" },
    { RbSettings::BleedingRevision,     "bleedingrev",          "" },
    { RbSettings::BleedingDate,         "bleedingdate",         "" },
};  

//! pointer to setting object to NULL
QSettings* RbSettings::systemSettings = NULL;
QSettings* RbSettings::userSettings = NULL;
//! global volatile settings
QMap<QString, QVariant> RbSettings::serverSettings;

void RbSettings::ensureRbSettingsExists()
{
    //check and create settings object
    if(systemSettings == NULL)
    {
        // only use built-in rbutil.ini
        systemSettings = new QSettings(":/ini/rbutil.ini", QSettings::IniFormat, 0);
    }
    
    if(userSettings == NULL)
    {
        // portable installation:
        // check for a configuration file in the program folder.
        QFileInfo config;
        config.setFile(QCoreApplication::instance()->applicationDirPath()
            + "/RockboxUtility.ini");
        if(config.isFile())
        {
            userSettings = new QSettings(QCoreApplication::instance()->applicationDirPath()
                + "/RockboxUtility.ini", QSettings::IniFormat, NULL);
            qDebug() << "[Settings] configuration: portable";
        }
        else
        {
            userSettings = new QSettings(QSettings::IniFormat,
            QSettings::UserScope, "rockbox.org", "RockboxUtility",NULL);
            qDebug() << "[Settings] configuration: system";
        }
    }
}

void RbSettings::sync()
{
    ensureRbSettingsExists();

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
            if(chown(qPrintable(userSettings->fileName()), realuid, realgid))
            { }
        }
    }
#endif
}

QString RbSettings::userSettingFilename()
{
    ensureRbSettingsExists();
    return userSettings->fileName();
}

QVariant RbSettings::value(enum SystemSettings setting)
{
    ensureRbSettingsExists();

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

QVariant RbSettings::value(enum UserSettings setting)
{
    QString empty;
    return subValue(empty, setting);
}

QVariant RbSettings::subValue(QString sub, enum UserSettings setting)
{
    ensureRbSettingsExists();

    // locate setting item
    int i = 0;
    while(UserSettingsList[i].setting != setting)
        i++;

    QString s = constructSettingPath(UserSettingsList[i].name, sub);
    qDebug() << "[Settings] GET U:" << s << userSettings->value(s).toString();
    return userSettings->value(s, UserSettingsList[i].def);
}

QVariant RbSettings::value(enum ServerSettings setting)
{
    ensureRbSettingsExists();

    // locate setting item
    int i = 0;
    while(ServerSettingsList[i].setting != setting)
        i++;

    QString s = constructSettingPath(ServerSettingsList[i].name);
    qDebug() << "[Settings] GET SERV:" << s
             << serverSettings.value(s, ServerSettingsList[i].def).toString();
    if(serverSettings.contains(s))
        return serverSettings.value(s);
    else
        return ServerSettingsList[i].def;
}

void RbSettings::setValue(enum UserSettings setting , QVariant value)
{
   QString empty;
   return setSubValue(empty, setting, value);
}

void RbSettings::setSubValue(QString sub, enum UserSettings setting, QVariant value)
{
    ensureRbSettingsExists();

    // locate setting item
    int i = 0;
    while(UserSettingsList[i].setting != setting)
        i++;

    QString s = constructSettingPath(UserSettingsList[i].name, sub);
    userSettings->setValue(s, value);
    qDebug() << "[Settings] SET U:" << s << userSettings->value(s).toString();
}

void RbSettings::setValue(enum ServerSettings setting, QVariant value)
{
    QString empty;
    return setPlatformValue(empty, setting, value);
}

void RbSettings::setPlatformValue(QString platform, enum ServerSettings setting, QVariant value)
{
    ensureRbSettingsExists();

    // locate setting item
    int i = 0;
    while(ServerSettingsList[i].setting != setting)
        i++;

    QString s = ServerSettingsList[i].name;
    s.replace(":platform:", platform);
    serverSettings.insert(s, value);
    qDebug() << "[Settings] SET SERV:" << s << serverSettings.value(s).toString();
}


QVariant RbSettings::platformValue(QString platform, enum SystemSettings setting)
{
    ensureRbSettingsExists();

    // locate setting item
    int i = 0;
    while(SystemSettingsList[i].setting != setting)
        i++;

    QString s = SystemSettingsList[i].name;
    s.replace(":platform:", platform);
    QString d = SystemSettingsList[i].def;
    d.replace(":platform:", platform);
    qDebug() << "[Settings] GET P:" << s << systemSettings->value(s, d).toString();
    return systemSettings->value(s, d);
}


QStringList RbSettings::platforms()
{
    ensureRbSettingsExists();

    QStringList result;
    systemSettings->beginGroup("platforms");
    QStringList a = systemSettings->childKeys();
    systemSettings->endGroup();
    for(int i = 0; i < a.size(); i++)
    {
        //only add not disabled targets
        QString target = systemSettings->value("platforms/"+a.at(i), "null").toString();
        if(systemSettings->value(target+"/status").toString() != "disabled")
            result.append(target);
    }
    return result;
}

QStringList RbSettings::languages()
{
    ensureRbSettingsExists();

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
    ensureRbSettingsExists();
    return systemSettings->value(platform + "/name").toString();
}

QString RbSettings::brand(QString platform)
{
    ensureRbSettingsExists();
    return systemSettings->value(platform + "/brand").toString();
}

QMap<int, QString> RbSettings::usbIdMap(enum MapType type)
{
    ensureRbSettingsExists();

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

