/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: voicefile.h 15932 2007-12-15 13:13:57Z domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "voicefile.h"

#define STATE_INVALID 0
#define STATE_PHRASE 1
#define STATE_VOICE 2


VoiceFileCreator::VoiceFileCreator(QObject* parent) :QObject(parent)
{

}

void VoiceFileCreator::abort()
{
    m_abort = true;
}

bool VoiceFileCreator::createVoiceFile(ProgressloggerInterface* logger)
{
    m_abort = false;
    m_logger = logger;
    m_logger->addItem("Starting Voicefile generation",LOGINFO);
    
    // test if tempdir exists
    if(!QDir(QDir::tempPath()+"/rbvoice/").exists())
    {
        QDir(QDir::tempPath()).mkdir("rbvoice");
    }
    
    m_path = QDir::tempPath() + "/rbvoice/";   
    
    // read rockbox-info.txt
    QFile info(m_mountpoint+"/.rockbox/rockbox-info.txt");
    if(!info.open(QIODevice::ReadOnly))
    {
        m_logger->addItem("failed to open rockbox-info.txt",LOGERROR);
        m_logger->abort();
        return false;
    }
    
    QString target, features,version;
    while (!info.atEnd()) {
        QString line = info.readLine();
        
        if(line.contains("Target:"))
        {
            target = line.remove("Target:").trimmed();
        }
        else if(line.contains("Features:"))
        {
            features = line.remove("Features:").trimmed();
        }
        else if(line.contains("Version:"))
        {
            version = line.remove("Version:").trimmed();
            version = version.left(version.indexOf("-")).remove(0,1);
        }        
    }
    info.close();
        
    //prepare download url
    QUrl genlangUrl = deviceSettings->value("genlang_url").toString() +"?lang=" +m_lang+"&t="+target+"&rev="+version+"&f="+features;
    
    qDebug() << "downloading " << genlangUrl;
    
    //download the correct genlang output   
    QTemporaryFile *downloadFile = new QTemporaryFile(this);
    downloadFile->open();
    filename = downloadFile->fileName();
    downloadFile->close();
    // get the real file.
    getter = new HttpGet(this);
    getter->setProxy(m_proxy);
    getter->setFile(downloadFile);
    getter->getFile(genlangUrl);

    connect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
    connect(getter, SIGNAL(downloadDone(int, bool)), this, SLOT(downloadRequestFinished(int, bool)));
    connect(getter, SIGNAL(dataReadProgress(int, int)), this, SLOT(updateDataReadProgress(int, int)));
    connect(m_logger, SIGNAL(aborted()), getter, SLOT(abort()));
    return true;
 }


void VoiceFileCreator::downloadRequestFinished(int id, bool error)
{
    qDebug() << "Install::downloadRequestFinished" << id << error;
    qDebug() << "error:" << getter->errorString();

    downloadDone(error);
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
        m_logger->abort();
        return;
    }
    if(getter->isCached()) m_logger->addItem(tr("Cached file used."), LOGINFO);
    if(error) {
        m_logger->addItem(tr("Download error: %1").arg(getter->errorString()),LOGERROR);
        m_logger->abort();
        return;
    }
    else m_logger->addItem(tr("Download finished."),LOGOK);
    QApplication::processEvents();
     
     
    m_logger->setProgressMax(0); 
    //open downloaded file
    QFile genlang(filename);
    if(!genlang.open(QIODevice::ReadOnly))
    {
        m_logger->addItem("failed to open downloaded file",LOGERROR);
        m_logger->abort();
        return;
    }    

    //tts
    m_tts = getTTS(userSettings->value("tts").toString());  
    m_tts->setUserCfg(userSettings);
    
    if(!m_tts->start())
    {
        m_logger->addItem("Init of TTS engine failed",LOGERROR);
        m_logger->abort();
        return;
    }

    // Encoder
    m_enc = getEncoder(userSettings->value("encoder").toString());  
    m_enc->setUserCfg(userSettings);
  
    if(!m_enc->start())
    {
        m_logger->addItem("Init of Encoder engine failed",LOGERROR);
        m_tts->stop();
        m_logger->abort();
        return;
    }

    QApplication::processEvents();
    
    connect(m_logger,SIGNAL(aborted()),this,SLOT(abort()));
    QStringList mp3files;
    
    QTextStream in(&genlang);
    in.setCodec("UTF-8");
    
    bool emptyfile = true;
    while (!in.atEnd()) 
    {
        if(m_abort)
        {
            m_logger->addItem("aborted.",LOGERROR);    
            break;
        }   

        QString comment = in.readLine();
        QString id = in.readLine();
        QString voice = in.readLine();    
        
        id = id.remove("id:").remove('"').trimmed();
        voice = voice.remove("voice:").remove('"').trimmed();
    
        QString wavname = m_path + "/" + id + ".wav";
        QString toSpeak = voice;
        QString encodedname = m_path + "/" + id +".mp3";
    
        // todo PAUSE
        if(id == "VOICE_PAUSE")
        {
            QFile::copy(":/builtin/builtin/VOICE_PAUSE.wav",m_path + "/VOICE_PAUSE.wav");
     
        }
        else
        {    
            if(voice == "") continue;
           
            m_logger->addItem(tr("creating ")+toSpeak,LOGINFO);    
            QApplication::processEvents();       
            m_tts->voice(toSpeak,wavname); // generate wav
        }
        
        // todo strip
        char buffer[255];
        
        wavtrim((char*)qPrintable(wavname),500,buffer,255);
        
        // encode wav 
        m_enc->encode(wavname,encodedname);
        // remove the wav file 
        QFile::remove(wavname);
        // remember the mp3 file for later removing
        mp3files << encodedname;
        // remember that we have done something
        emptyfile = false;
            
    }
    genlang.close();

    if(emptyfile)
    {
        m_logger->addItem(tr("The downloaded file was empty!"),LOGERROR);    
        m_logger->abort();
        return;
    }
    
    //make voicefile    
    FILE* ids2 = fopen(filename.toUtf8(), "r");
    if (ids2 == NULL)
    {
        m_logger->addItem("Error opening downloaded file",LOGERROR);    
        m_logger->abort();
        return;
    }

    FILE* output = fopen(QString(m_mountpoint + "/.rockbox/langs/" + m_lang + ".voice").toUtf8(), "wb");
    if (output == NULL)
    {
        m_logger->addItem("Error opening output file",LOGERROR);    
        return;
    }
    
    voicefont(ids2,m_targetid,(char*)(const char*)m_path.toUtf8(), output);
    
    //remove .mp3 files
    for(int i=0;i< mp3files.size(); i++)
    {
        QFile::remove(mp3files.at(i));
    }        
    
    m_logger->setProgressMax(100);
    m_logger->setProgressValue(100);
    m_logger->addItem("successfully created.",LOGOK);    
    m_logger->abort();    
}

void VoiceFileCreator::updateDataReadProgress(int read, int total)
{
    m_logger->setProgressMax(total);
    m_logger->setProgressValue(read);
    //qDebug() << "progress:" << read << "/" << total;

}

