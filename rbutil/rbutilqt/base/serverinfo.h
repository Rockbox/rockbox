/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2010 by Dominik Wenger
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

#ifndef SERVERINFO_H
#define SERVERINFO_H

#include <QtCore>

class ServerInfo : public QObject
{
    Q_OBJECT
    public:
    
        //! All Server infos
        enum ServerInfos {
            CurReleaseVersion,
            CurStatus,
            DailyRevision,
            DailyDate,
            BleedingRevision,
            BleedingDate,
        };
        
        //! read in buildinfo file
        static void readBuildInfo(QString file);
        //! read in bleeding info file
        static void readBleedingInfo(QString file);
        //! get a value from server info
        static QVariant value(enum ServerInfos setting);
        //! get a value from server info for a named platform.
        static QVariant platformValue(QString platform, enum ServerInfos setting);
    
    private:
        //! set a server info to all platforms where configurename matches
        static void setAllConfigPlatformValue(QString configplatform,ServerInfos info, QVariant value);
        //! set a server info value
        static void setValue(enum ServerInfos setting , QVariant value);
        //! set a value for a server info for a named platform.
        static void setPlatformValue(QString platform, enum ServerInfos setting, QVariant value);
        //! you shouldnt call this, its a fully static class
        ServerInfo() {}

        //! map of server infos
        static QMap<QString, QVariant> serverInfos;

};

#endif

