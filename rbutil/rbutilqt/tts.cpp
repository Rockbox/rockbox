/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: tts.cpp 15212 2007-10-19 21:49:07Z domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "tts.h"


// static variables
QMap<QString,QString> TTSBase::ttsList;
QMap<QString,TTSBase*> TTSBase::ttsCache;

// static functions
void TTSBase::initTTSList()
{
    ttsList["espeak"] = "Espeak TTS Engine";
    ttsList["flite"] = "Flite TTS Engine";
    ttsList["swift"] = "Swift TTS Engine";
#if defined(Q_OS_WIN)
    ttsList["sapi"] = "Sapi TTS Engine";
#endif

}

// function to get a specific encoder
TTSBase* TTSBase::getTTS(QString ttsName)
{
    // check cache
    if(ttsCache.contains(ttsName))
        return ttsCache.value(ttsName);

    TTSBase* tts;
    if(ttsName == "sapi")
    {
        tts = new TTSSapi();
        ttsCache[ttsName] = tts; 
        return tts;
    }
    else 
    {
        tts = new TTSExes(ttsName);
        ttsCache[ttsName] = tts;
        return tts;
    }
}

// get the list of encoders, nice names
QStringList TTSBase::getTTSList()
{
    // init list if its empty
    if(ttsList.count() == 0)
        initTTSList();

    return ttsList.keys();
}

// get nice name of a specific tts
QString TTSBase::getTTSName(QString tts)
{
    if(ttsList.isEmpty())
        initTTSList();
    return ttsList.value(tts);
}

/*********************************************************************
* TTS Base
**********************************************************************/
TTSBase::TTSBase(): QObject()
{

}

/*********************************************************************
* General TTS Exes
**********************************************************************/
TTSExes::TTSExes(QString name) : TTSBase()
{
    m_name = name;
    
    m_TemplateMap["espeak"] = "\"%exe\" \"%options\" -w \"%wavfile\" \"%text\"";
    m_TemplateMap["flite"] = "\"%exe\" \"%options\" -o \"%wavfile\" \"%text\"";
    m_TemplateMap["swift"] = "\"%exe\" \"%options\" -o \"%wavfile\" \"%text\"";
       
}

bool TTSExes::start(QString *errStr)
{
    m_TTSexec = settings->ttsPath(m_name);
    m_TTSOpts = settings->ttsOptions(m_name);
    
    m_TTSTemplate = m_TemplateMap.value(m_name);

    QFileInfo tts(m_TTSexec);
    if(tts.exists())
    {
        return true;
    }
    else
    {
        *errStr = tr("TTS executable not found");
        return false;
    }
}

bool TTSExes::voice(QString text,QString wavfile)
{
    QString execstring = m_TTSTemplate;

    execstring.replace("%exe",m_TTSexec);
    execstring.replace("%options",m_TTSOpts);
    execstring.replace("%wavfile",wavfile);
    execstring.replace("%text",text);
    //qDebug() << "voicing" << execstring;
    QProcess::execute(execstring);
    return true;

}

void TTSExes::showCfg()
{
#ifndef CONSOLE
    TTSExesGui gui;
#else
	TTSExesGuiCli gui;
#endif
    gui.setCfg(settings);
    gui.showCfg(m_name);
}

bool TTSExes::configOk()
{
    QString path = settings->ttsPath(m_name);
    
    if (QFileInfo(path).exists())
        return true;
  
    return false;
}

/*********************************************************************
* TTS Sapi
**********************************************************************/
TTSSapi::TTSSapi() : TTSBase()
{
    m_TTSTemplate = "cscript //nologo \"%exe\" /language:%lang /voice:\"%voice\" /speed:%speed \"%options\"";
    defaultLanguage ="english";
    m_sapi4 =false;
}


