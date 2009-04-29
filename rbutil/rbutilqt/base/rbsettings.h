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

        enum MapType {
            MapDevice,
            MapError,
            MapIncompatible,
        };
        enum UserSettings {
            RbutilVersion,
            CurrentPlatform,
            Mountpoint,
            CachePath,
            Build,
            ProxyType,
            Proxy,
            OfPath,
            Platform,
            Language,
            Tts,
            LastTalkedFolder,
            VoiceLanguage,
            TtsLanguage,
            TtsOptions,
            TtsPath,
            TtsVoice,
            EncoderPath,
            EncoderOptions,
            WavtrimThreshold,
            EncoderComplexity,
            TtsSpeed,
            CacheOffline,
            CacheDisabled,
            TtsUseSapi4,
            EncoderNarrowBand,
            EncoderQuality,
            EncoderVolume,
        };
        enum SystemSettings {
            ManualUrl,
            BleedingUrl,
            BootloaderUrl,
            BootloaderInfoUrl,
            FontUrl,
            VoiceUrl,
            DoomUrl,
            ReleaseUrl,
            DailyUrl,
            ServerConfUrl,
            GenlangUrl,
            ThemesUrl,
            BleedingInfo,
            CurPlatformName,
            CurManual,
            CurBootloaderMethod,
            CurBootloaderName,
            CurBootloaderFile,
            CurEncoder,
            CurResolution,
            CurBrand,
            CurName,
            CurBuildserverModel,
            CurConfigureModel,
            CurTargetId,
        };

        QVariant value(enum SystemSettings setting);
        // generic and "current selection" values -- getters
        QVariant value(enum UserSettings setting)
            { QString empty; return subValue(empty, setting); }
        void setValue(enum UserSettings setting , QVariant value)
            { QString empty; return setSubValue(empty, setting, value); }

        QVariant subValue(QString& sub, enum UserSettings setting);
        QVariant subValue(const char* sub, enum UserSettings setting)
            { QString s = sub; return subValue(s, setting); }
        void setSubValue(QString& sub, enum UserSettings setting, QVariant value);
        void setSubValue(const char* sub, enum UserSettings setting, QVariant value)
            { QString s = sub; return setSubValue(s, setting, value); }

        QStringList platforms(void);
        QStringList languages(void);

        QString name(QString plattform);
        QString brand(QString plattform);

        QMap<int, QString> usbIdMap(enum MapType);

    private:
        //! private copy constructors to prvent copying
        RbSettings&  operator= (const RbSettings& other)
            { (void)other; return *this; }
        RbSettings(const RbSettings& other) :QObject()
            { (void)other; }
        QString constructSettingPath(QString path, QString substitute = QString());

        //! pointers to our setting objects
        QSettings *systemSettings;
        QSettings *userSettings;
};

#endif
