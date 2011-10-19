/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
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
 
#ifndef ENCODERS_H
#define ENCODERS_H
 
#include <QtCore>
 
#include "encttssettings.h"
#include "rbspeex.h"


class EncBase : public EncTtsSettingInterface
{
    Q_OBJECT
    public:
        EncBase(QObject *parent );

        //! Child class should encode a wav file 
        virtual bool encode(QString input,QString output) =0;
        //! Child class should do startup
        virtual bool start()=0;
        //! Child class should stop
        virtual bool stop()=0;
        
        // settings
        //! Child class should return true when configuration is ok
        virtual bool configOk()=0;
         //! Child class should fill in the setttingsList
        virtual void generateSettings() = 0;
        //! Chlid class should commit the from SettingsList to permanent storage
        virtual void saveSettings() = 0;
     
        // static functions
        static QString getEncoderName(QString name);
        static EncBase* getEncoder(QObject* parent,QString name);
        static QStringList getEncoderList(void);

    private:
        static void initEncodernamesList(void);

    protected:
        static QMap<QString,QString> encoderList;
};


class EncExes : public EncBase
{
    enum ESettings
    {
        eEXEPATH,
        eEXEOPTIONS
    };
    
    Q_OBJECT
public:
    EncExes(QString name,QObject *parent = NULL);
    bool encode(QString input,QString output);
    bool start();
    bool stop() {return true;}
    
    // setting
    bool configOk();
    void generateSettings();
    void saveSettings();

private:
    QString m_name;
    QString m_EncExec;
    QString m_EncOpts;
    QMap<QString,QString> m_TemplateMap;
    QString m_EncTemplate;
};

class EncRbSpeex : public EncBase 
{
    enum ESettings
    {
        eVOLUME,
        eQUALITY,
        eCOMPLEXITY,
        eNARROWBAND
    };
    
    Q_OBJECT
public:
    EncRbSpeex(QObject *parent = NULL);
    bool encode(QString input,QString output);
    bool start();
    bool stop() {return true;}
    
    // for settings view
    bool configOk();
    void generateSettings();
    void saveSettings();
    
private:
    float quality;
    float volume;
    int complexity;
    bool narrowband;
    
    float defaultQuality;
    float defaultVolume;
    int defaultComplexity;
    bool defaultBand;
};

 
#endif
