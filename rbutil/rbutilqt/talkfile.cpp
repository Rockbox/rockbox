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


bool TalkFileCreator::initEncoder()
{
    QFileInfo enc(m_EncExec);
    if(enc.exists())
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool TalkFileCreator::initTTS()
{
    QFileInfo tts(m_TTSexec);
    
    if(tts.exists())
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool TalkFileCreator::createTalkFiles(ProgressloggerInterface* logger)
{
    m_abort = false;
    m_logger = logger;
    m_logger->addItem("Starting Talkfile generation",LOGINFO);
    if(!initTTS())
    {
        m_logger->addItem("Init of TTS engine failed",LOGERROR);
        return false;
    }
    if(!initEncoder())
    {
        m_logger->addItem("Init of encoder failed",LOGERROR);
        return false;
    }
    QApplication::processEvents();
    
    connect(logger,SIGNAL(aborted()),this,SLOT(abort()));
    m_logger->setProgressMax(0);
    QDirIterator it(m_dir,QDirIterator::Subdirectories);
    QSettings installlog(m_mountpoint + "/.rockbox/rbutil.log", QSettings::IniFormat, 0);
    installlog.beginGroup("talkfiles");
    // iterate over all entrys
    while (it.hasNext()) 
    {
        if(m_abort)
        {
            m_logger->addItem("Talkfile creation aborted",LOGERROR);
            return false;
        }

        QApplication::processEvents();
        QFileInfo fileInf = it.fileInfo();
        QString toSpeak;
        QString filename;
        QString wavfilename;
        
        if(fileInf.fileName() == "." || fileInf.fileName() == ".." || fileInf.suffix() == "talk")
        {
            it.next();
            continue;
        }
        if(fileInf.isDir())  // if it is a dir
        {
            toSpeak = fileInf.fileName();
            filename = fileInf.absolutePath() + "/_dirname.talk";
        }
        else   // if it is a file
        {
            if(m_stripExtensions)
                toSpeak = fileInf.baseName();
            else
                toSpeak = fileInf.fileName();
            filename = fileInf.absoluteFilePath() + ".talk";
        }
        wavfilename = filename + ".wav";
        
        QFileInfo filenameInf(filename);
        QFileInfo wavfilenameInf(wavfilename);
        
        if(!filenameInf.exists() || m_overwriteTalk)
        {
            if(!wavfilenameInf.exists() || m_overwriteWav)
            {
                m_logger->addItem("Voicing of " + toSpeak,LOGINFO);
                if(!voice(toSpeak,wavfilename))
                {
                    m_logger->addItem("Voicing of " + toSpeak + " failed",LOGERROR);
                    m_logger->abort();
                    return false;
                }
            }
            m_logger->addItem("Encoding of " + toSpeak,LOGINFO);
            if(!encode(wavfilename,filename))
            {
                m_logger->addItem("Encoding of " + wavfilename + " failed",LOGERROR);
                m_logger->abort();
                return false;
            }
        }
        
        QString now = QDate::currentDate().toString("yyyyMMdd");
        if(m_removeWav)
        {
            QFile wavfile(wavfilename);
            wavfile.remove();
            installlog.remove(wavfilename);
        }
        else
            installlog.setValue(wavfilename.remove(m_mountpoint),now);
        
        installlog.setValue(filename.remove(m_mountpoint),now);
        it.next();
    }
    
    installlog.endGroup();
    m_logger->addItem("Finished creating Talkfiles",LOGOK);
    m_logger->setProgressMax(1);
    m_logger->setProgressValue(1);
    m_logger->abort();
    
    return true; 

}

void TalkFileCreator::abort()
{
    m_abort = true;
}

bool TalkFileCreator::voice(QString text,QString wavfile)
{

    QString execstring = m_curTTSTemplate;

    execstring.replace("%exe",m_TTSexec);
    execstring.replace("%options",m_TTSOpts);
    execstring.replace("%wavfile",wavfile);
    execstring.replace("%text",text);

    QProcess::execute(execstring);
    return true;

}

bool TalkFileCreator::encode(QString input,QString output)
{
    QString execstring = m_curEncTemplate;

    execstring.replace("%exe",m_EncExec);
    execstring.replace("%options",m_EncOpts);
    execstring.replace("%input",input);
    execstring.replace("%output",output);

    QProcess::execute(execstring);
    return true;

}


