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

#include "talkgenerator.h"
#include "rbsettings.h"
#include "systeminfo.h"
#include "wavtrim.h"

TalkGenerator::TalkGenerator(QObject* parent): QObject(parent), encFutureWatcher(this), ttsFutureWatcher(this)
{
    m_userAborted = false;
}

//! \brief Creates Talkfiles.
//!
TalkGenerator::Status TalkGenerator::process(QList<TalkEntry>* list,int wavtrimth)
{
    QString errStr;
    bool warnings = false;

    //tts
    emit logItem(tr("Starting TTS Engine"),LOGINFO);
    m_tts = TTSBase::getTTS(this,RbSettings::value(RbSettings::Tts).toString());
    if(!m_tts->start(&errStr))
    {
        emit logItem(errStr.trimmed(),LOGERROR);
        emit logItem(tr("Init of TTS engine failed"),LOGERROR);
        emit done(true);
        return eERROR;
    }
    QCoreApplication::processEvents();

    // Encoder
    emit logItem(tr("Starting Encoder Engine"),LOGINFO);
    m_enc = EncBase::getEncoder(this,SystemInfo::value(SystemInfo::CurEncoder).toString());
    if(!m_enc->start())
    {
        emit logItem(tr("Init of Encoder engine failed"),LOGERROR);
        emit done(true);
        m_tts->stop();
        return eERROR;
    }
    QCoreApplication::processEvents();

    emit logProgress(0,0);

    // Voice entries
    emit logItem(tr("Voicing entries..."),LOGINFO);
    Status voiceStatus= voiceList(list,wavtrimth);
    if(voiceStatus == eERROR)
    {
        m_tts->stop();
        m_enc->stop();
        emit done(true);
        return eERROR;
    }
    else if( voiceStatus == eWARNING)
        warnings = true;

    QCoreApplication::processEvents();

    // Encoding Entries
    emit logItem(tr("Encoding files..."),LOGINFO);
    Status encoderStatus = encodeList(list);
    if( encoderStatus == eERROR)
    {
        m_tts->stop();
        m_enc->stop();
        emit done(true);
        return eERROR;
    }
    else if( voiceStatus == eWARNING)
        warnings = true;

    QCoreApplication::processEvents();

    m_tts->stop();
    m_enc->stop();
    emit logProgress(1,1);

    if(warnings)
        return eWARNING;
    return eOK;
}

//! \brief Voices a List of string
//!
TalkGenerator::Status TalkGenerator::voiceList(QList<TalkEntry>* list,int wavtrimth)
{
    emit logProgress(0, list->size());

    QStringList duplicates;

    m_ttsWarnings = false;
    for(int i=0; i < list->size(); i++)
    {
        (*list)[i].refs.tts = m_tts;
        (*list)[i].refs.wavtrim = wavtrimth;
        (*list)[i].refs.generator = this;

        // skip duplicated wav entries
        if(!duplicates.contains(list->at(i).wavfilename))
            duplicates.append(list->at(i).wavfilename);
        else
        {
            qDebug() << "[TalkGen] duplicate skipped";
            (*list)[i].voiced = true;
            continue;
        }
    }

    /* If the engine can't be parallelized, we use only 1 thread */
    // NOTE: setting the number of maximum threads to use to 1 doesn't seem to
    // work as expected -- it causes sporadically output files missing (see
    // FS#11994). As a stop-gap solution use a separate implementation in that
    // case for running the TTS.
    if((m_tts->capabilities() & TTSBase::RunInParallel) != 0)
    {
        int maxThreadCount = QThreadPool::globalInstance()->maxThreadCount();
        qDebug() << "[TalkGenerator] Maximum number of threads used:"
            << QThreadPool::globalInstance()->maxThreadCount();

        connect(&ttsFutureWatcher, SIGNAL(progressValueChanged(int)),
                this, SLOT(ttsProgress(int)));
        ttsFutureWatcher.setFuture(QtConcurrent::map(*list, &TalkGenerator::ttsEntryPoint));

        /* We use this loop as an equivalent to ttsFutureWatcher.waitForFinished() 
         * since the latter blocks all events */
        while(ttsFutureWatcher.isRunning())
            QCoreApplication::processEvents();

        /* Restore global settings, if we changed them */
        if ((m_tts->capabilities() & TTSBase::RunInParallel) == 0)
            QThreadPool::globalInstance()->setMaxThreadCount(maxThreadCount);

        if(ttsFutureWatcher.isCanceled())
            return eERROR;
        else if(m_ttsWarnings)
            return eWARNING;
        else
            return eOK;
    }
    else {
        qDebug() << "[TalkGenerator] Using single thread TTS workaround";
        int items = list->size();
        for(int i = 0; i < items; i++) {
            if(m_userAborted) {
                emit logItem(tr("Voicing aborted"), LOGERROR);
                return eERROR;
            }
            TalkEntry entry = list->at(i);
            TalkGenerator::ttsEntryPoint(entry);
            (*list)[i] = entry;
            emit logProgress(i, items);
        }
        return m_ttsWarnings ? eWARNING : eOK;
    }
}

