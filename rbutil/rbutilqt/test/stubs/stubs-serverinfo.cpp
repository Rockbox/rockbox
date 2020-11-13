/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2020 Dominik Riebeling
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

// Stubs for ServerInfo unit test.

#include "rbsettings.h"
#include "systeminfo.h"

QVariant SystemInfo::platformValue(SystemInfo::PlatformInfo info, QString platform)
{
    switch(info) {
        case SystemInfo::Manual:
            if (platform == "iriverh120") return "iriverh100";
            if (platform == "ipodmini2g") return "ipodmini1g";
            break;
        case SystemInfo::BuildserverModel:
            return platform.split('.').at(0);
        default:
            return QString();
    }
    return QString();
}

QVariant SystemInfo::value(SystemInfo::SystemInfos info, SystemInfo::BuildType type)
{
    (void)info;  // test is currently only using BuildUrl.
    switch(type) {
        case SystemInfo::BuildCurrent:
            return QString("https://unittest/dev/rockbox-%MODEL%.zip");
        case SystemInfo::BuildDaily:
            return QString("https://unittest/daily/rockbox-%MODEL%-%RELVERSION%.zip");
        case SystemInfo::BuildRelease:
            return QString("https://unittest/release/%RELVERSION%/rockbox-%MODEL%-%RELVERSION%.zip");
        case SystemInfo::BuildCandidate:
            return QString("https://unittest/rc/%RELVERSION%/rockbox-%MODEL%-%RELVERSION%.zip");
        default:
            break;
    }
    return QString();
}

QStringList SystemInfo::platforms(SystemInfo::PlatformType type, QString variant)
{
    // stub implementation: we have a fixed list of players, and only iaudiox5
    // has variant iaudiox5.v
    QStringList result;
    result << "iriverh100" << "iriverh120" << "iriverh300"
           << "ipodmini2g" << "archosrecorder" << "archosfmrecorder"
           << "gigabeatfx" << "iaudiom3" << "sansae200" << "iriverh10";
    switch (type)
    {
        case SystemInfo::PlatformBaseDisabled:
            // return base platforms only, i.e. return iaudiox5 for iaudiox5.v
            result << "iaudiox5";
            break;
        case SystemInfo::PlatformVariantDisabled:
            // return variants for the passed variant
            if (variant == "iaudiox5") {
                result.clear();
                result << "iaudiox5" << "iaudiox5.v";
            }
            else {
                result.clear();
                result << variant;
            }
            break;
        case SystemInfo::PlatformAllDisabled:
            // return all, both with and without variant.
            result << "iaudiox5" << "iaudiox5.v";
            break;
        default:
            break;
    }
    return result;
}


QVariant RbSettings::value(UserSettings setting)
{
    switch (setting)
    {
        case RbSettings::CurrentPlatform:
            return QString("ipodmini2g");
        default:
            return QString("");
    }
}

