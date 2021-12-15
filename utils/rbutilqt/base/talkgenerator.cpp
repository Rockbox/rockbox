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

#include "talkgenerator.h"
#include "rbsettings.h"
#include "playerbuildinfo.h"
#include "wavtrim.h"
#include "Logger.h"

TalkGenerator::TalkGenerator(QObject* parent): QObject(parent)
{

}

//! \brief Creates Talkfiles.
//!
TalkGenerator::Status TalkGenerator::process(QList<TalkEntry>* list,int wavtrimth)
{
    m_abort = false;
    QString errStr;
    bool warnings = false;

    //tts
    emit logItem(tr("Starting TTS Engine"), LOGINFO);
    m_tts = TTSBase::getTTS(this, RbSettings::value(RbSettings::Tts).toString());
    if(!m_tts)
    {
        LOG_ERROR() << "getting the TTS object failed!";
        emit logItem(tr("Init of TTS engine failed"), LOGERROR);
        emit done(true);
        return eERROR;
    }
    if(!m_tts->start(&errStr))
    {
        emit logItem(errStr.trimmed(),LOGERROR);
        emit logItem(tr("Init of TTS engine failed"), LOGERROR);
        emit done(true);
        return eERROR;
    }
    QCoreApplication::processEvents();

    // Encoder
    emit logItem(tr("Starting Encoder Engine"),LOGINFO);
    m_enc = EncoderBase::getEncoder(this, PlayerBuildInfo::instance()->value(
                    PlayerBuildInfo::Encoder).toString());
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
    int progressMax = list->size();
    int m_progress = 0;
    emit logProgress(m_progress,progressMax);

    QStringList errors;
    QStringList duplicates;

    bool warnings = false;
    for(int i=0; i < list->size(); i++)
    {
        if(m_abort)
        {
            emit logItem(tr("Voicing aborted"), LOGERROR);
            return eERROR;
        }

        // skip duplicated wav entrys
        if(!duplicates.contains(list->at(i).wavfilename))
            duplicates.append(list->at(i).wavfilename);
        else
        {
            LOG_INFO() << "duplicate skipped";
            (*list)[i].voiced = true;
            emit logProgress(++m_progress,progressMax);
            continue;
        }

        // skip already voiced entrys
        if(list->at(i).voiced == true)
        {
            emit logProgress(++m_progress,progressMax);
            continue;
        }
        // skip entry whith empty text
        if(list->at(i).toSpeak == "")
        {
            emit logProgress(++m_progress,progressMax);
            continue;
        }

        // voice entry
        QString error;
        LOG_INFO() << "voicing: " << list->at(i).toSpeak
                 << "to" << list->at(i).wavfilename;
        TTSStatus status = m_tts->voice(list->at(i).toSpeak,
                                        list->at(i).wavfilename, &error);
        if(status == Warning)
        {
            warnings = true;
            emit logItem(tr("Voicing of %1 failed: %2").arg(list->at(i).toSpeak).arg(error),
                    LOGWARNING);
        }
        else if (status == FatalError)
        {
            emit logItem(tr("Voicing of %1 failed: %2").arg(list->at(i).toSpeak).arg(error),
                    LOGERROR);
            return eERROR;
        }
        else
           (*list)[i].voiced = true;

        // wavtrim if needed
        if(wavtrimth != -1)
        {
            char buffer[255];
            if(wavtrim(list->at(i).wavfilename.toLocal8Bit().data(),
                       wavtrimth, buffer, 255))
            {
                LOG_ERROR() << "wavtrim returned error on"
                            << list->at(i).wavfilename;
                return eERROR;
            }
        }

        emit logProgress(++m_progress,progressMax);
        QCoreApplication::processEvents();
    }
    if(warnings)
        return eWARNING;
    else
        return eOK;
}


