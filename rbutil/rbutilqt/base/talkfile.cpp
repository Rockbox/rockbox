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

#include "talkfile.h"
#include "rbsettings.h"
#include "Logger.h"

TalkFileCreator::TalkFileCreator(QObject* parent): QObject(parent)
{

}

//! \brief Creates Talkfiles.
//!
//! \param logger A pointer to a Loggerobject
bool TalkFileCreator::createTalkFiles()
{
    m_abort = false;
    QString errStr;

    emit logItem(tr("Starting Talk file generation for folder %1")
            .arg(m_dir), LOGINFO);
    emit logProgress(0,0);
    QCoreApplication::processEvents();

    // read in Maps of paths - file/dirnames
    emit logItem(tr("Reading Filelist..."),LOGINFO);
    if(createTalkList(m_mountpoint + "/" + m_dir) == false)
    {
        emit logItem(tr("Talk file creation aborted"),LOGERROR);
        doAbort();
        return false;
    }
    QCoreApplication::processEvents();

    // generate entries
    {
        TalkGenerator generator(this);
        // no string corrections yet: do not set language for TalkGenerator.
        connect(&generator,SIGNAL(done(bool)),this,SIGNAL(done(bool)));
        connect(&generator,SIGNAL(logItem(QString,int)),this,SIGNAL(logItem(QString,int)));
        connect(&generator,SIGNAL(logProgress(int,int)),this,SIGNAL(logProgress(int,int)));
        connect(this,SIGNAL(aborted()),&generator,SLOT(abort()));

        if(generator.process(&m_talkList) == TalkGenerator::eERROR)
        {
            doAbort();
            return false;
        }
    }

    // Copying talk files
    emit logItem(tr("Copying Talkfiles..."),LOGINFO);
    if(copyTalkFiles(&errStr) == false)
    {
        emit logItem(errStr,LOGERROR);
        doAbort();
        return false;
    }

    // Deleting left overs
    if( !cleanup())
        return false;

    emit logItem(tr("Finished creating Talk files"),LOGOK);
    emit logProgress(1,1);
    emit done(false);

    return true;
}

//! \brief Strips everything after and including the last dot in a string. If there is no dot, nothing is changed
//!
//! \param filename The filename from which to strip the Extension
//! \returns the modified string
QString TalkFileCreator::stripExtension(QString filename)
{
    // only strip extension if there is a dot in the filename and there are chars before the dot
    if(filename.lastIndexOf(".") != -1 && filename.left(filename.lastIndexOf(".")) != "")
        return filename.left(filename.lastIndexOf("."));
    else
        return filename;
}

