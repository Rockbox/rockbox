/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
*
*   Copyright (C) 2009 by Dominik Wenger
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

#ifndef TTSFESTIVAL_H
#define TTSFESTIVAL_H

#include <QTemporaryFile>
#include "ttsbase.h"

class TTSFestival : public TTSBase
{
    enum ESettings
    {
        eSERVERPATH,
        eCLIENTPATH,
        eVOICE,
        eVOICEDESC
    };

    Q_OBJECT
    public:
        TTSFestival(QObject* parent=NULL) : TTSBase(parent) {}
        ~TTSFestival();
        bool start(QString *errStr);
        bool stop();
        TTSStatus voice(QString text,QString wavfile,  QString *errStr);

        // for settings
        bool configOk();
        void generateSettings();
        void saveSettings();

        private slots:
            void updateVoiceList();
        void updateVoiceDescription();
        void clearVoiceDescription();
    private:
        QTemporaryFile prologFile;
        QString prologPath;
        QString currentPath;
        QStringList  getVoiceList();
        QString getVoiceInfo(QString voice);

        inline void startServer();
        inline bool ensureServerRunning();
        QString queryServer(QString query, int timeout = -1);
        QProcess serverProcess;
        QStringList voices;
        QMap<QString, QString> voiceDescriptions;
};


#endif
