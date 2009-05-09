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

#include "voicefile.h"
#include "utils.h"
#include "rbsettings.h"

#define STATE_INVALID 0
#define STATE_PHRASE 1
#define STATE_VOICE 2


VoiceFileCreator::VoiceFileCreator(QObject* parent) :QObject(parent)
{
    m_wavtrimThreshold=500;
}

void VoiceFileCreator::abort()
{
    m_abort = true;
}

bool VoiceFileCreator::createVoiceFile(ProgressloggerInterface* logger)
{
    m_abort = false;
    m_logger = logger;
    m_logger->addItem(tr("Starting Voicefile generation"),LOGINFO);

    // test if tempdir exists
    if(!QDir(QDir::tempPath()+"/rbvoice/").exists())
    {
        QDir(QDir::tempPath()).mkdir("rbvoice");
    }

    m_path = QDir::tempPath() + "/rbvoice/";

    // read rockbox-info.txt
    RockboxInfo info(m_mountpoint);
    if(!info.open())
    {
        m_logger->addItem(tr("could not find rockbox-info.txt"),LOGERROR);
        m_logger->setFinished();
        emit done(false);
        return false;
    }

    QString target = info.target();
    QString features = info.features();
    QString version = info.version();
    version = version.left(version.indexOf("-")).remove(0,1);

    //prepare download url
    QUrl genlangUrl = RbSettings::value(RbSettings::GenlangUrl).toString()
            +"?lang=" + m_lang + "&t=" + target + "&rev=" + version + "&f=" + features;

    qDebug() << "downloading " << genlangUrl;

    //download the correct genlang output
    QTemporaryFile *downloadFile = new QTemporaryFile(this);
    downloadFile->open();
    filename = downloadFile->fileName();
    downloadFile->close();
    // get the real file.
    getter = new HttpGet(this);
    getter->setFile(downloadFile);

    connect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
    connect(getter, SIGNAL(dataReadProgress(int, int)), this, SLOT(updateDataReadProgress(int, int)));
    connect(m_logger, SIGNAL(aborted()), getter, SLOT(abort()));

    getter->getFile(genlangUrl);
    return true;
 }