bool TTSSapi::start(QString *errStr)
{    

    m_TTSOpts = settings->ttsOptions("sapi");
    m_TTSLanguage =settings->ttsLang("sapi");
    m_TTSVoice=settings->ttsVoice("sapi");
    m_TTSSpeed=QString("%1").arg(settings->ttsSpeed("sapi"));
    m_sapi4 = settings->ttsUseSapi4();
    
    QFile::remove(QDir::tempPath() +"/sapi_voice.vbs");
    QFile::copy(":/builtin/sapi_voice.vbs",QDir::tempPath() + "/sapi_voice.vbs");
    m_TTSexec = QDir::tempPath() +"/sapi_voice.vbs";
    
    QFileInfo tts(m_TTSexec);
    if(!tts.exists())
    {
        *errStr = tr("Could not copy the Sapi-script");
        return false;
    }    
    // create the voice process
    QString execstring = m_TTSTemplate;
    execstring.replace("%exe",m_TTSexec);
    execstring.replace("%options",m_TTSOpts);
    execstring.replace("%lang",m_TTSLanguage);
    execstring.replace("%voice",m_TTSVoice);
    execstring.replace("%speed",m_TTSSpeed);
    
    if(m_sapi4)
        execstring.append(" /sapi4 ");
    
    qDebug() << "init" << execstring; 
    voicescript = new QProcess(NULL);
    //connect(voicescript,SIGNAL(readyReadStandardError()),this,SLOT(error()));
    
    voicescript->start(execstring);
    if(!voicescript->waitForStarted())
    {
        *errStr = tr("Could not start the Sapi-script");
        return false;
    }
    
    if(!voicescript->waitForReadyRead(100))  
    {
        *errStr = voicescript->readAllStandardError();
        if(*errStr != "")
            return false;    
    }
    return true;
}


QStringList TTSSapi::getVoiceList(QString language)
{
    QStringList result;
 
    QFile::copy(":/builtin/sapi_voice.vbs",QDir::tempPath() + "/sapi_voice.vbs");
    m_TTSexec = QDir::tempPath() +"/sapi_voice.vbs";
    
    QFileInfo tts(m_TTSexec);
    if(!tts.exists())
        return result;
        
    // create the voice process
    QString execstring = "cscript //nologo \"%exe\" /language:%lang /listvoices";
    execstring.replace("%exe",m_TTSexec);
    execstring.replace("%lang",language);
    
    if(settings->ttsUseSapi4())
        execstring.append(" /sapi4 ");
    
    qDebug() << "init" << execstring; 
    voicescript = new QProcess(NULL);
    voicescript->start(execstring);
    if(!voicescript->waitForStarted())
        return result;
 
    voicescript->waitForReadyRead();
    
    QString dataRaw = voicescript->readAllStandardError().data();
    result = dataRaw.split(",",QString::SkipEmptyParts);
    result.sort();
    result.removeFirst();
    for(int i = 0; i< result.size();i++)
    {
        result[i] = result.at(i).simplified();
    }
    
    
    delete voicescript;
    QFile::setPermissions(QDir::tempPath() +"/sapi_voice.vbs",QFile::ReadOwner |QFile::WriteOwner|QFile::ExeOwner 
                                                             |QFile::ReadUser| QFile::WriteUser| QFile::ExeUser
                                                             |QFile::ReadGroup  |QFile::WriteGroup	|QFile::ExeGroup
                                                             |QFile::ReadOther  |QFile::WriteOther	|QFile::ExeOther );
    QFile::remove(QDir::tempPath() +"/sapi_voice.vbs");
    
    return result;
}



bool TTSSapi::voice(QString text,QString wavfile)
{
    QString query = "SPEAK\t"+wavfile+"\t"+text+"\r\n";
    qDebug() << "voicing" << query;
    voicescript->write(query.toUtf8());
    voicescript->write("SYNC\tbla\r\n");
    voicescript->waitForReadyRead();
    return true;
}

bool TTSSapi::stop()
{   
    QString query = "QUIT\r\n";
    voicescript->write(query.toUtf8());
    voicescript->waitForFinished();
    delete voicescript;
    QFile::setPermissions(QDir::tempPath() +"/sapi_voice.vbs",QFile::ReadOwner |QFile::WriteOwner|QFile::ExeOwner 
                                                             |QFile::ReadUser| QFile::WriteUser| QFile::ExeUser
                                                             |QFile::ReadGroup  |QFile::WriteGroup	|QFile::ExeGroup
                                                             |QFile::ReadOther  |QFile::WriteOther	|QFile::ExeOther );
    QFile::remove(QDir::tempPath() +"/sapi_voice.vbs");
    return true;
}


void TTSSapi::showCfg()
{
#ifndef CONSOLE
    TTSSapiGui gui(this);
#else
	TTSSapiGuiCli gui(this);
#endif
    gui.setCfg(settings);
    gui.showCfg();
}

bool TTSSapi::configOk()
{
    if(settings->ttsVoice("sapi").isEmpty())
        return false;
    return true;
}


