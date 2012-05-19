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
    m_TTSTemplate = "cscript //nologo \"%exe\" /language:%lang /voice:\"%voice\""
        " /speed:%speed \"%options\"";
    defaultLanguage = "english";
    m_sapi4 = false;
    m_started = false;
}

TTSBase::Capabilities TTSSapi::capabilities()
{
    return None;
}

void TTSSapi::generateSettings()
{
    // language
    QMap<QString, QStringList> languages = SystemInfo::languages();
    QStringList langs;
    for(int i = 0; i < languages.values().size(); ++i) {
        langs.append(languages.values().at(i).at(0));
    }
    EncTtsSetting* setting =new EncTtsSetting(this,EncTtsSetting::eSTRINGLIST,
        tr("Language:"),RbSettings::subValue("sapi",RbSettings::TtsLanguage),
        langs);
    connect(setting,SIGNAL(dataChanged()),this,SLOT(updateVoiceList()));
    insertSetting(eLANGUAGE,setting);
    // voice
    setting = new EncTtsSetting(this,EncTtsSetting::eSTRINGLIST,
        tr("Voice:"),RbSettings::subValue("sapi",RbSettings::TtsVoice),
        getVoiceList(RbSettings::subValue("sapi",RbSettings::TtsLanguage).toString()),
        EncTtsSetting::eREFRESHBTN);
    connect(setting,SIGNAL(refresh()),this,SLOT(updateVoiceList()));
    insertSetting(eVOICE,setting);
    //speed
    int speed = RbSettings::subValue("sapi", RbSettings::TtsSpeed).toInt();
    if(speed > 10 || speed < -10)
        speed = 0;
    insertSetting(eSPEED, new EncTtsSetting(this, EncTtsSetting::eINT,
                tr("Speed:"), speed, -10, 10));
    // options
    insertSetting(eOPTIONS,new EncTtsSetting(this,EncTtsSetting::eSTRING,
        tr("Options:"),RbSettings::subValue("sapi",RbSettings::TtsOptions)));

}

void TTSSapi::saveSettings()
{
    //save settings in user config
    RbSettings::setSubValue("sapi",RbSettings::TtsLanguage,
            getSetting(eLANGUAGE)->current().toString());
    RbSettings::setSubValue("sapi",RbSettings::TtsVoice,
            getSetting(eVOICE)->current().toString());
    RbSettings::setSubValue("sapi",RbSettings::TtsSpeed,
            getSetting(eSPEED)->current().toInt());
    RbSettings::setSubValue("sapi",RbSettings::TtsOptions,
            getSetting(eOPTIONS)->current().toString());

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
        *errStr = tr("Could not copy the SAPI script");
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

    qDebug() << "[TTSSapi] Start:" << execstring;
    voicescript = new QProcess(NULL);
    //connect(voicescript,SIGNAL(readyReadStandardError()),this,SLOT(error()));
    voicescript->start(execstring);
    qDebug() << "[TTSSapi] wait for process";
    if(!voicescript->waitForStarted())
    {
        *errStr = tr("Could not start SAPI process");
        qDebug() << "[TTSSapi] starting process timed out!";
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

    m_started = true;
    return true;
}

QString TTSSapi::voiceVendor(void)
{
    bool keeprunning = m_started;
    QString vendor;
    if(!m_started) {
        QString error;
        start(&error);
    }
    *voicestream << "QUERY\tVENDOR\r\n";
    voicestream->flush();
    while((vendor = voicestream->readLine()).isEmpty())
            QCoreApplication::processEvents();

    qDebug() << "[TTSSAPI] TTS vendor:" << vendor;
    if(!keeprunning) {
        stop();
    }
    return vendor;
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

    qDebug() << "[TTSSapi] Start:" << execstring;
    voicescript = new QProcess(NULL);
    voicescript->start(execstring);
    qDebug() << "[TTSSapi] wait for process";
    if(!voicescript->waitForStarted()) {
        qDebug() << "[TTSSapi] process startup timed out!";
        return result;
    }
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
    QFile::setPermissions(QDir::tempPath() +"/sapi_voice.vbs",
              QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner
            | QFile::ReadUser  | QFile::WriteUser  | QFile::ExeUser
            | QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup
            | QFile::ReadOther | QFile::WriteOther | QFile::ExeOther );
    QFile::remove(QDir::tempPath() +"/sapi_voice.vbs");
    return result;
}



TTSStatus TTSSapi::voice(QString text,QString wavfile, QString *errStr)
{
    (void) errStr;
    QString query = "SPEAK\t"+wavfile+"\t"+text;
    qDebug() << "[TTSSapi] voicing" << query;
    // append newline to query. Done now to keep debug output more readable.
    query.append("\r\n");
    *voicestream << query;
    *voicestream << "SYNC\tbla\r\n";
    voicestream->flush();
    // do NOT poll the output with readLine(), this causes sync issues!
    voicescript->waitForReadyRead();

    if(!QFileInfo(wavfile).isFile()) {
        qDebug() << "[TTSSapi] output file does not exist:" << wavfile;
        return FatalError;
    }
    return NoError;
}

bool TTSSapi::stop()
{
    *voicestream << "QUIT\r\n";
    voicestream->flush();
    voicescript->waitForFinished();
    delete voicestream;
    delete voicescript;
    QFile::setPermissions(QDir::tempPath() +"/sapi_voice.vbs",
              QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner
            | QFile::ReadUser  | QFile::WriteUser  | QFile::ExeUser
            | QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup
            | QFile::ReadOther | QFile::WriteOther | QFile::ExeOther );
    QFile::remove(QDir::tempPath() +"/sapi_voice.vbs");
    m_started = false;
    return true;
}

bool TTSSapi::configOk()
{
    if(RbSettings::subValue("sapi",RbSettings::TtsVoice).toString().isEmpty())
        return false;
    return true;
}

