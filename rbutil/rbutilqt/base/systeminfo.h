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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef SYSTEMINFO_H
#define SYSTEMINFO_H

#include <QtCore>

class SystemInfo : public QObject
{
    Q_OBJECT
    public:
        //! Type of requested usb-id map
        enum MapType {
            MapDevice,
            MapError,
            MapIncompatible,
        };

        enum BuildType {
            BuildCurrent,
            BuildDaily,
            BuildRelease,
            BuildCandidate
        };

        //! All system settings
        enum SystemInfos {
            BuildUrl,
            FontUrl,
            VoiceUrl,
            ManualUrl,
            BootloaderUrl,
            BootloaderInfoUrl,
            DoomUrl,
            Duke3DUrl,
            QuakeUrl,
            PuzzFontsUrl,
            Wolf3DUrl,
            XWorldUrl,
            ReleaseUrl,
            BuildInfoUrl,
            GenlangUrl,
            ThemesUrl,
            ThemesInfoUrl,
            RbutilUrl,
        };

        enum PlatformInfo {
            Manual,
            BootloaderMethod,
            BootloaderName,
            BootloaderFile,
            BootloaderFilter,
            Encoder,
            Brand,
            Name,
            ConfigureModel,
            PlayerPicture,
        };

        enum PlatformType {
            PlatformAll,
            PlatformAllDisabled,
            PlatformBase,
            PlatformBaseDisabled,
            PlatformVariant,
            PlatformVariantDisabled
        };

        //! return a list of all platforms (rbutil internal names)
        static QStringList platforms(enum PlatformType type = PlatformAll,
                                     QString variant="");
        //! returns a map of all languages.
        //! Maps <language code> to (<language name>, <display name>)
        static QMap<QString, QStringList> languages(bool namesOnly = false);
        //! returns a map of usb-ids and their targets
        static QMap<int, QStringList> usbIdMap(enum MapType type);
        //! get a value from system settings
        static QVariant value(enum SystemInfos info, BuildType type = BuildCurrent);
        //! get a value from system settings for a named platform.
        static QVariant platformValue(enum PlatformInfo info, QString platform = "");

    private:
        //! you shouldnt call this, its a fully static calls
        SystemInfo() {}
        //! create the setting objects if neccessary
        static void ensureSystemInfoExists();
        //! pointers to our setting objects
        static QSettings *systemInfos;
};

#endif

