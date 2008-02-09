/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: tts.h 15212 2007-10-19 21:49:07Z domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

 
#ifndef TTS_H
#define TTS_H


#include "rbsettings.h"
#include <QtGui>

#ifndef CONSOLE
#include "ttsgui.h"
#else
#include "ttsguicli.h"
#endif

class TTSBase;

//inits the tts List
void initTTSList();
// function to get a specific tts
TTSBase* getTTS(QString ttsname);
// get the list of tts, nice names
QStringList getTTSList();
QString getTTSName(QString tts);


class TTSBase : public QObject
{
    Q_OBJECT
public:
    TTSBase();
    virtual bool voice(QString text,QString wavfile) {(void)text; (void)wavfile; return false;}
    virtual bool start(QString *errStr){(void)errStr; return false;}
    virtual bool stop(){return false;}
    virtual void showCfg(){}
    virtual bool configOk(){return false;}
    
    void setCfg(RbSettings* sett){settings = sett;}
     
public slots:
    virtual void accept(void){}
    virtual void reject(void){}
    virtual void reset(void){}

protected:
    RbSettings* settings;
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
