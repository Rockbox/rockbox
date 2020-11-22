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

// Parse and provide information from build server via build-info file.
// This is a singleton.

#ifndef SERVERINFO_H
#define SERVERINFO_H

#include <QtCore>
#define STATUS_RETIRED  0
#define STATUS_UNUSABLE 1
#define STATUS_UNSTABLE 2
#define STATUS_STABLE   3

class ServerInfo : public QObject
{
    Q_OBJECT
    public:

        //! All Server infos
        enum ServerInfos {
            CurReleaseVersion,
            CurStatus,
            CurReleaseUrl,
            CurDevelUrl,
            BleedingRevision,
            BleedingDate,
            RelCandidateVersion,
            RelCandidateUrl,
            DailyVersion,
            DailyUrl
        };

        static ServerInfo* instance();

        //! read in buildinfo file
        void readBuildInfo(QString file);
        //! get a value from server info for a named platform.
        QVariant platformValue(enum ServerInfos setting, QString platform = "");
        //! Get status number as string
        QString statusAsString(QString platform = "");

    protected:
        ServerInfo() : serverSettings(nullptr) {}

    private:
        static ServerInfo* infoInstance;
        QSettings* serverSettings;

};

#endif

