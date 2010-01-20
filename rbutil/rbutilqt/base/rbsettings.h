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
        //! Type of requested usb-id map
        enum MapType {
            MapDevice,
            MapError,
            MapIncompatible,
        };
        
        //! All user settings
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
        
        //! All system settings
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
            RbutilUrl,
            BleedingInfo,
            CurPlatformName,
            CurManual,
            CurBootloaderMethod,
            CurBootloaderName,
            CurBootloaderFile,
            CurEncoder,
            CurBrand,
            CurName,
            CurBuildserverModel,
            CurConfigureModel,
        };
        
        //! All Server settings
        enum ServerSettings {
            CurReleaseVersion,
            CurStatus,
            DailyRevision,
            DailyDate,
            BleedingRevision,
            BleedingDate,
        };
        
        //! call this to flush the user Settings
        static void sync();
        //! returns the filename of the usersettings file
        static QString userSettingFilename();
        //! return a list of all platforms (rbutil internal names)
        static QStringList platforms(void);
        //! returns a list of all languages
        static QStringList languages(void);
        //! maps a platform to its name
        static QString name(QString plattform);
        //! maps a platform to its brand
        static QString brand(QString plattform);
        //! returns a map of usb-ids and their targets
        static QMap<int, QString> usbIdMap(enum MapType);
        //! get a value from system settings
        static QVariant value(enum SystemSettings setting);
        //! get a value from user settings
        static QVariant value(enum UserSettings setting);
        //! get a value from server settings
        static QVariant value(enum ServerSettings setting);
        //! set a user setting value
        static void setValue(enum UserSettings setting , QVariant value);
        //! set a server setting value
        static void setValue(enum ServerSettings setting , QVariant value);
        //! get a user setting from a subvalue (ie for encoders and tts engines)
        static QVariant subValue(QString sub, enum UserSettings setting);
        //! set a user setting from a subvalue (ie for encoders and tts engines)
        static void setSubValue(QString sub, enum UserSettings setting, QVariant value);
        //! set a value for a server settings for a named platform.
        static void setPlatformValue(QString platform, enum ServerSettings setting, QVariant value);
        //! get a value from system settings for a named platform.
        static QVariant platformValue(QString platform, enum SystemSettings setting);

    private:
        //! you shouldnt call this, its a fully static calls
        RbSettings() {}
        //! create the setting objects if neccessary
        static void ensureRbSettingsExists();
        //! create a settings path, substitute platform, tts and encoder
        static QString constructSettingPath(QString path, QString substitute = QString());

        //! pointers to our setting objects
        static QSettings *systemSettings;
        static QSettings *userSettings;
        static QSettings *serverSettings;
};

#endif

