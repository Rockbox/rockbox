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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef RBSETTINGS_H
#define RBSETTINGS_H

#include <QtCore>

class QSettings;

class RbSettings : public QObject
{
    Q_OBJECT

    public:
        RbSettings() {}

        //! open the settings files
        void open();
        //! call this to flush the user Settings
        void sync();

        // returns the filename of the usersettings file
        QString userSettingFilename();

        //! access functions for the settings
        QString curVersion();
        bool cacheOffline();
        bool cacheDisabled();
        QString mountpoint();
        QString manualUrl();
        QString bleedingUrl();
        QString lastRelease(QString platform);
        QString cachePath();
        QString build(void);
        QString bootloaderUrl();
        QString bootloaderInfoUrl();
        QString fontUrl();
        QString voiceUrl();
        QString doomUrl();
        QString releaseUrl();
        QString dailyUrl();
        QString serverConfUrl();
        QString themeUrl();
        QString genlangUrl();
        QString proxyType();
        QString proxy();
        QString bleedingInfo();
        QString ofPath();
        QString lastTalkedFolder();
        QString voiceLanguage();
        int wavtrimTh();
        QString ttsPath(QString tts);
        QString ttsOptions(QString tts);
        QString ttsVoice(QString tts);
        int ttsSpeed(QString tts);
        QString ttsLang(QString tts);
        bool ttsUseSapi4();
        QString encoderPath(QString enc);
        QString encoderOptions(QString enc);
        double encoderQuality(QString enc);
        int encoderComplexity(QString enc);
        double encoderVolume(QString enc);
        bool encoderNarrowband(QString enc);
        
        QStringList allPlatforms();
        QString name(QString plattform);
        QString brand(QString plattform);
        QStringList allLanguages();
        QString nameOfTargetId(int id);
        QMap<int, QString> usbIdMap();
        QMap<int, QString> usbIdErrorMap();
        QMap<int, QString> usbIdIncompatMap();
        
        bool curNeedsBootloader();
        QString curBrand();
        QString curName();
        QString curPlatform();
        QString curPlatformName();
        QString curManual();
        bool curReleased();
        QString curBootloaderMethod();
        QString curBootloaderName();
        QString curVoiceName();
        QString curLang();
        QString curEncoder();
        QString curTTS();
        QString curResolution();
        QString curBootloaderFile();
        int curTargetId();

        //! Set Functions
        void setCurVersion(QString version);
        void setOfPath(QString path);
        void setCachePath(QString path);
        void setBuild(QString build);
        void setLastTalkedDir(QString dir);
        void setVoiceLanguage(QString lang);
        void setWavtrimTh(int th);
        void setProxy(QString proxy);
        void setProxyType(QString proxytype);
        void setLang(QString lang);
        void setMountpoint(QString mp);
        void setCurPlatform(QString platt);
        void setCacheDisable(bool on);
        void setCacheOffline(bool on);
        void setCurTTS(QString tts);
        void setTTSPath(QString tts, QString path);
        void setTTSOptions(QString tts, QString options);
        void setTTSSpeed(QString tts, int speed);
        void setTTSVoice(QString tts, QString voice);
        void setTTSLang(QString tts, QString lang);
        void setTTSUseSapi4(bool value);
        void setEncoderPath(QString enc, QString path);
        void setEncoderOptions(QString enc, QString options);
        void setEncoderQuality(QString enc, double q);
        void setEncoderComplexity(QString enc, int c);
        void setEncoderVolume(QString enc,double v);
        void setEncoderNarrowband(QString enc,bool nb);

    private:
        
        //! helper function to get an entry in the current platform section
        QVariant deviceSettingCurGet(QString entry,QString def="");
        //! helper function to get an entry out of a group in the userSettings
        QVariant userSettingsGroupGet(QString group,QString entry,QVariant def="");
        //! helper function to set an entry in a group in the userSettings
        void userSettingsGroupSet(QString group,QString entry,QVariant value);
        
        
        //! private copy constructors to prvent copying
        RbSettings&  operator= (const RbSettings& other)
            { (void)other; return *this; }
        RbSettings(const RbSettings& other) :QObject()
            { (void)other; }
    
        //! pointers to our setting objects
        QSettings *devices;
        QSettings *userSettings;

};

#endif
