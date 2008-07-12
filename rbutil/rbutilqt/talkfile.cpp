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

//! \brief Creates Talkfiles. 
//!
//! \param logger A pointer to a Loggerobject
bool TalkFileCreator::createTalkFiles(ProgressloggerInterface* logger)
{
    m_abort = false;
    m_logger = logger;
    
    QMultiMap<QString,QString> fileList;
    QMultiMap<QString,QString> dirList;
    QStringList toSpeakList;
    QString errStr;
    
    m_logger->addItem(tr("Starting Talk file generation"),LOGINFO);
    
    //tts
    m_tts = TTSBase::getTTS(settings->curTTS());  
    m_tts->setCfg(settings);
    
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
         
    // read in Maps of paths - file/dirnames
    m_logger->addItem(tr("Reading Filelist..."),LOGINFO);
    if(createDirAndFileMaps(m_dir,&dirList,&fileList) == false)
    {
        m_logger->addItem(tr("Talk file creation aborted"),LOGERROR);
        doAbort(toSpeakList);
        return false;
    }
    
    // create List of all Files/Dirs to speak
    QMapIterator<QString, QString> dirIt(dirList);
    while (dirIt.hasNext()) 
    {
        dirIt.next();
        // insert only non dublicate dir entrys into list
        if(!toSpeakList.contains(dirIt.value()))
        {
            qDebug() << "toSpeaklist dir:" << dirIt.value();
            toSpeakList.append(dirIt.value()); 
        }
    }
    QMapIterator<QString, QString> fileIt(fileList);
    while (fileIt.hasNext()) 
    {
        fileIt.next();
        // insert only non- dublictae file entrys into list
        if(!toSpeakList.contains(fileIt.value()))
        {
            if(m_stripExtensions)
                toSpeakList.append(stripExtension(fileIt.value()));
            else
                toSpeakList.append(fileIt.value()); 
        }
    }
    
    // Voice entrys
    m_logger->addItem(tr("Voicing entrys..."),LOGINFO);
    if(voiceList(toSpeakList,&errStr) == false)
    {
        m_logger->addItem(errStr,LOGERROR);
        doAbort(toSpeakList);
        return false;
    }
    
    // Encoding Entrys
    m_logger->addItem(tr("Encoding files..."),LOGINFO);
    if(encodeList(toSpeakList,&errStr) == false)
    {
        m_logger->addItem(errStr,LOGERROR);
        doAbort(toSpeakList);
        return false;
    }
        
    // Copying talk files    
    m_logger->addItem(tr("Copying Talkfile for Dirs..."),LOGINFO);
    if(copyTalkDirFiles(dirList,&errStr) == false)
    {
        m_logger->addItem(errStr,LOGERROR);
        doAbort(toSpeakList);    
        return false;
    }
        
    //Copying file talk files
    m_logger->addItem(tr("Copying Talkfile for Files..."),LOGINFO);
    if(copyTalkFileFiles(fileList,&errStr) == false)
    {
        m_logger->addItem(errStr,LOGERROR);
        doAbort(toSpeakList);    
        return false;
    }
     
    // Deleting left overs
    if( !cleanup(toSpeakList))
        return false;

    m_tts->stop();
    m_enc->stop();
    m_logger->addItem(tr("Finished creating Talk files"),LOGOK);
    m_logger->setProgressMax(1);
    m_logger->setProgressValue(1);
    m_logger->abort();

    return true;
}

//! \brief resets the internal progress counter, and sets the Progressbar in the Logger
//!
//! \param max  The maximum to shich the Progressbar is set.
void TalkFileCreator::resetProgress(int max)
{
    m_progress = 0;
    m_logger->setProgressMax(max);
    m_logger->setProgressValue(m_progress);
}

//! \brief Strips everything after and including the last dot in a string. If there is no dot, nothing is changed
//!
//! \param filename The filename from which to strip the Extension
//! \returns the modified string
QString TalkFileCreator::stripExtension(QString filename)
{
    if(filename.lastIndexOf(".") != -1)
        return filename.left(filename.lastIndexOf("."));
    else
        return filename;
}