//! \brief Does needed Tasks when we need to abort. Cleans up Files. Stops the Logger, Stops TTS and Encoder
//!
void TalkFileCreator::doAbort()
{
    cleanup();
    emit logProgress(0,1);
    emit done(true);
}
//! \brief creates a list of what to generate
//!
//! \param startDir The directory from which to start scanning
bool TalkFileCreator::createTalkList(QDir startDir)
{
    LOG_INFO() << "generating list of files" << startDir;
    m_talkList.clear();

     // create Iterator
    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if(m_recursive)
        flags = QDirIterator::Subdirectories;

    QDirIterator it(startDir,flags);

    //create temp directory
    QDir tempDir(QDir::tempPath()+ "/talkfiles/");
    if(!tempDir.exists())
        tempDir.mkpath(QDir::tempPath()+ "/talkfiles/");

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
                // check if we should ignore it
                if(m_generateOnlyNew && QFileInfo::exists(dir.path() + "/_dirname.talk"))
                {
                    continue;
                }

                //generate entry
                TalkGenerator::TalkEntry entry;
                entry.toSpeak = dir.dirName();
                entry.wavfilename = QDir::tempPath() + "/talkfiles/"
                    + QCryptographicHash::hash(entry.toSpeak.toUtf8(),
                    QCryptographicHash::Md5).toHex() + ".wav";
                entry.talkfilename = QDir::tempPath() + "/talkfiles/"
                    + QCryptographicHash::hash(entry.toSpeak.toUtf8(),
                    QCryptographicHash::Md5).toHex() + ".talk";
                entry.target = dir.path() + "/_dirname.talk";
                entry.voiced = false;
                entry.encoded = false;
                LOG_INFO() << "toSpeak:" << entry.toSpeak
                           << "target:" << entry.target
                           << "intermediates:" << entry.wavfilename << entry.talkfilename;
                m_talkList.append(entry);
            }
        }
        else  // its a File
        {
            // insert into List
            if( !fileInf.fileName().isEmpty() && !fileInf.fileName().endsWith(".talk") && m_talkFiles)
            {
                //test if we should ignore this file
                bool match = false;
                for(int i=0; i < m_ignoreFiles.size();i++)
                {
                    QRegExp rx(m_ignoreFiles[i].trimmed());
                    rx.setPatternSyntax(QRegExp::Wildcard);
                    if(rx.exactMatch(fileInf.fileName()))
                        match = true;
                }
                if(match)
                    continue;

                // check if we should ignore it
                if(m_generateOnlyNew && QFileInfo::exists(fileInf.path() + "/" + fileInf.fileName() + ".talk"))
                {
                    continue;
                }

                //generate entry
                TalkGenerator::TalkEntry entry;
                if(m_stripExtensions)
                    entry.toSpeak = stripExtension(fileInf.fileName());
                else
                    entry.toSpeak = fileInf.fileName();
                entry.wavfilename = QDir::tempPath() + "/talkfiles/"
                    + QCryptographicHash::hash(entry.toSpeak.toUtf8(),
                    QCryptographicHash::Md5).toHex() + ".wav";
                entry.talkfilename = QDir::tempPath() + "/talkfiles/"
                    + QCryptographicHash::hash(entry.toSpeak.toUtf8(),
                    QCryptographicHash::Md5).toHex() + ".talk";
                entry.target =  fileInf.path() + "/" + fileInf.fileName() + ".talk";
                entry.voiced = false;
                entry.encoded = false;
                LOG_INFO() << "toSpeak:" << entry.toSpeak
                           << "target:" << entry.target
                           << "intermediates:"
                           << entry.wavfilename << entry.talkfilename;
                m_talkList.append(entry);
            }
        }
        QCoreApplication::processEvents();
    }
    LOG_INFO() << "list created, entries:" << m_talkList.size();
    return true;
}


//! \brief copys Talkfiles from the temp dir to the target. Progress and installlog is handled inside
//!
//! \param errString Pointer to a QString where the error cause is written.
//! \returns true on success, false on error or user abort
bool TalkFileCreator::copyTalkFiles(QString* errString)
{
    int progressMax = m_talkList.size();
    int m_progress = 0;
    emit logProgress(m_progress,progressMax);

    QSettings installlog(m_mountpoint + "/.rockbox/rbutil.log", QSettings::IniFormat, nullptr);
    installlog.beginGroup("talkfiles");

    for(int i=0; i < m_talkList.size(); i++)
    {
        if(m_abort)
        {
            *errString = tr("File copy aborted");
            return false;
        }

        // skip not encoded files
        if(m_talkList[i].encoded == false)
        {
            emit logProgress(++m_progress,progressMax);
            continue; // this file was skipped in one of the previous steps
        }
        // remove target if it exists, and if we should overwrite it
        if(QFile::exists(m_talkList[i].target))
            QFile::remove(m_talkList[i].target);

        // copying
        LOG_INFO() << "copying" << m_talkList[i].talkfilename
                   << "to" << m_talkList[i].target;
        if(!QFile::copy(m_talkList[i].talkfilename,m_talkList[i].target))
        {
            *errString = tr("Copying of %1 to %2 failed").arg(m_talkList[i].talkfilename).arg(m_talkList[i].target);
            return false;
        }

        // add to installlog
        QString now = QDate::currentDate().toString("yyyyMMdd");
        installlog.setValue(m_talkList[i].target.remove(0,m_mountpoint.length()),now);

        emit logProgress(++m_progress,progressMax);
        QCoreApplication::processEvents();
    }
    installlog.endGroup();
    installlog.sync();
    return true;
}


//! \brief Cleans up Files potentially left in the temp dir
//!
bool TalkFileCreator::cleanup()
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
    return true;
}

//! \brief slot, which is connected to the abort of the Logger. Sets a flag, so Creating Talkfiles ends at the next possible position
//!
void TalkFileCreator::abort()
{
    m_abort = true;
    emit aborted();
}