void TalkGenerator::ttsEntryPoint(TalkEntry& entry)
{
    if (!entry.voiced && !entry.toSpeak.isEmpty())
    {
        QString error;
        qDebug() << "[TalkGen] voicing: " << entry.toSpeak << "to" << entry.wavfilename;
        TTSStatus status = entry.refs.tts->voice(entry.toSpeak,entry.wavfilename, &error);
        if (status == Warning || status == FatalError)
        {
            entry.refs.generator->ttsFailEntry(entry, status, error);
            return;
        }
        if (entry.refs.wavtrim != -1)
        {
            char buffer[255];
            wavtrim(entry.wavfilename.toLocal8Bit().data(), entry.refs.wavtrim, buffer, 255);
        }
        entry.voiced = true;
    }
}

void TalkGenerator::ttsFailEntry(const TalkEntry& entry, TTSStatus status, QString error)
{
    if(status == Warning)
    {
        m_ttsWarnings = true;
        emit logItem(tr("Voicing of %1 failed: %2").arg(entry.toSpeak).arg(error),
                    LOGWARNING);
    }
    else if (status == FatalError)
    {
        emit logItem(tr("Voicing of %1 failed: %2").arg(entry.toSpeak).arg(error),
                    LOGERROR);
        abort();
    }
}

void TalkGenerator::ttsProgress(int value)
{
    emit logProgress(value,ttsFutureWatcher.progressMaximum());
}

//! \brief Encodes a List of strings
//!
TalkGenerator::Status TalkGenerator::encodeList(QList<TalkEntry>* list)
{
    QStringList duplicates;

    int itemsCount = list->size();
    emit logProgress(0, itemsCount);

    /* Do some preprocessing and remove entries that have not been voiced. */
    for (int idx=0; idx < itemsCount; idx++)
    {
        if(list->at(idx).voiced == false)
        {
            qDebug() << "[TalkGen] unvoiced entry" << list->at(idx).toSpeak <<"detected";
            list->removeAt(idx);
            itemsCount--;
            idx--;
            continue;
        }
        if(duplicates.contains(list->at(idx).talkfilename))
        {
            (*list)[idx].encoded = true; /* make sure we skip this entry */
            continue;
        }
        duplicates.append(list->at(idx).talkfilename);
        (*list)[idx].refs.encoder = m_enc;
        (*list)[idx].refs.generator = this; /* not really needed, unless we end up 
                                               voicing and encoding with two different
                                               TalkGenerators.*/
    }

    connect(&encFutureWatcher, SIGNAL(progressValueChanged(int)), 
            this, SLOT(encProgress(int)));
    encFutureWatcher.setFuture(QtConcurrent::map(*list, &TalkGenerator::encEntryPoint));

    /* We use this loop as an equivalent to encFutureWatcher.waitForFinished() 
     * since the latter blocks all events */
    while (encFutureWatcher.isRunning())
        QCoreApplication::processEvents(QEventLoop::AllEvents);

    if (encFutureWatcher.isCanceled())
        return eERROR;
    else
        return eOK;
}

void TalkGenerator::encEntryPoint(TalkEntry& entry)
{
    if(!entry.encoded)
    {
        bool res = entry.refs.encoder->encode(entry.wavfilename, entry.talkfilename);
        entry.encoded = res;
        if (!entry.encoded)
            entry.refs.generator->encFailEntry(entry);
        }
    return;
}

void TalkGenerator::encProgress(int value)
{
    emit logProgress(value, encFutureWatcher.progressMaximum());
}

void TalkGenerator::encFailEntry(const TalkEntry& entry)
{
    emit logItem(tr("Encoding of %1 failed").arg(entry.wavfilename), LOGERROR);
    abort();      
}

//! \brief slot, which is connected to the abort of the Logger. Sets a flag, so Creating Talkfiles ends at the next possible position
//!
void TalkGenerator::abort()
{
    if (ttsFutureWatcher.isRunning())
    {
        ttsFutureWatcher.cancel();
        emit logItem(tr("Voicing aborted"), LOGERROR);
    }
    if (encFutureWatcher.isRunning())
    {
        encFutureWatcher.cancel();
        emit logItem(tr("Encoding aborted"), LOGERROR);
    }
    m_userAborted = true;
}

