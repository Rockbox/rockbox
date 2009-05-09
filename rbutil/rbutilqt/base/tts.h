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

#include <QtCore>
#include <QProcess>
#include <QDateTime>
#include <QRegExp>
#include <QTcpSocket>

#include "encttssettings.h"

enum TTSStatus{ FatalError, NoError, Warning };

class TTSBase : public EncTtsSettingInterface
{
    Q_OBJECT
    public:
        TTSBase(QObject *parent);
        //! Child class should generate a clip
        virtual TTSStatus voice(QString text,QString wavfile, QString* errStr) =0;
        //! Child class should do startup
        virtual bool start(QString *errStr) =0;
        //! child class should stop
        virtual bool stop() =0;
        
        // configuration
        //! Child class should return true, when configuration is good
        virtual bool configOk()=0;        
         //! Child class should generate and insertSetting(..) its settings
        virtual void generateSettings() = 0;
        //! Chlid class should commit the Settings to permanent storage
        virtual void saveSettings() = 0;
        
        // static functions
        static TTSBase* getTTS(QObject* parent,QString ttsname);
        static QStringList getTTSList();
        static QString getTTSName(QString tts); 
                
    private:
        //inits the tts List
        static void initTTSList();

    protected:
        static QMap<QString,QString> ttsList;
};

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
        TTSSapi(QObject* parent=NULL);
        
        TTSStatus voice(QString text,QString wavfile, QString *errStr);
        bool start(QString *errStr);
        bool stop();
        
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
        QString m_TTSTemplate;
        QString m_TTSLanguage;
        QString m_TTSVoice;
        QString m_TTSSpeed;
        bool m_sapi4;
};


class TTSExes : public TTSBase
{
    enum ESettings
    {
        eEXEPATH,
        eOPTIONS
    };
    
    Q_OBJECT
    public:
        TTSExes(QString name,QObject* parent=NULL);
        TTSStatus voice(QString text,QString wavfile, QString *errStr);
        bool start(QString *errStr);
        bool stop() {return true;}
        
        // for settings 
        void generateSettings();
        void saveSettings();
        bool configOk();

    private:
        QString m_name;
        QString m_TTSexec;
        QString m_TTSOpts;
        QString m_TTSTemplate;
        QMap<QString,QString> m_TemplateMap;
};

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
    TTSFestival(QObject* parent=NULL) :TTSBase(parent) {}
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
	QStringList  getVoiceList(QString path ="");
	QString 	 getVoiceInfo(QString voice,QString path ="");
    
	inline void	startServer(QString path="");
	inline void	ensureServerRunning(QString path="");
	QString	queryServer(QString query, int timeout = -1,QString path="");
	QProcess serverProcess;
	QStringList voices;
	QMap<QString, QString> voiceDescriptions;
};

#endif
