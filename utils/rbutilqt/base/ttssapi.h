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

#ifndef TTSSAPI_H
#define TTSSAPI_H

#include "ttsbase.h"

class TTSSapi : public TTSBase
{
    //! Enum to identify the settings
    enum ESettings
    {
        eLANGUAGE,
        eVOICE,
        eSPEED,
        eOPTIONS
    };

    Q_OBJECT
    public:
        TTSSapi(QObject* parent=nullptr);

        TTSStatus voice(QString text,QString wavfile, QString *errStr);
        bool start(QString *errStr);
        bool stop();
        QString voiceVendor(void);
        Capabilities capabilities();

        // for settings
        bool configOk();
        void generateSettings();
        void saveSettings();

    private slots:
        void updateVoiceList();

    private:
        QStringList getVoiceList(QString language);

        QProcess* voicescript;
        QTextStream* voicestream;
        QString defaultLanguage;

        QString m_TTSexec;
        QString m_TTSOpts;
        QString m_TTSLanguage;
        QString m_TTSVoice;
        QString m_TTSSpeed;
        bool m_started;

    protected:
        QString m_TTSTemplate;
        QString m_TTSVoiceTemplate;
        QString m_TTSType;
};



#endif
