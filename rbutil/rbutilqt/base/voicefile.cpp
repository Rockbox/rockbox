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

#include <QtCore>
#include "voicefile.h"
#include "utils.h"
#include "rockboxinfo.h"
#include "rbsettings.h"
#include "systeminfo.h"
#include "ziputil.h"
#include "Logger.h"

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
    m_targetid = info.targetID().toInt();
    m_versionstring = info.version();
    m_voiceformat = info.voicefmt();
    QString version = m_versionstring.left(m_versionstring.indexOf("-")).remove("r");

    // check if voicefile is present on target
    QString fn = m_mountpoint + "/.rockbox/langs/voicestrings.zip";
    LOG_INFO() << "searching for zipped voicestrings at" << fn;
    if(QFileInfo(fn).isFile()) {
        // search for binary voice strings file in archive
        ZipUtil z(this);
        if(z.open(fn)) {
            QStringList contents = z.files();
            int index;
            for(index = 0; index < contents.size(); ++index) {
                // strip any path, we don't know the structure in the zip
                if(QFileInfo(contents.at(index)).baseName() == m_lang) {
                    break;
                }
            }
            if(index < contents.size()) {
                LOG_INFO() << "extracting strings file from zip";
                // extract strings
                QTemporaryFile stringsfile;
                stringsfile.open();
                QString sfn = stringsfile.fileName();
                // ZipUtil::extractArchive() only compares the filename.
                if(z.extractArchive(sfn, QFileInfo(contents.at(index)).fileName())) {
                    emit logItem(tr("Extracted voice strings from installation"), LOGINFO);

                    stringsfile.seek(0);
                    QByteArray data = stringsfile.readAll();
                    const char* buf = data.constData();
                    // check file header
                    // header (4 bytes): cookie = 9a, version = 06, targetid, options
                    // subheader for each user. Only "core" for now.
                    // subheader (6 bytes): count (2bytes), size (2bytes), offset (2bytes)
                    if(buf[0] != (char)0x9a || buf[1] != 0x06 || buf[2] != m_targetid) {
                        emit logItem(tr("Extracted voice strings incompatible"), LOGINFO);
                    }
                    else {
                        QMap<int, QString> voicestrings;

                        /* skip header */
                        int idx = 10;
                        do {
                            unsigned int id = ((unsigned char)buf[idx])<<8
                                            | ((unsigned char)buf[idx+1]);
                            // need to use strlen here, since QString::size()
                            // returns number of characters, not bytes.
                            int len = strlen(&buf[idx + 2]);
                            voicestrings[id] = QString::fromUtf8(&buf[idx+2]);
                            idx += 2 + len + 1;

                        } while(idx < data.size());

                        stringsfile.close();

                        // create input file suitable for voicefont from strings.
                        QTemporaryFile voicefontlist;
                        voicefontlist.open();
                        m_filename = voicefontlist.fileName();
                        for(int i = 0; i < voicestrings.size(); ++i) {
                            QByteArray qba;
                            qba = QString("id: %1_%2\n")
                                    .arg(voicestrings.keys().at(i) < 0x8000 ? "LANG" : "VOICE")
                                    .arg(voicestrings.keys().at(i)).toUtf8();
                            voicefontlist.write(qba);
                            qba = QString("voice: \"%1\"\n").arg(
                                    voicestrings[voicestrings.keys().at(i)]).toUtf8();
                            voicefontlist.write(qba);
                        }
                        voicefontlist.close();

                        // everything successful, now create the actual voice file.
                        create();
                        return true;
                    }

                }
            }
        }
    }
    emit logItem(tr("Could not retrieve strings from installation, downloading"), LOGINFO);
    // if either no zip with voice strings is found or something went wrong
    // retrieving the necessary files we'll end up here, trying to get the
    // genlang output as previously from the webserver.

    // prepare download url
    QString genlang = SystemInfo::value(SystemInfo::GenlangUrl).toString();
    genlang.replace("%LANG%", m_lang);
    genlang.replace("%TARGET%", target);
    genlang.replace("%REVISION%", version);
    genlang.replace("%FEATURES%", features);
    QUrl genlangUrl(genlang);
    LOG_INFO() << "downloading" << genlangUrl;

    //download the correct genlang output
    QTemporaryFile *downloadFile = new QTemporaryFile(this);
    downloadFile->open();
    m_filename = downloadFile->fileName();
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
    LOG_INFO() << "download done, error:" << error;

    // update progress bar
    emit logProgress(1,1);
    if(getter->httpResponse() != 200 && !getter->isCached()) {
        emit logItem(tr("Download error: received HTTP error %1.")
                .arg(getter->httpResponse()),LOGERROR);
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
    create();
}


