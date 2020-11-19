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
#include "Logger.h"

TTSSapi::TTSSapi(QObject* parent) : TTSBase(parent)
{
    m_TTSTemplate = "cscript //nologo \"%exe\" /language:%lang "
        "/voice:\"%voice\" /speed:%speed \"%options\"";
    m_TTSVoiceTemplate = "cscript //nologo \"%exe\" /language:%lang /listvoices";
    m_TTSType = "sapi";
    defaultLanguage = "english";
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
    for(int i = 0; i < languages.size(); ++i) {
        langs.append(languages.values().at(i).at(0));
    }
    EncTtsSetting* setting = new EncTtsSetting(this,
            EncTtsSetting::eSTRINGLIST, tr("Language:"),
            RbSettings::subValue(m_TTSType, RbSettings::TtsLanguage),
            langs);
    connect(setting,SIGNAL(dataChanged()),this,SLOT(updateVoiceList()));
    insertSetting(eLANGUAGE,setting);
    // voice
    setting = new EncTtsSetting(this,
            EncTtsSetting::eSTRINGLIST, tr("Voice:"),
            RbSettings::subValue(m_TTSType, RbSettings::TtsVoice),
            getVoiceList(RbSettings::subValue(m_TTSType,
                    RbSettings::TtsLanguage).toString()),
            EncTtsSetting::eREFRESHBTN);
    connect(setting,SIGNAL(refresh()),this,SLOT(updateVoiceList()));
    insertSetting(eVOICE,setting);
    //speed
    int speed = RbSettings::subValue(m_TTSType, RbSettings::TtsSpeed).toInt();
    if(speed > 10 || speed < -10)
        speed = 0;
    insertSetting(eSPEED, new EncTtsSetting(this,
                EncTtsSetting::eINT, tr("Speed:"), speed, -10, 10));
    // options
    insertSetting(eOPTIONS, new EncTtsSetting(this,
                EncTtsSetting::eSTRING, tr("Options:"),
                RbSettings::subValue(m_TTSType, RbSettings::TtsOptions)));

}

void TTSSapi::saveSettings()
{
    //save settings in user config
    RbSettings::setSubValue(m_TTSType, RbSettings::TtsLanguage,
            getSetting(eLANGUAGE)->current().toString());
    RbSettings::setSubValue(m_TTSType, RbSettings::TtsVoice,
            getSetting(eVOICE)->current().toString());
    RbSettings::setSubValue(m_TTSType, RbSettings::TtsSpeed,
            getSetting(eSPEED)->current().toInt());
    RbSettings::setSubValue(m_TTSType, RbSettings::TtsOptions,
            getSetting(eOPTIONS)->current().toString());

    RbSettings::sync();
}

void TTSSapi::updateVoiceList()
{
    LOG_INFO() << "updating voicelist";
    QStringList voiceList = getVoiceList(getSetting(eLANGUAGE)->current().toString());
    getSetting(eVOICE)->setList(voiceList);
    if(voiceList.size() > 0) getSetting(eVOICE)->setCurrent(voiceList.at(0));
    else getSetting(eVOICE)->setCurrent("");
}

bool TTSSapi::start(QString *errStr)
{

    m_TTSOpts = RbSettings::subValue(m_TTSType, RbSettings::TtsOptions).toString();
    m_TTSLanguage =RbSettings::subValue(m_TTSType, RbSettings::TtsLanguage).toString();
    m_TTSVoice=RbSettings::subValue(m_TTSType, RbSettings::TtsVoice).toString();
    m_TTSSpeed=RbSettings::subValue(m_TTSType, RbSettings::TtsSpeed).toString();

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

    LOG_INFO() << "Start:" << execstring;
    voicescript = new QProcess(nullptr);
    //connect(voicescript,SIGNAL(readyReadStandardError()),this,SLOT(error()));
    voicescript->start(execstring);
    LOG_INFO() << "wait for process";
    if(!voicescript->waitForStarted())
    {
        *errStr = tr("Could not start SAPI process");
        LOG_ERROR() << "starting process timed out!";
        return false;
    }

    if(!voicescript->waitForReadyRead(300))
    {
        *errStr = voicescript->readAllStandardError();
        if(*errStr != "")
            return false;
    }

    voicestream = new QTextStream(voicescript);
#if QT_VERSION < 0x060000
    voicestream->setCodec("UTF16-LE");
#else
    voicestream->setEncoding(QStringConverter::Utf16LE);
#endif

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

    LOG_INFO() << "TTS vendor:" << vendor;
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
    QString execstring = m_TTSVoiceTemplate;
    execstring.replace("%exe",m_TTSexec);
    execstring.replace("%lang",language);

    LOG_INFO() << "Start:" << execstring;
    voicescript = new QProcess(nullptr);
    voicescript->start(execstring);
    LOG_INFO() << "wait for process";
    if(!voicescript->waitForStarted()) {
        LOG_INFO() << "process startup timed out!";
        return result;
    }
    voicescript->closeWriteChannel();
    voicescript->waitForReadyRead();

    QString dataRaw = voicescript->readAllStandardError().data();
    if(dataRaw.startsWith("Error")) {
        LOG_INFO() << "Error:" << dataRaw;
    }
#if QT_VERSION >= 0x050e00
    result = dataRaw.split(";", Qt::SkipEmptyParts);
#else
    result = dataRaw.split(";", QString::SkipEmptyParts);
#endif
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
    LOG_INFO() << "voicing" << query;
    // append newline to query. Done now to keep debug output more readable.
    query.append("\r\n");
    *voicestream << query;
    *voicestream << "SYNC\tbla\r\n";
    voicestream->flush();
    // do NOT poll the output with readLine(), this causes sync issues!
    voicescript->waitForReadyRead();

    if(!QFileInfo(wavfile).isFile()) {
        LOG_ERROR() << "output file does not exist:" << wavfile;
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
    if(RbSettings::subValue(m_TTSType, RbSettings::TtsVoice).toString().isEmpty())
        return false;
    return true;
}

