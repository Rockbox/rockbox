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
#include "playerbuildinfo.h"
#include "Logger.h"

TTSSapi::TTSSapi(QObject* parent) : TTSBase(parent)
{
    m_TTSTemplate << "//nologo";
    m_TTSTemplate << "%exe";
    m_TTSTemplate << "/language:%lang";
    m_TTSTemplate << "/voice:%voice";
    m_TTSTemplate << "/speed:%speed";
    m_TTSTemplate << "%options";

    m_TTSVoiceTemplate << "//nologo";
    m_TTSVoiceTemplate << "%exe";
    m_TTSVoiceTemplate << "/language:%lang";
    m_TTSVoiceTemplate << "/listvoices";

    m_TTSType = "sapi";
    defaultLanguage = "english";
    voicescript = nullptr;
    voicestream = nullptr;
    m_started = false;
}

TTSBase::Capabilities TTSSapi::capabilities()
{
    return None;
}

void TTSSapi::generateSettings()
{
    // language
    QMap<QString, QVariant> langmap = PlayerBuildInfo::instance()->value(
                PlayerBuildInfo::LanguageList).toMap();
    EncTtsSetting* setting = new EncTtsSetting(this,
            EncTtsSetting::eSTRINGLIST, tr("Language:"),
            RbSettings::subValue(m_TTSType, RbSettings::TtsLanguage),
            langmap.keys());
    connect(setting, &EncTtsSetting::dataChanged, this, &TTSSapi::updateVoiceList);
    insertSetting(eLANGUAGE,setting);
    // voice
    setting = new EncTtsSetting(this,
            EncTtsSetting::eSTRINGLIST, tr("Voice:"),
            RbSettings::subValue(m_TTSType, RbSettings::TtsVoice),
            getVoiceList(RbSettings::subValue(m_TTSType,
                    RbSettings::TtsLanguage).toString()),
            EncTtsSetting::eREFRESHBTN);
    connect(setting, &EncTtsSetting::refresh, this, &TTSSapi::updateVoiceList);
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
    QStringList exec = m_TTSTemplate;
    exec.replaceInStrings("%exe",m_TTSexec);
    exec.replaceInStrings("%options",m_TTSOpts);
    exec.replaceInStrings("%lang",m_TTSLanguage);
    exec.replaceInStrings("%voice",m_TTSVoice);
    exec.replaceInStrings("%speed",m_TTSSpeed);

    LOG_INFO() << "Start: cscript " << exec;
    voicescript = new QProcess(nullptr);
    //connect(voicescript,SIGNAL(readyReadStandardError()),this,SLOT(error()));
    voicescript->start("cscript", exec);
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
    voicestream->setEncoding(QStringConverter::Utf16LE);

    m_started = true;
    return true;
}

QString TTSSapi::voiceVendor(void)
{
    bool keeprunning = m_started;
    QString vendor = "(unknown)";
    if(!m_started) {
        QString error;
        if(!start(&error)) {
            LOG_ERROR() << "could not start SAPI while querying vendor:" << error;
            return vendor;
        }
    }
    *voicestream << "QUERY\tVENDOR\r\n";
    voicestream->flush();
    if(voicescript->waitForReadyRead(5000)) {
        QString response = voicestream->readLine();
        if(!response.isEmpty())
            vendor = response;
    }
    else {
        LOG_ERROR() << "SAPI timed out while querying the voice vendor";
    }

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
    QStringList exec = m_TTSVoiceTemplate;
    exec.replaceInStrings("%exe",m_TTSexec);
    exec.replaceInStrings("%lang",language);

    LOG_INFO() << "Start: cscript " << exec;
    voicescript = new QProcess(nullptr);
    voicescript->start("cscript", exec);
    LOG_INFO() << "wait for process";
    if(!voicescript->waitForStarted()) {
        LOG_INFO() << "process startup timed out!";
        return result;
    }
    voicescript->closeWriteChannel();
    voicescript->waitForReadyRead();

    const QString dataRaw = voicescript->readAllStandardError().constData();
    if(dataRaw.startsWith("Error")) {
        LOG_INFO() << "Error:" << dataRaw;
    }
    result = dataRaw.split(";", Qt::SkipEmptyParts);
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



TTSStatus TTSSapi::voice(const QString& text, const QString& wavfile, QString *errStr)
{
    QString query = "SPEAK\t"+wavfile+"\t"+text;
    LOG_INFO() << "voicing" << query;
    // append newline to query. Done now to keep debug output more readable.
    query.append("\r\n");

    // Some third-party SAPI voices occasionally stop responding after many
    // consecutive requests. Restart the private cscript process and retry the
    // current string instead of discarding the entire voice-file operation.
    constexpr int maxAttempts = 3;
    constexpr int responseTimeout = 20000;
    for(int attempt = 1; attempt <= maxAttempts; ++attempt) {
        QFile::remove(wavfile);
        *voicestream << query;
        *voicestream << "SYNC\tbla\r\n";
        voicestream->flush();

        // Wait for the explicit SYNC reply. QProcess::waitForReadyRead() alone
        // is insufficient: the script can also write warnings, and older code
        // mistook those for completion before the wave file existed.
        QElapsedTimer timer;
        timer.start();
        bool synced = false;
        while(timer.elapsed() < responseTimeout) {
            int remaining = responseTimeout - static_cast<int>(timer.elapsed());
            if(!voicescript->waitForReadyRead(remaining))
                break;

            QString response = voicestream->readLine();
            if(response == "bla") {
                synced = true;
                break;
            }
            if(response.startsWith("ERROR\t")) {
                *errStr = response.mid(6);
                LOG_ERROR() << "SAPI error:" << *errStr;
                return FatalError;
            }
        }

        if(synced && QFileInfo(wavfile).isFile())
            return NoError;

        if(synced) {
            *errStr = tr("SAPI did not create the output wave file");
            LOG_ERROR() << "output file does not exist:" << wavfile;
            return FatalError;
        }

        LOG_WARNING() << "SAPI timed out on attempt" << attempt
                      << "of" << maxAttempts << "for" << text;
        if(attempt < maxAttempts) {
            stop();
            QString startError;
            if(!start(&startError)) {
                *errStr = tr("Could not restart SAPI after a timeout: %1")
                              .arg(startError);
                LOG_ERROR() << *errStr;
                return FatalError;
            }
        }
    }

    *errStr = tr("SAPI timed out repeatedly while generating speech");
    LOG_ERROR() << *errStr;
    return FatalError;
}

bool TTSSapi::stop()
{
    if(!m_started || voicescript == nullptr)
        return true;

    *voicestream << "QUIT\r\n";
    voicestream->flush();
    if(!voicescript->waitForFinished(5000)) {
        LOG_WARNING() << "SAPI process did not quit, terminating it";
        voicescript->terminate();
        if(!voicescript->waitForFinished(2000)) {
            LOG_WARNING() << "SAPI process did not terminate, killing it";
            voicescript->kill();
            voicescript->waitForFinished(2000);
        }
    }
    delete voicestream;
    delete voicescript;
    voicestream = nullptr;
    voicescript = nullptr;
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