void VoiceFileCreator::create(void)
{
    //open downloaded file
    QFile genlang(m_filename);
    if(!genlang.open(QIODevice::ReadOnly))
    {
        emit logItem(tr("failed to open downloaded file"),LOGERROR);
        emit done(true);
        return;
    }

    //read in downloaded file
    emit logItem(tr("Reading strings..."),LOGINFO);
    QTextStream in(&genlang);
#if QT_VERSION < 0x060000
    in.setCodec("UTF-8");
#else
    in.setEncoding(QStringConverter::Utf8);
#endif
    QString id, voice;
    bool idfound = false;
    bool voicefound=false;
    bool useCorrection = RbSettings::value(RbSettings::UseTtsCorrections).toBool();
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
            voice = voice.remove("<").remove(">");
            voicefound=true;
        }

        if(idfound && voicefound)
        {
            TalkGenerator::TalkEntry entry;
            entry.toSpeak = voice;
            entry.wavfilename = m_path + "/" + id + ".wav";
            //voicefont wants them with .mp3 extension
            entry.talkfilename = m_path + "/" + id + ".mp3";
            entry.voiced = false;
            entry.encoded = false;
            if(id == "VOICE_PAUSE")
            {
                QFile::copy(":/builtin/VOICE_PAUSE.wav",m_path + "/VOICE_PAUSE.wav");
                entry.wavfilename = m_path + "/VOICE_PAUSE.wav";
                entry.voiced = true;
                m_talkList.append(entry);
            }
            else if(entry.toSpeak.isEmpty()) {
                LOG_WARNING() << "Empty voice string for ID" << id;
            }
            else {
                m_talkList.append(entry);
            }
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
        // set language for string correction. If not set no correction will be made.
        if(useCorrection)
            generator.setLang(m_lang);
        connect(&generator,SIGNAL(done(bool)),this,SIGNAL(done(bool)));
        connect(&generator,SIGNAL(logItem(QString,int)),this,SIGNAL(logItem(QString,int)));
        connect(&generator,SIGNAL(logProgress(int,int)),this,SIGNAL(logProgress(int,int)));
        connect(this,SIGNAL(aborted()),&generator,SLOT(abort()));

        if(generator.process(&m_talkList, m_wavtrimThreshold) == TalkGenerator::eERROR)
        {
            cleanup();
            emit logProgress(0,1);
            emit done(true);
            return;
        }
    }

    //make voicefile
    emit logItem(tr("Creating voicefiles..."),LOGINFO);
    FILE* ids2 = fopen(m_filename.toLocal8Bit(), "r");
    if (ids2 == nullptr)
    {
        cleanup();
        emit logItem(tr("Error opening downloaded file"),LOGERROR);
        emit done(true);
        return;
    }

    FILE* output = fopen(QString(m_mountpoint + "/.rockbox/langs/" + m_lang
                + ".voice").toLocal8Bit(), "wb");
    if (output == nullptr)
    {
        cleanup();
        fclose(ids2);
        emit logItem(tr("Error opening output file"),LOGERROR);
        emit done(true);
        return;
    }

    LOG_INFO() << "Running voicefont, format" << m_voiceformat;
    voicefont(ids2,m_targetid,m_path.toLocal8Bit().data(), output, m_voiceformat);
    // ids2 and output are closed by voicefont().

    //cleanup
    cleanup();

    // Add Voice file to the install log
    QSettings installlog(m_mountpoint + "/.rockbox/rbutil.log", QSettings::IniFormat, nullptr);
    installlog.beginGroup(QString("Voice (self created, %1)").arg(m_lang));
    installlog.setValue("/.rockbox/langs/" + m_lang + ".voice", m_versionstring);
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