void VoiceFileCreator::downloadDone(bool error)
{
    qDebug() << "Voice creator::downloadDone, error:" << error;

    // update progress bar
    int max = m_logger->getProgressMax();
    if(max == 0) {
        max = 100;
        m_logger->setProgressMax(max);
    }
    m_logger->setProgressValue(max);
    if(getter->httpResponse() != 200 && !getter->isCached()) {
        m_logger->addItem(tr("Download error: received HTTP error %1.").arg(getter->httpResponse()),LOGERROR);
        m_logger->setFinished();
        emit done(false);
        return;
    }
    if(getter->isCached()) m_logger->addItem(tr("Cached file used."), LOGINFO);
    if(error) {
        m_logger->addItem(tr("Download error: %1").arg(getter->errorString()),LOGERROR);
        m_logger->setFinished();
        emit done(false);
        return;
    }
    else m_logger->addItem(tr("Download finished."),LOGOK);
    QCoreApplication::processEvents();


    m_logger->setProgressMax(0);
    //open downloaded file
    QFile genlang(filename);
    if(!genlang.open(QIODevice::ReadOnly))
    {
        m_logger->addItem(tr("failed to open downloaded file"),LOGERROR);
        m_logger->setFinished();
        emit done(false);
        return;
    }

    //tts
    m_tts = TTSBase::getTTS(this,RbSettings::value(RbSettings::Tts).toString());

    QString errStr;
    if(!m_tts->start(&errStr))
    {
        m_logger->addItem(errStr,LOGERROR);
        m_logger->addItem(tr("Init of TTS engine failed"),LOGERROR);
        m_logger->setFinished();
        emit done(false);
        return;
    }

    // Encoder
    m_enc = EncBase::getEncoder(this,RbSettings::value(RbSettings::CurEncoder).toString());

    if(!m_enc->start())
    {
        m_logger->addItem(tr("Init of Encoder engine failed"),LOGERROR);
        m_tts->stop();
        m_logger->setFinished();
        emit done(false);
        return;
    }

    QCoreApplication::processEvents();
    connect(m_logger,SIGNAL(aborted()),this,SLOT(abort()));

    //read in downloaded file
    QList<QPair<QString,QString> > voicepairs;
    QTextStream in(&genlang);
    in.setCodec("UTF-8");
    QString id, voice;
    bool idfound = false;
    bool voicefound=false;
    while (!in.atEnd())
    {
        QString line = in.readLine();
        if(line.contains("id:"))  //ID found
        {
            id = line.remove("id:").remove('"').trimmed();
            idfound = true;
        }
        else if(line.contains("voice:"))  // voice found
        {
            voice = line.remove("voice:").remove('"').trimmed();
            voicefound=true;
        }

        if(idfound && voicefound)
        {
            voicepairs.append(QPair<QString,QString>(id,voice));
            idfound=false;
            voicefound=false;
        }
    }
    genlang.close();

    // check for empty list
    if(voicepairs.size() == 0)
    {
        m_logger->addItem(tr("The downloaded file was empty!"),LOGERROR);
        m_logger->setFinished();
        m_tts->stop();
        emit done(false);
        return;
    }

    m_logger->setProgressMax(voicepairs.size());
    m_logger->setProgressValue(0);

    // create voice clips
    QStringList mp3files;
    for(int i=0; i< voicepairs.size(); i++)
    {
        if(m_abort)
        {
            m_logger->addItem("aborted.",LOGERROR);
            m_logger->setFinished();
            m_tts->stop();
            emit done(false);
            return;
        }

        m_logger->setProgressValue(i);

        QString wavname = m_path + "/" + voicepairs.at(i).first + ".wav";
        QString toSpeak = voicepairs.at(i).second;
        QString encodedname = m_path + "/" + voicepairs.at(i).first +".mp3";

        // todo PAUSE
        if(voicepairs.at(i).first == "VOICE_PAUSE")
        {
            QFile::copy(":/builtin/builtin/VOICE_PAUSE.wav",m_path + "/VOICE_PAUSE.wav");

        }
        else
        {
            if(toSpeak == "") continue;

            m_logger->addItem(tr("creating ")+toSpeak,LOGINFO);
            QCoreApplication::processEvents();

            // TODO: add support for aborting the operation
            QString errStr;
            m_tts->voice(toSpeak,wavname, &errStr); // generate wav
        }

        // todo strip
        char buffer[255];

        wavtrim((char*)qPrintable(wavname),m_wavtrimThreshold,buffer,255);

        // encode wav
        m_enc->encode(wavname,encodedname);
        // remove the wav file
        QFile::remove(wavname);
        // remember the mp3 file for later removing
        mp3files << encodedname;
    }


    //make voicefile
    FILE* ids2 = fopen(filename.toUtf8(), "r");
    if (ids2 == NULL)
    {
        m_logger->addItem(tr("Error opening downloaded file"),LOGERROR);
        m_logger->setFinished();
        emit done(false);
        return;
    }

    FILE* output = fopen(QString(m_mountpoint + "/.rockbox/langs/" + m_lang + ".voice").toUtf8(), "wb");
    if (output == NULL)
    {
        m_logger->addItem(tr("Error opening output file"),LOGERROR);
        emit done(false);
        return;
    }

    voicefont(ids2,m_targetid,(char*)(const char*)m_path.toUtf8(), output);

    //remove .mp3 files
    for(int i=0;i< mp3files.size(); i++)
    {
        QFile::remove(mp3files.at(i));
    }

    // Add Voice file to the install log
    QSettings installlog(m_mountpoint + "/.rockbox/rbutil.log", QSettings::IniFormat, 0);
    installlog.beginGroup("selfcreated Voice");
    installlog.setValue("/.rockbox/langs/" + m_lang + ".voice",QDate::currentDate().toString("yyyyMMdd"));
    installlog.endGroup();
    installlog.sync();

    m_logger->setProgressMax(100);
    m_logger->setProgressValue(100);
    m_logger->addItem(tr("successfully created."),LOGOK);
    m_logger->setFinished();

    emit done(true);
}

void VoiceFileCreator::updateDataReadProgress(int read, int total)
{
    m_logger->setProgressMax(total);
    m_logger->setProgressValue(read);
    //qDebug() << "progress:" << read << "/" << total;

}