//! \brief Encodes a List of strings
//!
TalkGenerator::Status TalkGenerator::encodeList(QList<TalkEntry>* list)
{
    QStringList duplicates;

    int progressMax = list->size();
    int m_progress = 0;
    emit logProgress(m_progress,progressMax);

    for(int i=0; i < list->size(); i++)
    {
        if(m_abort)
        {
            emit logItem(tr("Encoding aborted"), LOGERROR);
            return eERROR;
        }

         //skip non-voiced entrys
        if(list->at(i).voiced == false)
        {
            LOG_WARNING() << "non voiced entry detected:"
                          << list->at(i).toSpeak;
            emit logProgress(++m_progress,progressMax);
            continue;
        }
        //skip duplicates
         if(!duplicates.contains(list->at(i).talkfilename))
            duplicates.append(list->at(i).talkfilename);
        else
        {
            LOG_INFO() << "duplicate skipped";
            (*list)[i].encoded = true;
            emit logProgress(++m_progress,progressMax);
            continue;
        }

        //encode entry
        LOG_INFO() << "encoding " << list->at(i).wavfilename
                   << "to" << list->at(i).talkfilename;
        if(!m_enc->encode(list->at(i).wavfilename,list->at(i).talkfilename))
        {
            emit logItem(tr("Encoding of %1 failed").arg(
                QFileInfo(list->at(i).wavfilename).baseName()), LOGERROR);
            return eERROR;
        }
        (*list)[i].encoded = true;
        emit logProgress(++m_progress,progressMax);
        QCoreApplication::processEvents();
    }
    return eOK;
}

//! \brief slot, which is connected to the abort of the Logger.
//Sets a flag, so Creating Talkfiles ends at the next possible position
//!
void TalkGenerator::abort()
{
    m_abort = true;
}

QString TalkGenerator::correctString(QString s)
{
    QString corrected = s;
    int i = 0;
    int max = m_corrections.size();
    while(i < max) {
        corrected = corrected.replace(QRegExp(m_corrections.at(i).search,
                m_corrections.at(i).modifier.contains("i")
                    ? Qt::CaseInsensitive : Qt::CaseSensitive),
                m_corrections.at(i).replace);
        i++;
    }

    if(corrected != s)
        LOG_INFO() << "corrected string" << s << "to" << corrected;

    return corrected;
    m_abort = true;
}

void TalkGenerator::setLang(QString name)
{
    m_lang = name;

    // re-initialize corrections list
    m_corrections.clear();
    QFile correctionsFile(":/builtin/voice-corrections.txt");
    correctionsFile.open(QIODevice::ReadOnly);

    QString engine = RbSettings::value(RbSettings::Tts).toString();
    TTSBase* tts = TTSBase::getTTS(this,RbSettings::value(RbSettings::Tts).toString());
    if(!tts)
    {
        LOG_ERROR() << "getting the TTS object failed!";
        return;
    }
    QString vendor = tts->voiceVendor();
    delete tts;

    if(m_lang.isEmpty())
        m_lang = "english";
    LOG_INFO() << "building string corrections list for"
               << m_lang << engine << vendor;
    QTextStream stream(&correctionsFile);
    while(!stream.atEnd()) {
        QString line = stream.readLine();
        if(line.startsWith(" ") || line.length() < 10)
            continue;
        // separator is first character
        QString separator = line.at(0);
        line.remove(0, 1);
        QStringList items = line.split(separator);
        // we need to have at least 6 separate entries.
        if(items.size() < 6)
            continue;

        QRegExp re_lang(items.at(0));
        QRegExp re_engine(items.at(1));
        QRegExp re_vendor(items.at(2));
        if(!re_lang.exactMatch(m_lang)) {
            continue;
        }
        if(!re_vendor.exactMatch(vendor)) {
            continue;
        }
        if(!re_engine.exactMatch(engine)) {
            continue;
        }
        struct CorrectionItems co;
        co.search = items.at(3);
        co.replace = items.at(4);
        // Qt uses backslash for back references, Perl uses dollar sign.
        co.replace.replace(QRegExp("\\$(\\d+)"), "\\\\1");
        co.modifier = items.at(5);
        m_corrections.append(co);
    }
    correctionsFile.close();
}
