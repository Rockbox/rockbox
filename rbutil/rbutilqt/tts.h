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

 
#ifndef TTS_H
#define TTS_H


#include "rbsettings.h"
#include <QtCore>

#ifndef CONSOLE
#include "ttsgui.h"
#else
#include "ttsguicli.h"
#endif


class TTSBase : public QObject
{
    Q_OBJECT
    public:
        TTSBase();
        virtual bool voice(QString text,QString wavfile)
            { (void)text; (void)wavfile; return false; }
        virtual bool start(QString *errStr) { (void)errStr; return false; }
        virtual bool stop() { return false; }
        virtual void showCfg(){}
        virtual bool configOk() { return false; }

        void setCfg(RbSettings* sett) { settings = sett; }
        
        static TTSBase* getTTS(QString ttsname);
        static QStringList getTTSList();
        static QString getTTSName(QString tts);
        
    public slots:
        virtual void accept(void){}
        virtual void reject(void){}
        virtual void reset(void){}
        
    private:
        //inits the tts List
        static void initTTSList();

    protected:
        RbSettings* settings;
        static QMap<QString,QString> ttsList;
        static QMap<QString,TTSBase*> ttsCache;
};

class TTSSapi : public TTSBase
{
 Q_OBJECT
    public:
        TTSSapi();
        virtual bool voice(QString text,QString wavfile);
        virtual bool start(QString *errStr);
        virtual bool stop();
        virtual void showCfg();
        virtual bool configOk();
    
        QStringList getVoiceList(QString language);
    private:
        QProcess* voicescript;
        
        QString defaultLanguage;
        
        QString m_TTSexec;
        QString m_TTSOpts;
        QString m_TTSTemplate;
        QString m_TTSLanguage;
        QString m_TTSVoice;
        QString m_TTSSpeed;
        bool m_sapi4;
};


class TTSExes : public TTSBase
{
    Q_OBJECT
    public:
        TTSExes(QString name);
        virtual bool voice(QString text,QString wavfile);
        virtual bool start(QString *errStr);
        virtual bool stop() {return true;}
        virtual void showCfg();
        virtual bool configOk();

    private:
        QString m_name;
        QString m_TTSexec;
        QString m_TTSOpts;
        QString m_TTSTemplate;
        QMap<QString,QString> m_TemplateMap;
};

#endif
