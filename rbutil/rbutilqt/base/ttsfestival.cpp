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

#include <QtCore>
#include <QTcpSocket>

#include "ttsfestival.h"
#include "utils.h"
#include "rbsettings.h"

TTSFestival::~TTSFestival()
{
    qDebug() << "[Festival] Destroying instance";
    stop();
}

TTSBase::Capabilities TTSFestival::capabilities()
{
    return RunInParallel;
}

void TTSFestival::generateSettings()
{
    // server path
    QString exepath = RbSettings::subValue("festival-server",
                        RbSettings::TtsPath).toString();
    if(exepath == "" ) exepath = Utils::findExecutable("festival");
    insertSetting(eSERVERPATH,new EncTtsSetting(this,
                        EncTtsSetting::eSTRING, "Path to Festival server:",
                        exepath,EncTtsSetting::eBROWSEBTN));

    // client path
    QString clientpath = RbSettings::subValue("festival-client",
                        RbSettings::TtsPath).toString();
    if(clientpath == "" ) clientpath = Utils::findExecutable("festival_client");
    insertSetting(eCLIENTPATH,new EncTtsSetting(this,EncTtsSetting::eSTRING,
                        tr("Path to Festival client:"),
                        clientpath,EncTtsSetting::eBROWSEBTN));

    // voice
    EncTtsSetting* setting = new EncTtsSetting(this,
                        EncTtsSetting::eSTRINGLIST, tr("Voice:"),
                        RbSettings::subValue("festival", RbSettings::TtsVoice),
                        getVoiceList(), EncTtsSetting::eREFRESHBTN);
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
    RbSettings::setSubValue("festival-server",
            RbSettings::TtsPath,getSetting(eSERVERPATH)->current().toString());
    RbSettings::setSubValue("festival-client",
            RbSettings::TtsPath,getSetting(eCLIENTPATH)->current().toString());
    RbSettings::setSubValue("festival",
            RbSettings::TtsVoice,getSetting(eVOICE)->current().toString());

    RbSettings::sync();
}

void TTSFestival::updateVoiceDescription()
{
    // get voice Info with current voice and path
    currentPath = getSetting(eSERVERPATH)->current().toString();
    QString info = getVoiceInfo(getSetting(eVOICE)->current().toString());
    currentPath = "";
    
    getSetting(eVOICEDESC)->setCurrent(info);
}

void TTSFestival::clearVoiceDescription()
{
    getSetting(eVOICEDESC)->setCurrent("");
}

void TTSFestival::updateVoiceList()
{
   currentPath = getSetting(eSERVERPATH)->current().toString();
   QStringList voiceList = getVoiceList();
   currentPath = "";
   
   getSetting(eVOICE)->setList(voiceList);
   if(voiceList.size() > 0) getSetting(eVOICE)->setCurrent(voiceList.at(0));
   else getSetting(eVOICE)->setCurrent("");
}

void TTSFestival::startServer()
{
    if(!configOk())
        return;

    if(serverProcess.state() != QProcess::Running)
    {
        QString path;
        /* currentPath is set by the GUI - if it's set, it is the currently set
         path in the configuration GUI; if it's not set, use the saved path */
        if (currentPath == "")
            path = RbSettings::subValue("festival-server",RbSettings::TtsPath).toString();
        else
            path = currentPath;

        serverProcess.start(QString("%1 --server").arg(path));
        serverProcess.waitForStarted();

        /* A friendlier version of a spinlock */
        while (serverProcess.pid() == 0 && serverProcess.state() != QProcess::Running)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        if(serverProcess.state() == QProcess::Running)
            qDebug() << "[Festival] Server is up and running";
        else
            qDebug() << "[Festival] Server failed to start, state: " << serverProcess.state();
    }
}

bool TTSFestival::ensureServerRunning()
{
    if(serverProcess.state() != QProcess::Running)
    {
        startServer();
    }
    return serverProcess.state() == QProcess::Running;
}

bool TTSFestival::start(QString* errStr)
{
    qDebug() << "[Festival] Starting server with voice " << RbSettings::subValue("festival", RbSettings::TtsVoice).toString();
    
    bool running = ensureServerRunning();
    if (!RbSettings::subValue("festival",RbSettings::TtsVoice).toString().isEmpty())
    {
        /* There's no harm in using both methods to set the voice .. */
        QString voiceSelect = QString("(voice.select '%1)\n")
        .arg(RbSettings::subValue("festival", RbSettings::TtsVoice).toString());
        queryServer(voiceSelect, 3000);
        
        if(prologFile.open())
        {
          prologFile.write(voiceSelect.toAscii());
          prologFile.close();
          prologPath = QFileInfo(prologFile).absoluteFilePath();
          qDebug() << "[Festival] Prolog created at " << prologPath;
        }
        
    }
    
    if (!running)
      (*errStr) = "Festival could not be started";
    return running;
}

