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
#include "systeminfo.h"

VoiceFileCreator::VoiceFileCreator(QObject* parent) :QObject(parent)
{
    m_wavtrimThreshold=500;
}

void VoiceFileCreator::abort()
{
    m_abort = true;
    emit aborted();
}

bool VoiceFileCreator::createVoiceFile()
{
    m_talkList.clear();
    m_abort = false;
    emit logItem(tr("Starting Voicefile generation"),LOGINFO);

    // test if tempdir exists
    if(!QDir(QDir::tempPath()+"/rbvoice/").exists())
    {
        QDir(QDir::tempPath()).mkdir("rbvoice");
    }
    m_path = QDir::tempPath() + "/rbvoice/";

    // read rockbox-info.txt
    RockboxInfo info(m_mountpoint);
    if(!info.success())
    {
        emit logItem(tr("could not find rockbox-info.txt"),LOGERROR);
        emit done(true);
        return false;
    }

    QString target = info.target();
    QString features = info.features();
    QString version = info.version();
    m_targetid = info.targetID().toInt();
    version = version.left(version.indexOf("-")).remove("r");
 
    //prepare download url
    QUrl genlangUrl = SystemInfo::value(SystemInfo::GenlangUrl).toString()
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
    connect(getter, SIGNAL(dataReadProgress(int, int)), this, SIGNAL(logProgress(int, int)));
    connect(this, SIGNAL(aborted()), getter, SLOT(abort()));
    emit logItem(tr("Downloading voice info..."),LOGINFO);
    getter->getFile(genlangUrl);
    return true;
 }


void VoiceFileCreator::downloadDone(bool error)
{
    qDebug() << "Voice creator::downloadDone, error:" << error;

    // update progress bar
    emit logProgress(1,1);
    if(getter->httpResponse() != 200 && !getter->isCached()) {
        emit logItem(tr("Download error: received HTTP error %1.").arg(getter->httpResponse()),LOGERROR);
        emit done(true);
        return;
    }
    
    if(getter->isCached()) 
        emit logItem(tr("Cached file used."), LOGINFO);
    if(error)
    {
        emit logItem(tr("Download error: %1").arg(getter->errorString()),LOGERROR);
        emit done(true);
        return;
    }
    else 
        emit logItem(tr("Download finished."),LOGINFO);
    
    QCoreApplication::processEvents();

    //open downloaded file
    QFile genlang(filename);
    if(!genlang.open(QIODevice::ReadOnly))
    {
        emit logItem(tr("failed to open downloaded file"),LOGERROR);
        emit done(true);
        return;
    }

    QCoreApplication::processEvents();

    //read in downloaded file
    emit logItem(tr("Reading strings..."),LOGINFO);
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
            TalkGenerator::TalkEntry entry;
            entry.toSpeak = voice;
            entry.wavfilename = m_path + "/" + id + ".wav";
            entry.talkfilename = m_path + "/" + id + ".mp3";   //voicefont wants them with .mp3 extension
            entry.voiced = false;
            entry.encoded = false;
            if(id == "VOICE_PAUSE")
            {
                QFile::copy(":/builtin/VOICE_PAUSE.wav",m_path + "/VOICE_PAUSE.wav");
                entry.wavfilename = m_path + "/VOICE_PAUSE.wav";
                entry.voiced = true;
            }
            m_talkList.append(entry);
            idfound=false;
            voicefound=false;
        }
    }
    genlang.close();

    // check for empty list
    if(m_talkList.size() == 0)
    {
        emit logItem(tr("The downloaded file was empty!"),LOGERROR);
        emit done(true);
        return;
    }

    // generate files
    {
        TalkGenerator generator(this);
        connect(&generator,SIGNAL(done(bool)),this,SIGNAL(done(bool)));
        connect(&generator,SIGNAL(logItem(QString,int)),this,SIGNAL(logItem(QString,int)));
        connect(&generator,SIGNAL(logProgress(int,int)),this,SIGNAL(logProgress(int,int)));
        connect(this,SIGNAL(aborted()),&generator,SLOT(abort()));
    
        if(generator.process(&m_talkList) == TalkGenerator::eERROR)
        {
            cleanup();
            emit logProgress(0,1);
            emit done(true);
            return;
        }
    }
    
    //make voicefile
    emit logItem(tr("Creating voicefiles..."),LOGINFO);
    FILE* ids2 = fopen(filename.toLocal8Bit(), "r");
    if (ids2 == NULL)
    {
        cleanup();
        emit logItem(tr("Error opening downloaded file"),LOGERROR);
        emit done(true);
        return;
    }

    FILE* output = fopen(QString(m_mountpoint + "/.rockbox/langs/" + m_lang
                + ".voice").toLocal8Bit(), "wb");
    if (output == NULL)
    {
        cleanup();
        fclose(ids2);
        emit logItem(tr("Error opening output file"),LOGERROR);
        emit done(true);
        return;
    }

    voicefont(ids2,m_targetid,m_path.toLocal8Bit().data(), output);
    // ids2 and output are closed by voicefont().

    //cleanup
    cleanup();

    // Add Voice file to the install log
    QSettings installlog(m_mountpoint + "/.rockbox/rbutil.log", QSettings::IniFormat, 0);
    installlog.beginGroup("selfcreated Voice");
    installlog.setValue("/.rockbox/langs/" + m_lang + ".voice",
            QDate::currentDate().toString("yyyyMMdd"));
    installlog.endGroup();
    installlog.sync();

    emit logProgress(1,1);
    emit logItem(tr("successfully created."),LOGOK);
    
    emit done(false);
}

//! \brief Cleans up Files potentially left in the temp dir
//!
void VoiceFileCreator::cleanup()
{
    emit logItem(tr("Cleaning up..."),LOGINFO);

    for(int i=0; i < m_talkList.size(); i++)
    {
        if(QFile::exists(m_talkList[i].wavfilename))
                QFile::remove(m_talkList[i].wavfilename);
        if(QFile::exists(m_talkList[i].talkfilename))
                QFile::remove(m_talkList[i].talkfilename);

        QCoreApplication::processEvents();
    }
    emit logItem(tr("Finished"),LOGINFO);
  
    return;
}

