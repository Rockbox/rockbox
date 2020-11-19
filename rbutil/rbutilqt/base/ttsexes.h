/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
*
*   Copyright (C) 2009 by Dominik Wenger
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

#ifndef TTSEXES_H
#define TTSEXES_H

#include <QtCore>
#include "ttsbase.h"

class TTSExes : public TTSBase
{
    enum ESettings
    {
        eEXEPATH,
        eOPTIONS
    };

    Q_OBJECT
    public:
        TTSExes(QObject* parent=nullptr);
        TTSStatus voice(QString text, QString wavfile, QString *errStr);
        bool start(QString *errStr);
        bool stop() {return true;}
        QString voiceVendor(void) { return QString(); }
        Capabilities capabilities();

        // for settings
        void generateSettings();
        void saveSettings();
        bool configOk();

    private:
        void loadSettings(void);

    protected:
        QString m_TTSTemplate;
        QString m_TTSSpeakTemplate;
        QString m_name;
        QString m_TTSexec;
        QString m_TTSOpts;
        TTSBase::Capabilities m_capabilities;
};

#endif
