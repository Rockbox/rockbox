/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: rbsettings.cpp 16150 2008-01-23 21:54:40Z domonoky $
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

void RbSettings::open()
{
    // only use built-in rbutil.ini
    devices = new QSettings(":/ini/rbutil.ini", QSettings::IniFormat, 0);
     // portable installation:
    // check for a configuration file in the program folder.
    QFileInfo config;
    config.setFile(QCoreApplication::instance()->applicationDirPath() + "/RockboxUtility.ini");
    if(config.isFile()) 
    {
        userSettings = new QSettings(QCoreApplication::instance()->applicationDirPath() + "/RockboxUtility.ini",
            QSettings::IniFormat, this);
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
} 

QString RbSettings::userSettingFilename()
{
    return userSettings->fileName();
}

bool RbSettings::cacheOffline()
{
    return userSettings->value("offline").toBool();
}

bool RbSettings::curNeedsBootloader()
{
    QString platform = userSettings->value("platform").toString();
    devices->beginGroup(platform);
    QString result = devices->value("needsbootloader", "").toString(); 
    devices->endGroup();
    if( result == "no")
    {
        return false;
    }
    else
        return true;
}

QString RbSettings::mountpoint()
{
    return userSettings->value("mountpoint").toString();
}

QString RbSettings::manualUrl()
{
    return devices->value("manual_url").toString();
}

QString RbSettings::bleedingUrl()
{
    return devices->value("bleeding_url").toString();
}


QString RbSettings::lastRelease()
{
    return devices->value("last_release").toString();
}

QString RbSettings::cachePath()
{
    return userSettings->value("cachepath", QDir::tempPath()).toString();
}

QString RbSettings::bootloaderUrl()
{
    return devices->value("bootloader_url").toString();
}

QString RbSettings::bootloaderInfoUrl()
{
    return devices->value("bootloader_info_url").toString();
}

QString RbSettings::fontUrl()
{
    return devices->value("font_url").toString();
}

QString RbSettings::voiceUrl()
{
    return devices->value("voice_url").toString();
}

QString RbSettings::doomUrl()
{
    return devices->value("doom_url").toString();
}

QString RbSettings::downloadUrl()
{
    return devices->value("download_url").toString();
}

QString RbSettings::dailyUrl()
{
    return devices->value("daily_url").toString();
}

QString RbSettings::serverConfUrl()
{
    return devices->value("server_conf_url").toString();
}

QString RbSettings::genlangUrl()
{
    return devices->value("genlang_url").toString();
}

QString RbSettings::themeUrl()
{
    return devices->value("themes_url").toString();
}

QString RbSettings::bleedingInfo()
{
    return devices->value("bleeding_info").toString();
}

bool RbSettings::cacheDisabled()
{
    return userSettings->value("cachedisable").toBool();
}

QString RbSettings::proxyType()
{
    return userSettings->value("proxytype", "none").toString();
}

QString RbSettings::proxy()
{
    return userSettings->value("proxy").toString();
}

QString RbSettings::ofPath()
{
    return userSettings->value("ofpath").toString();
}

QString RbSettings::curBrand()
{
    QString platform = userSettings->value("platform").toString();
    return brand(platform);
}

QString RbSettings::curName()
{
    QString platform = userSettings->value("platform").toString();
    return name(platform);
}

QString RbSettings::curPlatform()
{
    return userSettings->value("platform").toString();
}

QString RbSettings::curPlatformName()
{
    QString platform = userSettings->value("platform").toString();
    devices->beginGroup(platform);
    QString name = devices->value("platform").toString();
    devices->endGroup();
    return name;
}

QString RbSettings::curManual()
{
    QString platform = userSettings->value("platform").toString();
    devices->beginGroup(platform);
    QString name = devices->value("manualname","rockbox-" +
            devices->value("platform").toString()).toString();
    devices->endGroup();
    return name;
}

bool RbSettings::curReleased()
{
    QString platform = userSettings->value("platform").toString();
    devices->beginGroup(platform);
    QString released = devices->value("released").toString();
    devices->endGroup();
    
    if(released == "yes")
        return true;
    else 
        return false;
}

QString RbSettings::curBootloaderMethod()
{
    QString platform = userSettings->value("platform").toString();
    devices->beginGroup(platform);
    QString method = devices->value("bootloadermethod").toString();
    devices->endGroup();
    return method;
}

QString RbSettings::curBootloaderName()
{
    QString platform = userSettings->value("platform").toString();
    devices->beginGroup(platform);
    QString name = devices->value("bootloadername").toString();
    devices->endGroup();
    return name;
}

QString RbSettings::curVoiceName()
{
    QString platform = userSettings->value("platform").toString();
    devices->beginGroup(platform);
    QString name = devices->value("voicename").toString();
    devices->endGroup();
    return name;
}

QString RbSettings::curLang()
{
    return userSettings->value("lang").toString();
}

QString RbSettings::curEncoder()
{
    return userSettings->value("encoder").toString();
}

QString RbSettings::curTTS()
{
    return userSettings->value("tts").toString();
}

QString RbSettings::lastTalkedFolder()
{
    return userSettings->value("last_talked_folder").toString();
}
 
QString RbSettings::voiceLanguage()
{
    return userSettings->value("voicelanguage").toString();
}
 
int RbSettings::wavtrimTh()
{
    return userSettings->value("wavtrimthreshold",500).toInt();
}

QString RbSettings::ttsPath(QString tts)
{
    userSettings->beginGroup(tts);
    QString path = userSettings->value("ttspath").toString();
    userSettings->endGroup();
    return path;
    
}
QString RbSettings::ttsOptions(QString tts)
{
    userSettings->beginGroup(tts);
    QString op = userSettings->value("ttsoptions").toString();
    userSettings->endGroup();
    return op;
}
QString RbSettings::ttsVoice(QString tts)
{
    userSettings->beginGroup(tts);
    QString op = userSettings->value("ttsvoice","Microsoft Sam").toString();
    userSettings->endGroup();
    return op;
}
int RbSettings::ttsSpeed(QString tts)
{
    userSettings->beginGroup(tts);
    int sp = userSettings->value("ttsspeed",0).toInt();
    userSettings->endGroup();
    return sp;
}
QString RbSettings::ttsLang(QString tts)
{
    userSettings->beginGroup(tts);
    QString op = userSettings->value("ttslanguage","english").toString();
    userSettings->endGroup();
    return op;
}

bool RbSettings::ttsUseSapi4()
{
    userSettings->beginGroup("sapi");
    bool op = userSettings->value("useSapi4",false).toBool();
    userSettings->endGroup();
    return op;
}

QString RbSettings::encoderPath(QString enc)
{
    userSettings->beginGroup(enc);
    QString path = userSettings->value("encoderpath").toString();
    userSettings->endGroup();
    return path;
}
QString RbSettings::encoderOptions(QString enc)
{
    userSettings->beginGroup(enc);
    QString op = userSettings->value("encoderoptions").toString();
    userSettings->endGroup();
    return op;
}

double RbSettings::encoderQuality(QString enc)
{
    userSettings->beginGroup(enc);
    double q =userSettings->value("quality",8.f).toDouble();
    userSettings->endGroup();
    return q;
}
int RbSettings::encoderComplexity(QString enc)
{
    userSettings->beginGroup(enc);
    int c = userSettings->value("complexity",10).toInt();
    userSettings->endGroup();
    return c;
}
double RbSettings::encoderVolume(QString enc)
{
    userSettings->beginGroup(enc);
    double v = userSettings->value("volume",1.f).toDouble();
    userSettings->endGroup();
    return v;
}
bool RbSettings::encoderNarrowband(QString enc)
{
    userSettings->beginGroup(enc);
    bool nb = userSettings->value("narrowband",false).toBool();
    userSettings->endGroup();
    return nb;
}
 
QStringList RbSettings::allPlatforms()
{
    QStringList result;
    devices->beginGroup("platforms");
    QStringList a = devices->childKeys();
    for(int i = 0; i < a.size(); i++) 
    {
        result.append(devices->value(a.at(i), "null").toString());
    }
    devices->endGroup();
    return result;
}

QStringList RbSettings::allLanguages()
{
    QStringList result;
    devices->beginGroup("languages");
    QStringList a = devices->childKeys();
    for(int i = 0; i < a.size(); i++) 
    {
        result.append(devices->value(a.at(i), "null").toString());
    }
    devices->endGroup();
    return result;
}

QString RbSettings::name(QString plattform)
{
    devices->beginGroup(plattform);
    QString name = devices->value("name").toString();
    devices->endGroup();
    return name;
}

QString RbSettings::brand(QString plattform)
{
    devices->beginGroup(plattform);
    QString brand = devices->value("brand").toString();
    devices->endGroup();
    return brand;
}

QMap<int, QString> RbSettings::usbIdMap()
{
    QMap<int, QString> map;
     // get a list of ID -> target name
    QStringList platforms;
    devices->beginGroup("platforms");
    platforms = devices->childKeys();
    devices->endGroup();
    
    for(int i = 0; i < platforms.size(); i++)
    {
        devices->beginGroup("platforms");
        QString target = devices->value(platforms.at(i)).toString();
        devices->endGroup();
        devices->beginGroup(target);
        QStringList ids = devices->value("usbid").toStringList();
        int j = ids.size();
        while(j--)
            map.insert(ids.at(j).toInt(0, 16), target);

        devices->endGroup();
    }
    return map;
}

QMap<int, QString> RbSettings::usbIdErrorMap()
{

    QMap<int, QString> map;
     // get a list of ID -> target name
    QStringList platforms;
    devices->beginGroup("platforms");
    platforms = devices->childKeys();
    devices->endGroup();
    
    for(int i = 0; i < platforms.size(); i++)
    {
        devices->beginGroup("platforms");
        QString target = devices->value(platforms.at(i)).toString();
        devices->endGroup();
        devices->beginGroup(target);
        QStringList ids = devices->value("usberror").toStringList();
        int j = ids.size();
        while(j--)
            map.insert(ids.at(j).toInt(0, 16), target);
        devices->endGroup();
    }
    return map;
}


QMap<int, QString> RbSettings::usbIdIncompatMap()
{

    QMap<int, QString> map;
     // get a list of ID -> target name
    QStringList platforms;
    devices->beginGroup("platforms");
    platforms = devices->childKeys();
    devices->endGroup();
    
    for(int i = 0; i < platforms.size(); i++)
    {
        devices->beginGroup("platforms");
        QString target = devices->value(platforms.at(i)).toString();
        devices->endGroup();
        devices->beginGroup(target);
        QStringList ids = devices->value("usbincompat").toStringList();
        int j = ids.size();
        while(j--)
            map.insert(ids.at(j).toInt(0, 16), target);
        devices->endGroup();
    }
    return map;
}


QString RbSettings::curResolution()
{
    QString platform = userSettings->value("platform").toString();
    devices->beginGroup(platform);
    QString resolution = devices->value("resolution").toString();
    devices->endGroup();
    return resolution;
}

int RbSettings::curTargetId()
{
    QString platform = userSettings->value("platform").toString();
    devices->beginGroup(platform);
    int id = devices->value("targetid").toInt();
    devices->endGroup();
    return id;
}


void RbSettings::setOfPath(QString path)
{
    userSettings->setValue("ofpath",path);
}

void RbSettings::setCachePath(QString path)
{
    userSettings->setValue("cachepath", path);
}

void RbSettings::setBuild(QString build)
{
    userSettings->setValue("build", build);
}

void RbSettings::setLastTalkedDir(QString dir)
{
    userSettings->setValue("last_talked_folder", dir);
}

void RbSettings::setVoiceLanguage(QString dir)
{
    userSettings->setValue("voicelanguage", dir);
}

void RbSettings::setWavtrimTh(int th)
{
    userSettings->setValue("wavtrimthreshold", th);
}

void RbSettings::setProxy(QString proxy)
{
    userSettings->setValue("proxy", proxy);
}

void RbSettings::setProxyType(QString proxytype)
{
    userSettings->setValue("proxytype", proxytype);
}

void RbSettings::setLang(QString lang)
{
    userSettings->setValue("lang", lang);
}

void RbSettings::setMountpoint(QString mp)
{
    userSettings->setValue("mountpoint",mp);
}

void RbSettings::setCurPlatform(QString platt)
{
    userSettings->setValue("platform",platt);
}


void RbSettings::setCacheDisable(bool on)
{
    userSettings->setValue("cachedisable",on);
}

void RbSettings::setCacheOffline(bool on)
{
    userSettings->setValue("offline",on);
}

void RbSettings::setCurTTS(QString tts)
{
    userSettings->setValue("tts",tts);
}

void RbSettings::setCurEncoder(QString enc)
{
    userSettings->setValue("encoder",enc);
}

void RbSettings::setTTSPath(QString tts, QString path)
{
    userSettings->beginGroup(tts);
    userSettings->setValue("ttspath",path);
    userSettings->endGroup();
}

void RbSettings::setTTSOptions(QString tts, QString options)
{
    userSettings->beginGroup(tts);
    userSettings->setValue("ttsoptions",options);
    userSettings->endGroup();
}

void RbSettings::setTTSVoice(QString tts, QString voice)
{
    userSettings->beginGroup(tts);
    userSettings->setValue("ttsvoice",voice);
    userSettings->endGroup();
}

void RbSettings::setTTSSpeed(QString tts, int speed)
{
    userSettings->beginGroup(tts);
    userSettings->setValue("ttsspeed",speed);
    userSettings->endGroup();
}

void RbSettings::setTTSLang(QString tts, QString lang)
{
    userSettings->beginGroup(tts);
    userSettings->setValue("ttslanguage",lang);
    userSettings->endGroup();
}

void RbSettings::setTTSUseSapi4(bool value)
{
    userSettings->beginGroup("sapi");
    userSettings->setValue("useSapi4",value);
    userSettings->endGroup();
} 


void RbSettings::setEncoderPath(QString enc, QString path)
{
    userSettings->beginGroup(enc);
    userSettings->setValue("encoderpath",path);
    userSettings->endGroup();
}

void RbSettings::setEncoderOptions(QString enc, QString options)
{
    userSettings->beginGroup(enc);
    userSettings->setValue("encoderoptions",options);
    userSettings->endGroup();
}

void RbSettings::setEncoderQuality(QString enc, double q)
{
    userSettings->beginGroup(enc);
    userSettings->setValue("quality",q);
    userSettings->endGroup();
}
void RbSettings::setEncoderComplexity(QString enc, int c)
{
    userSettings->beginGroup(enc);
    userSettings->setValue("complexity",c);
    userSettings->endGroup();
}
void RbSettings::setEncoderVolume(QString enc,double v)
{
     userSettings->beginGroup(enc);
    userSettings->setValue("volume",v);
    userSettings->endGroup();
}
void RbSettings::setEncoderNarrowband(QString enc,bool nb)
{
     userSettings->beginGroup(enc);
    userSettings->setValue("narrowband",nb);
    userSettings->endGroup();
}
