/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: rbsettings.h 16059 2008-01-11 23:59:12Z domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
        bool cacheOffline();
        bool cacheDisabled();
        QString mountpoint();
        QString manualUrl();
        QString bleedingUrl();
        QString lastRelease();
        QString cachePath();
        QString bootloaderUrl();
        QString bootloaderInfoUrl();
        QString fontUrl();
        QString voiceUrl();
        QString doomUrl();
        QString downloadUrl();
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
        int curTargetId();

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
        void setCurEncoder(QString enc);
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
        QSettings *devices;
        QSettings *userSettings;

};

#endif
