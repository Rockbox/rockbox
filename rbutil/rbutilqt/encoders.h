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
 
#ifndef ENCODERS_H
#define ENCODERS_H
 
#include <QtCore>
 
#include "rbsettings.h"

#include "rbspeex.h"


class EncBase : public QObject
{
    Q_OBJECT
    public:
        EncBase(QObject *parent );

        virtual bool encode(QString input,QString output)
            {(void)input; (void)output; return false;}
        virtual bool start(){return false;}
        virtual bool stop(){return false;}
        virtual void showCfg(){}
        virtual bool configOk(){return false;}

        void setCfg(RbSettings *sett){settings = sett;}
        static QString getEncoderName(QString);
        static EncBase* getEncoder(QString);
        static QStringList getEncoderList(void);

    public slots:
        virtual void accept(void){}
        virtual void reject(void){}
        virtual void reset(void){}
    private:
        static void initEncodernamesList(void);

    protected:
        RbSettings* settings;

        static QMap<QString,QString> encoderList;
        static QMap<QString,EncBase*> encoderCache;
};



class EncExes : public EncBase
{
    Q_OBJECT
public:
    EncExes(QString name,QObject *parent = NULL);
    virtual bool encode(QString input,QString output);
    virtual bool start();
    virtual bool stop() {return true;}
    virtual void showCfg();
    virtual bool configOk();
    
private:
    QString m_name;
    QString m_EncExec;
    QString m_EncOpts;
    QMap<QString,QString> m_TemplateMap;
    QString m_EncTemplate;
};

class EncRbSpeex : public EncBase 
{
    Q_OBJECT
public:
    EncRbSpeex(QObject *parent = NULL);
    virtual bool encode(QString input,QString output);
    virtual bool start();
    virtual bool stop() {return true;}
    virtual void showCfg();
    virtual bool configOk();

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
