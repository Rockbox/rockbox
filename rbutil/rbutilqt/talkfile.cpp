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

#include "talkfile.h"

TalkFileCreator::TalkFileCreator(QObject* parent): QObject(parent)
{

}

bool TalkFileCreator::createTalkFiles(ProgressloggerInterface* logger)
{
    m_abort = false;
    m_logger = logger;
    m_logger->addItem(tr("Starting Talk file generation"),LOGINFO);
    
    //tts
    m_tts = TTSBase::getTTS(settings->curTTS());  
    m_tts->setCfg(settings);
    
    QString errStr;
    if(!m_tts->start(&errStr))
    {
        m_logger->addItem(errStr.trimmed(),LOGERROR);
        m_logger->addItem(tr("Init of TTS engine failed"),LOGERROR);
        m_logger->abort();
        return false;
    }

    // Encoder
    m_enc = EncBase::getEncoder(settings->curEncoder());  
    m_enc->setCfg(settings);
  
    if(!m_enc->start())
    {
        m_logger->addItem(tr("Init of Encoder engine failed"),LOGERROR);
        m_logger->abort();
        m_tts->stop();
        return false;
    }

    QCoreApplication::processEvents();

    connect(logger,SIGNAL(aborted()),this,SLOT(abort()));
    m_logger->setProgressMax(0);
  
    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if(m_recursive)
        flags = QDirIterator::Subdirectories;
        
    QDirIterator it(m_dir,flags);
    QSettings installlog(m_mountpoint + "/.rockbox/rbutil.log", QSettings::IniFormat, 0);
    installlog.beginGroup("talkfiles");
    // iterate over all entrys
    while (it.hasNext())
    {
        if(m_abort)
        {
            m_logger->addItem(tr("Talk file creation aborted"),LOGERROR);
            m_logger->abort();
            m_tts->stop();
            return false;
        }

        QCoreApplication::processEvents();
        QFileInfo fileInf = it.fileInfo();
        QString toSpeak;
        QString filename;
        QString wavfilename;
        QString filepath;

        QString path = fileInf.filePath(); 
        qDebug() << path;

        if( path.endsWith("..") || path.endsWith(".") || path.endsWith(".talk") )
        {
            it.next();
            continue;
        }

        //! if it is a dir
        if(fileInf.isDir())  
        {
            // skip entry if folder talking isnt enabled
            if(m_talkFolders == false) 
            {
              it.next();
              continue;
            }
            
            toSpeak = fileInf.fileName();
            
            filepath = fileInf.filePath() + "/";
            filename = "_dirname.talk";
            qDebug() << "toSpeak: " << toSpeak << "filename: " << filename << " path: " <<filepath; 
        }
        else   // if it is a file
        {
            // skip entry if file talking isnt enabled
            if(m_talkFiles == false)
            {
               it.next();
               continue;
            }
                   
            // create toSpeak string
            if(m_stripExtensions)
                toSpeak = fileInf.baseName();
            else
                toSpeak = fileInf.fileName();
            // create filename and path
            filepath = fileInf.absolutePath();
            filename = fileInf.fileName() + ".talk";
            
        }
        
        wavfilename = QDir::tempPath()+ "/"+ filename + ".wav";

        QFileInfo filenameInf(filepath+filename);
        QFileInfo wavfilenameInf(wavfilename);

        //! the actual generation of the .talk files
        if(!filenameInf.exists() || m_overwriteTalk)
        {
            if(!wavfilenameInf.exists() || m_overwriteWav)
            {
                m_logger->addItem(tr("Voicing of %1").arg(toSpeak),LOGINFO);
                if(!m_tts->voice(toSpeak,wavfilename))
                {
                    m_logger->addItem(tr("Voicing of %s failed").arg(toSpeak),LOGERROR);
                    m_logger->abort();
                    m_tts->stop();
                    m_enc->stop();

                    return false;
                }
                QCoreApplication::processEvents();
            }
            m_logger->addItem(tr("Encoding of %1").arg(toSpeak),LOGINFO);
            if(!m_enc->encode(wavfilename,filepath+filename))
            {
                m_logger->addItem(tr("Encoding of %1 failed").arg(wavfilename),LOGERROR);
                m_logger->abort();
                m_tts->stop();
                m_enc->stop();

                return false;
            }
            QCoreApplication::processEvents();
        }
        
        //! remove the intermedia wav file, if requested
        QString now = QDate::currentDate().toString("yyyyMMdd");
        if(m_removeWav)
        {
            QFile wavfile(wavfilename);
            wavfile.remove();
            installlog.remove(wavfilename);
        }
        else
            installlog.setValue(wavfilename.remove(0,m_mountpoint.length()),now);
        
        //! add the .talk file to the install log
        installlog.setValue(QString(filepath+filename).remove(0,m_mountpoint.length()),now);
        it.next();
    }

    installlog.endGroup();
    m_tts->stop();
    m_logger->addItem(tr("Finished creating Talk files"),LOGOK);
    m_logger->setProgressMax(1);
    m_logger->setProgressValue(1);
    m_logger->abort();

    return true;

}

void TalkFileCreator::abort()
{
    m_abort = true;
}