bool TTSFestival::stop()
{
    serverProcess.terminate();
    serverProcess.kill();

    return true;
}

TTSStatus TTSFestival::voice(QString text, QString wavfile, QString* errStr)
{
    qDebug() << "[Festival] Voicing " << text << "->" << wavfile;

    QString path = RbSettings::subValue("festival-client",
            RbSettings::TtsPath).toString();
    QString cmd = QString("%1 --server localhost --otype riff --ttw --withlisp"
            " --output \"%2\" --prolog \"%3\" - ").arg(path).arg(wavfile).arg(prologPath);
    qDebug() << "[Festival] Client cmd: " << cmd;

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
        qDebug() << "[Festival] Could not voice string: " << response;
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
    bool ret;
    if (currentPath == "")
    {
        QString serverPath = RbSettings::subValue("festival-server",
                                    RbSettings::TtsPath).toString();
        QString clientPath = RbSettings::subValue("festival-client",
                                    RbSettings::TtsPath).toString();

        ret = QFileInfo(serverPath).isExecutable() &&
            QFileInfo(clientPath).isExecutable();
        if(RbSettings::subValue("festival",RbSettings::TtsVoice).toString().size() > 0
                && voices.size() > 0)
            ret = ret && (voices.indexOf(RbSettings::subValue("festival",
                            RbSettings::TtsVoice).toString()) != -1);
    }
    else /* If we're currently configuring the server, we need to know that 
            the entered path is valid */
        ret = QFileInfo(currentPath).isExecutable();
    
    return ret;
}

QStringList TTSFestival::getVoiceList()
{
    if(!configOk())
        return QStringList();

    if(voices.size() > 0)
    {
        qDebug() << "[Festival] Using voice cache";
        return voices;
    }

    QString response = queryServer("(voice.list)", 10000);

    // get the 2nd line. It should be (<voice_name>, <voice_name>)
    response = response.mid(response.indexOf('\n') + 1, -1);
    response = response.left(response.indexOf('\n')).trimmed();

    voices = response.mid(1, response.size()-2).split(' ');

    voices.sort();
    if (voices.size() == 1 && voices[0].size() == 0)
        voices.removeAt(0);
    if (voices.size() > 0)
        qDebug() << "[Festival] Voices: " << voices;
    else
        qDebug() << "[Festival] No voices. Response was: " << response;

    return voices;
}

QString TTSFestival::getVoiceInfo(QString voice)
{
    if(!configOk())
        return "";

    if(!getVoiceList().contains(voice))
        return "";

    if(voiceDescriptions.contains(voice))
        return voiceDescriptions[voice];

    QString response = queryServer(QString("(voice.description '%1)").arg(voice),
                            10000);

    if (response == "")
    {
        voiceDescriptions[voice]=tr("No description available");
    }
    else
    {
        response = response.remove(QRegExp("(description \"*\")",
                    Qt::CaseInsensitive, QRegExp::Wildcard));
        qDebug() << "[Festival] voiceInfo w/o descr: " << response;
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
                // add a colon between the key and the value
                line = line.insert(firstSpace, ':');
                // capitalize the value
                line[firstSpace+2] = line[firstSpace+2].toUpper();
            }

            description += line + "\n";
        }
        voiceDescriptions[voice] = description.trimmed();
    }

    return voiceDescriptions[voice];
}

QString TTSFestival::queryServer(QString query, int timeout)
{
    if(!configOk())
        return "";

    // this operation could take some time
    emit busy();
    
    qDebug() << "[Festival] queryServer with " << query;

    if (!ensureServerRunning())
    {
      qDebug() << "[Festival] queryServer: ensureServerRunning failed";
      emit busyEnd();
      return "";
    }

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
        /* make sure we wait a little as we don't want to flood the server
         * with requests */
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
        lines.removeFirst(); /* should be LP */
        lines.removeLast();  /* should be ft_StUfF_keyOK */
    }
    else
        qDebug() << "[Festival] Response too short: " << response;

    emit busyEnd();
    return lines.join("\n");

}

