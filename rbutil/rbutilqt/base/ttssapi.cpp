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
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "ttssapi.h"
#include "utils.h"
#include "rbsettings.h"
#include "systeminfo.h"

TTSSapi::TTSSapi(QObject* parent) : TTSBase(parent)
{
    m_TTSTemplate = "cscript //nologo \"%exe\" /language:%lang /voice:\"%voice\" /speed:%speed \"%options\"";
    defaultLanguage ="english";
    m_sapi4 =false;
}

void TTSSapi::generateSettings()
{
    // language
    QStringList languages = SystemInfo::languages();
    languages.sort();
    EncTtsSetting* setting =new EncTtsSetting(this,EncTtsSetting::eSTRINGLIST,
        tr("Language:"),RbSettings::subValue("sapi",RbSettings::TtsLanguage),languages);
    connect(setting,SIGNAL(dataChanged()),this,SLOT(updateVoiceList()));
    insertSetting(eLANGUAGE,setting);
    // voice
    setting = new EncTtsSetting(this,EncTtsSetting::eSTRINGLIST,
        tr("Voice:"),RbSettings::subValue("sapi",RbSettings::TtsVoice),getVoiceList(RbSettings::subValue("sapi",RbSettings::TtsLanguage).toString()),EncTtsSetting::eREFRESHBTN);
    connect(setting,SIGNAL(refresh()),this,SLOT(updateVoiceList()));
    insertSetting(eVOICE,setting);
    //speed
    insertSetting(eSPEED,new EncTtsSetting(this,EncTtsSetting::eINT,
        tr("Speed:"),RbSettings::subValue("sapi",RbSettings::TtsSpeed),-10,10));
    // options
    insertSetting(eOPTIONS,new EncTtsSetting(this,EncTtsSetting::eSTRING,
        tr("Options:"),RbSettings::subValue("sapi",RbSettings::TtsOptions)));

}

void TTSSapi::saveSettings()
{
    //save settings in user config
    RbSettings::setSubValue("sapi",RbSettings::TtsLanguage,getSetting(eLANGUAGE)->current().toString());
    RbSettings::setSubValue("sapi",RbSettings::TtsVoice,getSetting(eVOICE)->current().toString());
    RbSettings::setSubValue("sapi",RbSettings::TtsSpeed,getSetting(eSPEED)->current().toInt());
    RbSettings::setSubValue("sapi",RbSettings::TtsOptions,getSetting(eOPTIONS)->current().toString());

    RbSettings::sync();
}

void TTSSapi::updateVoiceList()
{
    qDebug() << "update voiceList";
    QStringList voiceList = getVoiceList(getSetting(eLANGUAGE)->current().toString());
    getSetting(eVOICE)->setList(voiceList);
    if(voiceList.size() > 0) getSetting(eVOICE)->setCurrent(voiceList.at(0));
    else getSetting(eVOICE)->setCurrent("");
}

bool TTSSapi::start(QString *errStr)
{

    m_TTSOpts = RbSettings::subValue("sapi",RbSettings::TtsOptions).toString();
    m_TTSLanguage =RbSettings::subValue("sapi",RbSettings::TtsLanguage).toString();
    m_TTSVoice=RbSettings::subValue("sapi",RbSettings::TtsVoice).toString();
    m_TTSSpeed=RbSettings::subValue("sapi",RbSettings::TtsSpeed).toString();
    m_sapi4 = RbSettings::subValue("sapi",RbSettings::TtsUseSapi4).toBool();

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

    if(!voicescript->waitForReadyRead(300))
    {
        *errStr = voicescript->readAllStandardError();
        if(*errStr != "")
            return false;
    }

    voicestream = new QTextStream(voicescript);
    voicestream->setCodec("UTF16-LE");

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

    if(RbSettings::value(RbSettings::TtsUseSapi4).toBool())
        execstring.append(" /sapi4 ");

    qDebug() << "init" << execstring;
    voicescript = new QProcess(NULL);
    voicescript->start(execstring);
    qDebug() << "wait for started";
    if(!voicescript->waitForStarted())
        return result;
    voicescript->closeWriteChannel();
    voicescript->waitForReadyRead();

    QString dataRaw = voicescript->readAllStandardError().data();
    result = dataRaw.split(",",QString::SkipEmptyParts);
    if(result.size() > 0)
    {
        result.sort();
        result.removeFirst();
        for(int i = 0; i< result.size();i++)
        {
            result[i] = result.at(i).simplified();
        }
    }

    delete voicescript;
    QFile::setPermissions(QDir::tempPath() +"/sapi_voice.vbs",QFile::ReadOwner |QFile::WriteOwner|QFile::ExeOwner
                                                             |QFile::ReadUser| QFile::WriteUser| QFile::ExeUser
                                                             |QFile::ReadGroup  |QFile::WriteGroup    |QFile::ExeGroup
                                                             |QFile::ReadOther  |QFile::WriteOther    |QFile::ExeOther );
    QFile::remove(QDir::tempPath() +"/sapi_voice.vbs");
    return result;
}



TTSStatus TTSSapi::voice(QString text,QString wavfile, QString *errStr)
{
    (void) errStr;
    QString query = "SPEAK\t"+wavfile+"\t"+text+"\r\n";
    qDebug() << "voicing" << query;
    *voicestream << query;
    *voicestream << "SYNC\tbla\r\n";
    voicestream->flush();
    voicescript->waitForReadyRead();
    return NoError;
}

bool TTSSapi::stop()
{

    *voicestream << "QUIT\r\n";
    voicestream->flush();
    voicescript->waitForFinished();
    delete voicestream;
    delete voicescript;
    QFile::setPermissions(QDir::tempPath() +"/sapi_voice.vbs",QFile::ReadOwner |QFile::WriteOwner|QFile::ExeOwner
                                                             |QFile::ReadUser| QFile::WriteUser| QFile::ExeUser
                                                             |QFile::ReadGroup  |QFile::WriteGroup    |QFile::ExeGroup
                                                             |QFile::ReadOther  |QFile::WriteOther    |QFile::ExeOther );
    QFile::remove(QDir::tempPath() +"/sapi_voice.vbs");
    return true;
}

bool TTSSapi::configOk()
{
    if(RbSettings::subValue("sapi",RbSettings::TtsVoice).toString().isEmpty())
        return false;
    return true;
}
