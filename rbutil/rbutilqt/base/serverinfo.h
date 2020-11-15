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
            ManualPdfUrl,
            ManualHtmlUrl,
            ManualZipUrl,
            BleedingRevision,
            BleedingDate,
            RelCandidateVersion,
            RelCandidateUrl,
        };

        //! read in buildinfo file
        static void readBuildInfo(QString file);
        //! get a value from server info for a named platform.
        static QVariant platformValue(enum ServerInfos setting, QString platform = "");
        //! Convert status number to string
        static QString statusToString(int status);

    private:
        //! you shouldnt call this, its a fully static class
        ServerInfo() {}

        //! map of server infos
        static QMap<QString, QVariant> serverInfos;

};

#endif