//! \brief Does needed Tasks when we need to abort. Cleans up Files. Stops the Logger, Stops TTS and Encoder
//!
//! \param cleanupList  List of filenames to give to cleanup()
void TalkFileCreator::doAbort(QStringList cleanupList)
{
    cleanup(cleanupList);
    m_logger->setProgressMax(1);
    m_logger->setProgressValue(0);
    m_logger->abort();
    m_tts->stop();
    m_enc->stop();    
}

//! \brief Creates MultiMaps (paths -> File/dir names)  of all Dirs and Files in a Folder.
//!  Depending on settings, either Dirs  or Files can be ignored.
//! Also recursion is controlled by settings
//!
//! \param startDir     The dir where it beginns scanning
//! \param dirMap       The MulitMap where the dirs are stored
//! \param filMap       The MultiMap where Files are stored
//! \returns true on Success, false if User aborted.  
bool TalkFileCreator::createDirAndFileMaps(QDir startDir,QMultiMap<QString,QString> *dirMap,QMultiMap<QString,QString> *fileMap)
{
    // create Iterator
    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if(m_recursive)
        flags = QDirIterator::Subdirectories;
        
    QDirIterator it(startDir,flags);
    
    // read in Maps of paths - file/dirnames
    while (it.hasNext())
    {
        it.next();
        if(m_abort)
        {
            return false;
        }
        
        QFileInfo fileInf = it.fileInfo();
        
        // its a dir
        if(fileInf.isDir())
        {
            QDir dir = fileInf.dir();
            
            // insert into List
            if(!dir.dirName().isEmpty() && m_talkFolders)
            {
                qDebug() << "Dir: " << dir.dirName() << " - " << dir.path();
                dirMap->insert(dir.path(),dir.dirName());
            }
        }
        else  // its a File
        {
            // insert into List
            if( !fileInf.fileName().isEmpty() && !fileInf.fileName().endsWith(".talk") && m_talkFiles)
            {
                qDebug() << "File: " << fileInf.fileName() << " - " << fileInf.path();
                fileMap->insert(fileInf.path(),fileInf.fileName());
            }
        }
        QCoreApplication::processEvents();
    }
    return true;    
}

//! \brief Voices a List of string to the temp dir. Progress is handled inside.
//!
//!  \param toSpeak QStringList with the Entrys to voice.
//! \param errString pointer to where the Error cause is written
//! \returns true on success, false on error or user abort
bool TalkFileCreator::voiceList(QStringList toSpeak,QString* errString)
{
    resetProgress(toSpeak.size());

    for(int i=0; i < toSpeak.size(); i++)
    {
        if(m_abort)
        {
            *errString = tr("Talk file creation aborted");
            return false;
        }
        
        QString filename = QDir::tempPath()+ "/"+  toSpeak[i] + ".wav";
                
        if(!m_tts->voice(toSpeak[i],filename))
        {
            *errString =tr("Voicing of %s failed").arg(toSpeak[i]);
            return false;
        }       
        m_logger->setProgressValue(++m_progress);
        QCoreApplication::processEvents();
    }
    return true;
}


//! \brief Encodes a List of strings from/to the temp dir. Progress is handled inside.
//! It expects the inputfile in the temp dir with the name in the List appended with ".wav" 
//!
//!  \param toSpeak QStringList with the Entrys to encode.
//! \param errString pointer to where the Error cause is written
//! \returns true on success, false on error or user abort
bool TalkFileCreator::encodeList(QStringList toEncode,QString* errString)
{
    resetProgress(toEncode.size());
    for(int i=0; i < toEncode.size(); i++)
    {
        if(m_abort)
        {
            *errString = tr("Talk file creation aborted");
            return false;
        }
        
        QString wavfilename = QDir::tempPath()+ "/"+   toEncode[i] + ".wav";
        QString filename = QDir::tempPath()+ "/"+  toEncode[i] + ".talk";
        
        if(!m_enc->encode(wavfilename,filename))
        {
            *errString =tr("Encoding of %1 failed").arg(filename);
            return false;
        }    
        m_logger->setProgressValue(++m_progress);
        QCoreApplication::processEvents();        
    }
    return true;
}

