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

#include "ttsfestival.h"
#include "utils.h"
#include "rbsettings.h"

TTSFestival::~TTSFestival()
{
    stop();
}

void TTSFestival::generateSettings()
{
    // server path
    QString exepath = RbSettings::subValue("festival-server",
                        RbSettings::TtsPath).toString();
    if(exepath == "" ) exepath = findExecutable("festival");
    insertSetting(eSERVERPATH,new EncTtsSetting(this,
                        EncTtsSetting::eSTRING, "Path to Festival server:",
                        exepath,EncTtsSetting::eBROWSEBTN));

    // client path
    QString clientpath = RbSettings::subValue("festival-client",
                        RbSettings::TtsPath).toString();
    if(clientpath == "" ) clientpath = findExecutable("festival_client");
    insertSetting(eCLIENTPATH,new EncTtsSetting(this,EncTtsSetting::eSTRING,
                        tr("Path to Festival client:"),
                        clientpath,EncTtsSetting::eBROWSEBTN));

    // voice
    EncTtsSetting* setting = new EncTtsSetting(this,
                        EncTtsSetting::eSTRINGLIST, tr("Voice:"),
                        RbSettings::subValue("festival", RbSettings::TtsVoice),
                        getVoiceList(exepath), EncTtsSetting::eREFRESHBTN);
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
    QString info = getVoiceInfo(getSetting(eVOICE)->current().toString(),
            getSetting(eSERVERPATH)->current().toString());
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

    QString path = RbSettings::subValue("festival-client",
            RbSettings::TtsPath).toString();
    QString cmd = QString("%1 --server localhost --otype riff --ttw --withlisp"
            " --output \"%2\" - ").arg(path).arg(wavfile);
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
    QString serverPath = RbSettings::subValue("festival-server",
                                RbSettings::TtsPath).toString();
    QString clientPath = RbSettings::subValue("festival-client",
                                RbSettings::TtsPath).toString();

    bool ret = QFileInfo(serverPath).isExecutable() &&
        QFileInfo(clientPath).isExecutable();
    if(RbSettings::subValue("festival",RbSettings::TtsVoice).toString().size() > 0
            && voices.size() > 0)
        ret = ret && (voices.indexOf(RbSettings::subValue("festival",
                        RbSettings::TtsVoice).toString()) != -1);
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

    QString response = queryServer(QString("(voice.description '%1)").arg(voice),
                            3000,path);

    if (response == "")
    {
        voiceDescriptions[voice]=tr("No description available");
    }
    else
    {
        response = response.remove(QRegExp("(description \"*\")",
                    Qt::CaseInsensitive, QRegExp::Wildcard));
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
        lines.removeFirst();
        lines.removeLast();
    }
    else
        qDebug() << "Response too short: " << response;

    emit busyEnd();
    return lines.join("\n");

}

