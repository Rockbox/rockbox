/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
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

class RbSettings : public QObject
{
    Q_OBJECT
    public:

        //! All user settings
        enum UserSettings {
            RbutilVersion,
            ShowChangelog,
            CurrentPlatform,
            Mountpoint,
            CachePath,
            Build,
            ProxyType,
            Proxy,
            OfPath,
            Platform,
            Language,
            BackupPath,
            InstallRockbox,
            InstallFonts,
            InstallThemes,
            InstallGamefiles,
            InstallVoice,
            InstallManual,
            Tts,
            UseTtsCorrections,
            TalkFolders,
            TalkProcessFiles,
            TalkProcessFolders,
            TalkRecursive,
            TalkSkipExisting,
            TalkStripExtensions,
            TalkIgnoreFiles,
            TalkIgnoreWildcards,
            VoiceLanguage,
            TtsLanguage,
            TtsOptions,
            TtsPath,
            TtsVoice,
            TtsPitch,
            EncoderPath,
            EncoderOptions,
            WavtrimThreshold,
            EncoderComplexity,
            TtsSpeed,
            CacheDisabled,
            TtsUseSapi4,
            EncoderNarrowBand,
            EncoderQuality,
            EncoderVolume,
        };

        //! call this to flush the user Settings
        static void sync();
        //! returns the filename of the usersettings file
        static QString userSettingFilename();
        //! get a value from user settings
        static QVariant value(enum UserSettings setting);
        //! set a user setting value
        static void setValue(enum UserSettings setting , QVariant value);
        //! get a user setting from a subvalue (ie for encoders and tts engines)
        static QVariant subValue(QString sub, enum UserSettings setting);
        //! set a user setting from a subvalue (ie for encoders and tts engines)
        static void setSubValue(QString sub, enum UserSettings setting, QVariant value);

    private:
        //! you shouldnt call this, its a fully static calls
        RbSettings() {}
        //! create the setting objects if neccessary
        static void ensureRbSettingsExists();
        //! create a settings path, substitute platform, tts and encoder
        static QString constructSettingPath(QString path, QString substitute = QString());

        //! pointers to our setting object
        static QSettings *userSettings;
};

#endif