//! \brief copys Talkfile for Dirs from the temp dir to the target. Progress and installlog is handled inside
//!
//! \param dirMap   a MultiMap of Paths -> Dirnames
//! \param errString Pointer to a QString where the error cause is written.
//! \returns true on success, false on error or user abort 
bool TalkFileCreator::copyTalkDirFiles(QMultiMap<QString,QString> dirMap,QString* errString)
{
    resetProgress(dirMap.size());
    
    QSettings installlog(m_mountpoint + "/.rockbox/rbutil.log", QSettings::IniFormat, 0);
    installlog.beginGroup("talkfiles");
    
    QMapIterator<QString, QString> it(dirMap);
    while (it.hasNext()) 
    {
        it.next();
        if(m_abort)
        {
            *errString = tr("Talk file creation aborted");
            return false;
        }
                
        QString source = QDir::tempPath()+ "/"+  it.value() + ".talk";
        QString target = it.key() + "/" + "_dirname.talk";

         // remove target if it exists, and if we should overwrite it
        if(m_overwriteTalk && QFile::exists(target))
            QFile::remove(target);
        
        // copying
        if(!QFile::copy(source,target))
        {
            *errString = tr("Copying of %1 to %2 failed").arg(source).arg(target);
            return false;
        } 
        
        // add to installlog
        QString now = QDate::currentDate().toString("yyyyMMdd");
        installlog.setValue(target.remove(0,m_mountpoint.length()),now);
       
        m_logger->setProgressValue(++m_progress);
        QCoreApplication::processEvents();
    }
    installlog.endGroup();
    installlog.sync(); 
    return true;  
}

//! \brief copys Talkfile for Files from the temp dir to the target. Progress and installlog is handled inside
//!
//! \param fileMap   a MultiMap of Paths -> Filenames
//! \param errString Pointer to a QString where the error cause is written.
//! \returns true on success, false on error or user abort 
bool TalkFileCreator::copyTalkFileFiles(QMultiMap<QString,QString> fileMap,QString* errString)
{
    resetProgress(fileMap.size());
    
    QSettings installlog(m_mountpoint + "/.rockbox/rbutil.log", QSettings::IniFormat, 0);
    installlog.beginGroup("talkfiles");
    
    QMapIterator<QString, QString> it(fileMap);
    while (it.hasNext()) 
    {
        it.next();
        if(m_abort)
        {
            *errString = tr("Talk file creation aborted");
            return false;
        }
                
        QString source;
        QString target = it.key() + "/" + it.value() + ".talk";
        
        // correct source if we hav stripExtension enabled        
        if(m_stripExtensions)
            source = QDir::tempPath()+ "/"+ stripExtension(it.value()) + ".talk"; 
        else
            source = QDir::tempPath()+ "/"+  it.value() + ".talk";
        
        // remove target if it exists, and if we should overwrite it
        if(m_overwriteTalk && QFile::exists(target))
            QFile::remove(target);
        
        // copy file
        qDebug() << "copying: " << source << " to " << target;
        if(!QFile::copy(source,target))
        {
            *errString = tr("Copying of %1 to %2 failed").arg(source).arg(target);
            return false;
        }    
        
        //  add to Install log
        QString now = QDate::currentDate().toString("yyyyMMdd");
        installlog.setValue(target.remove(0,m_mountpoint.length()),now);
        
        m_logger->setProgressValue(++m_progress);
        QCoreApplication::processEvents();
    }
    installlog.endGroup();
    installlog.sync(); 
    return true;  
}


//! \brief Cleans up Files potentially left in the temp dir
//! 
//! \param list List of file to try to delete in the temp dir. Function appends ".wav" and ".talk" to the filenames
bool TalkFileCreator::cleanup(QStringList list)
{
    m_logger->addItem(tr("Cleaning up.."),LOGINFO);
        
    for(int i=0; i < list.size(); i++)
    {            
        if(QFile::exists(QDir::tempPath()+ "/"+  list[i] + ".wav"))
                QFile::remove(QDir::tempPath()+ "/"+  list[i] + ".wav");
        if(QFile::exists(QDir::tempPath()+ "/"+  list[i] + ".talk"))
                QFile::remove(QDir::tempPath()+ "/"+  list[i] + ".talk");
        
        QCoreApplication::processEvents();
    }
    return true;
}

//! \brief slot, which is connected to the abort of the Logger. Sets a flag, so Creating Talkfiles ends at the next possible position
//!
void TalkFileCreator::abort()
{
    m_abort = true;
}

