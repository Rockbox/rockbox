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

#include "tts.h"
#include "utils.h"
#include "rbsettings.h"
/*********************************************************************
* TTS Base
**********************************************************************/
QMap<QString,QString> TTSBase::ttsList;

TTSBase::TTSBase(QObject* parent): EncTtsSettingInterface(parent)
{

}

// static functions
void TTSBase::initTTSList()
{
    ttsList["espeak"] = "Espeak TTS Engine";
    ttsList["flite"] = "Flite TTS Engine";
    ttsList["swift"] = "Swift TTS Engine";
#if defined(Q_OS_WIN)
    ttsList["sapi"] = "Sapi TTS Engine";
#endif
#if defined(Q_OS_LINUX)
    ttsList["festival"] = "Festival TTS Engine";
#endif
}

// function to get a specific encoder
TTSBase* TTSBase::getTTS(QObject* parent,QString ttsName)
{

    TTSBase* tts;
#if defined(Q_OS_WIN)
    if(ttsName == "sapi")
    {
        tts = new TTSSapi(parent);
        return tts;
    }
    else
#endif
#if defined(Q_OS_LINUX)
    if (ttsName == "festival")
    {
        tts = new TTSFestival(parent);
        return tts;
    }
    else
#endif
    if (true) // fix for OS other than WIN or LINUX
    {
        tts = new TTSExes(ttsName,parent);
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
* General TTS Exes
**********************************************************************/
TTSExes::TTSExes(QString name,QObject* parent) : TTSBase(parent)
{
    m_name = name;

    m_TemplateMap["espeak"] = "\"%exe\" %options -w \"%wavfile\" \"%text\"";
    m_TemplateMap["flite"] = "\"%exe\" %options -o \"%wavfile\" -t \"%text\"";
    m_TemplateMap["swift"] = "\"%exe\" %options -o \"%wavfile\" \"%text\"";

}

void TTSExes::generateSettings()
{
    QString exepath =RbSettings::subValue(m_name,RbSettings::TtsPath).toString();
    if(exepath == "") exepath = findExecutable(m_name);

    insertSetting(eEXEPATH,new EncTtsSetting(this,EncTtsSetting::eSTRING,
        tr("Path to TTS engine:"),exepath,EncTtsSetting::eBROWSEBTN));
    insertSetting(eOPTIONS,new EncTtsSetting(this,EncTtsSetting::eSTRING,
        tr("TTS engine options:"),RbSettings::subValue(m_name,RbSettings::TtsOptions)));
}

void TTSExes::saveSettings()
{
    RbSettings::setSubValue(m_name,RbSettings::TtsPath,getSetting(eEXEPATH)->current().toString());
    RbSettings::setSubValue(m_name,RbSettings::TtsOptions,getSetting(eOPTIONS)->current().toString());
    RbSettings::sync();
}

bool TTSExes::start(QString *errStr)
{
    m_TTSexec = RbSettings::subValue(m_name,RbSettings::TtsPath).toString();
    m_TTSOpts = RbSettings::subValue(m_name,RbSettings::TtsOptions).toString();

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

TTSStatus TTSExes::voice(QString text,QString wavfile, QString *errStr)
{
    (void) errStr;
    QString execstring = m_TTSTemplate;

    execstring.replace("%exe",m_TTSexec);
    execstring.replace("%options",m_TTSOpts);
    execstring.replace("%wavfile",wavfile);
    execstring.replace("%text",text);
    //qDebug() << "voicing" << execstring;
    QProcess::execute(execstring);
    return NoError;

}

bool TTSExes::configOk()
{
    QString path = RbSettings::subValue(m_name,RbSettings::TtsPath).toString();

    if (QFileInfo(path).exists())
        return true;

    return false;
}

/*********************************************************************
* TTS Sapi
**********************************************************************/
TTSSapi::TTSSapi(QObject* parent) : TTSBase(parent)
{
    m_TTSTemplate = "cscript //nologo \"%exe\" /language:%lang /voice:\"%voice\" /speed:%speed \"%options\"";
    defaultLanguage ="english";
    m_sapi4 =false;
}

void TTSSapi::generateSettings()
{
    // language
    QStringList languages = RbSettings::languages();
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
/**********************************************************************
 * TSSFestival - client-server wrapper
 **********************************************************************/
TTSFestival::~TTSFestival()
{
    stop();
}

void TTSFestival::generateSettings()
{
    // server path
    QString exepath = RbSettings::subValue("festival-server",RbSettings::TtsPath).toString();
    if(exepath == "" ) exepath = findExecutable("festival");
    insertSetting(eSERVERPATH,new EncTtsSetting(this,EncTtsSetting::eSTRING,"Path to Festival server:",exepath,EncTtsSetting::eBROWSEBTN));

    // client path
    QString clientpath = RbSettings::subValue("festival-client",RbSettings::TtsPath).toString();
    if(clientpath == "" ) clientpath = findExecutable("festival_client");
    insertSetting(eCLIENTPATH,new EncTtsSetting(this,EncTtsSetting::eSTRING,
        tr("Path to Festival client:"),clientpath,EncTtsSetting::eBROWSEBTN));

    // voice
    EncTtsSetting* setting = new EncTtsSetting(this,EncTtsSetting::eSTRINGLIST,
        tr("Voice:"),RbSettings::subValue("festival",RbSettings::TtsVoice),getVoiceList(exepath),EncTtsSetting::eREFRESHBTN);
    connect(setting,SIGNAL(refresh()),this,SLOT(updateVoiceList()));
    connect(setting,SIGNAL(dataChanged()),this,SLOT(clearVoiceDescription()));
    insertSetting(eVOICE,setting);

    //voice description
    setting = new EncTtsSetting(this,EncTtsSetting::eREADONLYSTRING,
        tr("Voice description:"),"",EncTtsSetting::eREFRESHBTN);
    connect(setting,SIGNAL(refresh()),this,SLOT(updateVoiceDescription()));
    insertSetting(eVOICEDESC,setting);
}

void TTSFestival::saveSettings()
{
    //save settings in user config
    RbSettings::setSubValue("festival-server",RbSettings::TtsPath,getSetting(eSERVERPATH)->current().toString());
    RbSettings::setSubValue("festival-client",RbSettings::TtsPath,getSetting(eCLIENTPATH)->current().toString());
    RbSettings::setSubValue("festival",RbSettings::TtsVoice,getSetting(eVOICE)->current().toString());

    RbSettings::sync();
}

void TTSFestival::updateVoiceDescription()
{
    // get voice Info with current voice and path
    QString info = getVoiceInfo(getSetting(eVOICE)->current().toString(),getSetting(eSERVERPATH)->current().toString());
    getSetting(eVOICEDESC)->setCurrent(info);
}

void TTSFestival::clearVoiceDescription()
{
    getSetting(eVOICEDESC)->setCurrent("");
}

void TTSFestival::updateVoiceList()
{
   QStringList voiceList = getVoiceList(getSetting(eSERVERPATH)->current().toString());
   getSetting(eVOICE)->setList(voiceList);
   if(voiceList.size() > 0) getSetting(eVOICE)->setCurrent(voiceList.at(0));
   else getSetting(eVOICE)->setCurrent("");
}

void TTSFestival::startServer(QString path)
{
    if(!configOk())
        return;

    if(path == "")
        path = RbSettings::subValue("festival-server",RbSettings::TtsPath).toString();

    serverProcess.start(QString("%1 --server").arg(path));
    serverProcess.waitForStarted();

    queryServer("(getpid)",300,path);
    if(serverProcess.state() == QProcess::Running)
        qDebug() << "Festival is up and running";
    else
        qDebug() << "Festival failed to start";
}

void TTSFestival::ensureServerRunning(QString path)
{
    if(serverProcess.state() != QProcess::Running)
    {
        startServer(path);
    }
}

bool TTSFestival::start(QString* errStr)
{
    (void) errStr;
    ensureServerRunning();
    if (!RbSettings::subValue("festival",RbSettings::TtsVoice).toString().isEmpty())
        queryServer(QString("(voice.select '%1)")
        .arg(RbSettings::subValue("festival", RbSettings::TtsVoice).toString()));

    return true;
}

bool TTSFestival::stop()
{
    serverProcess.terminate();
    serverProcess.kill();

    return true;
}

TTSStatus TTSFestival::voice(QString text, QString wavfile, QString* errStr)
{
    qDebug() << text << "->" << wavfile;

    QString path = RbSettings::subValue("festival-client",RbSettings::TtsPath).toString();
    QString cmd = QString("%1 --server localhost --otype riff --ttw --withlisp --output \"%2\" - ").arg(path).arg(wavfile);
    qDebug() << cmd;

    QProcess clientProcess;
    clientProcess.start(cmd);
    clientProcess.write(QString("%1.\n").arg(text).toAscii());
    clientProcess.waitForBytesWritten();
    clientProcess.closeWriteChannel();
    clientProcess.waitForReadyRead();
    QString response = clientProcess.readAll();
    response = response.trimmed();
    if(!response.contains("Utterance"))
    {
        qDebug() << "Could not voice string: " << response;
        *errStr = tr("engine could not voice string");
        return Warning;
        /* do not stop the voicing process because of a single string
        TODO: needs proper settings */
    }
    clientProcess.closeReadChannel(QProcess::StandardError);
    clientProcess.closeReadChannel(QProcess::StandardOutput);
    clientProcess.terminate();
    clientProcess.kill();

    return NoError;
}

bool TTSFestival::configOk()
{
    QString serverPath = RbSettings::subValue("festival-server",RbSettings::TtsPath).toString();
    QString clientPath = RbSettings::subValue("festival-client",RbSettings::TtsPath).toString();

    bool ret = QFileInfo(serverPath).isExecutable() &&
        QFileInfo(clientPath).isExecutable();
    if(RbSettings::subValue("festival",RbSettings::TtsVoice).toString().size() > 0 && voices.size() > 0)
        ret = ret && (voices.indexOf(RbSettings::subValue("festival",RbSettings::TtsVoice).toString()) != -1);
    return ret;
}

QStringList TTSFestival::getVoiceList(QString path)
{
    if(!configOk())
        return QStringList();

    if(voices.size() > 0)
    {
        qDebug() << "Using voice cache";
        return voices;
    }

    QString response = queryServer("(voice.list)",3000,path);

    // get the 2nd line. It should be (<voice_name>, <voice_name>)
    response = response.mid(response.indexOf('\n') + 1, -1);
    response = response.left(response.indexOf('\n')).trimmed();

    voices = response.mid(1, response.size()-2).split(' ');

    voices.sort();
    if (voices.size() == 1 && voices[0].size() == 0)
        voices.removeAt(0);
    if (voices.size() > 0)
        qDebug() << "Voices: " << voices;
    else
        qDebug() << "No voices.";

    return voices;
}

QString TTSFestival::getVoiceInfo(QString voice,QString path)
{
    if(!configOk())
        return "";

    if(!getVoiceList().contains(voice))
        return "";

    if(voiceDescriptions.contains(voice))
        return voiceDescriptions[voice];

    QString response = queryServer(QString("(voice.description '%1)").arg(voice), 3000,path);

    if (response == "")
    {
        voiceDescriptions[voice]=tr("No description available");
    }
    else
    {
        response = response.remove(QRegExp("(description \"*\")", Qt::CaseInsensitive, QRegExp::Wildcard));
        qDebug() << "voiceInfo w/o descr: " << response;
        response = response.remove(')');
        QStringList responseLines = response.split('(', QString::SkipEmptyParts);
        responseLines.removeAt(0); // the voice name itself

        QString description;
        foreach(QString line, responseLines)
        {
            line = line.remove('(');
            line = line.simplified();

            line[0] = line[0].toUpper(); // capitalize the key

            int firstSpace = line.indexOf(' ');
            if (firstSpace > 0)
            {
                line = line.insert(firstSpace, ':'); // add a colon between the key and the value
                line[firstSpace+2] = line[firstSpace+2].toUpper(); // capitalize the value
            }

            description += line + "\n";
        }
        voiceDescriptions[voice] = description.trimmed();
    }

    return voiceDescriptions[voice];
}

QString TTSFestival::queryServer(QString query, int timeout,QString path)
{
    if(!configOk())
        return "";

    // this operation could take some time
    emit busy();

    ensureServerRunning(path);

    qDebug() << "queryServer with " << query;
    QString response;

    QDateTime endTime;
    if(timeout > 0)
        endTime = QDateTime::currentDateTime().addMSecs(timeout);

    /* Festival is *extremely* unreliable. Although at this
     * point we are sure that SIOD is accepting commands,
     * we might end up with an empty response. Hence, the loop.
     */
    while(true)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QTcpSocket socket;

        socket.connectToHost("localhost", 1314);
        socket.waitForConnected();

        if(socket.state() == QAbstractSocket::ConnectedState)
        {
            socket.write(QString("%1\n").arg(query).toAscii());
            socket.waitForBytesWritten();
            socket.waitForReadyRead();

            response = socket.readAll().trimmed();

            if (response != "LP" && response != "")
                break;
        }
        socket.abort();
        socket.disconnectFromHost();

        if(timeout > 0 && QDateTime::currentDateTime() >= endTime)
        {
            emit busyEnd();
            return "";
        }
        /* make sure we wait a little as we don't want to flood the server with requests */
        QDateTime tmpEndTime = QDateTime::currentDateTime().addMSecs(500);
        while(QDateTime::currentDateTime() < tmpEndTime)
            QCoreApplication::processEvents(QEventLoop::AllEvents);
    }
    if(response == "nil")
    {
        emit busyEnd();
        return "";
    }

    QStringList lines = response.split('\n');
    if(lines.size() > 2)
    {
        lines.removeFirst();
        lines.removeLast();
    }
    else
        qDebug() << "Response too short: " << response;

    emit busyEnd();
    return lines.join("\n");

}

