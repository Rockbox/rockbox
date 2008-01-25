/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: encoders.h 15212 2007-10-19 21:49:07Z domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
#ifndef ENCODERS_H
#define ENCODERS_H
 
#include <QtGui>
 
#include "ui_rbspeexcfgfrm.h"
#include "ui_encexescfgfrm.h"
#include "rbsettings.h"

extern "C"
{
    #include "rbspeex.h"
}

class EncBase;

//inits the encoder List
void initEncoderList();
// function to get a specific encoder
EncBase* getEncoder(QString encname);
// get the list of encoders, nice names
QStringList getEncoderList();


class EncBase : public QDialog
{
    Q_OBJECT
public:
    EncBase(QWidget *parent );
    
    virtual bool encode(QString input,QString output){return false;}
    virtual bool start(){return false;}
    virtual bool stop(){return false;}
    virtual void showCfg(){}
    virtual bool configOk(){return false;}

    void setCfg(RbSettings *sett){settings = sett;}
     
public slots:
    virtual void accept(void){}
    virtual void reject(void){}
    virtual void reset(void){}

protected:
   
    RbSettings* settings;
};



class EncExes : public EncBase
{
    Q_OBJECT
public:
    EncExes(QString name,QWidget *parent = NULL);
    virtual bool encode(QString input,QString output);
    virtual bool start();
    virtual bool stop() {return true;}
    virtual void showCfg();
    virtual bool configOk();
    
public slots:
    virtual void accept(void);
    virtual void reject(void);
    virtual void reset(void);
    void browse(void);

private:
    Ui::EncExesCfgFrm ui;
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
    EncRbSpeex(QWidget *parent = NULL);
    virtual bool encode(QString input,QString output);
    virtual bool start();
    virtual bool stop() {return true;}
    virtual void showCfg();
    virtual bool configOk();

public slots:
    virtual void accept(void);
    virtual void reject(void);
    virtual void reset(void);

private:
    Ui::RbSpeexCfgFrm ui;
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
