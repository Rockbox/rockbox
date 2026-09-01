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
#include "zipinstaller.h"
#include "utils.h"
#include "rbsettings.h"
#include "ziputil.h"
#include "Logger.h"

class ZipInstallThread : public QThread
{
public:
    enum Result {
        Success,
        OpenFailed,
        NotEnoughSpace,
        ExtractionFailed,
        CopyFailed
    };

    explicit ZipInstallThread(ZipInstaller *owner)
        : QThread(owner), m_owner(owner), m_unzip(true),
          m_result(OpenFailed), m_requiredSpace(0)
    {
    }

    void setArchive(const QString& file, const QString& mountpoint)
    {
        m_file = file;
        m_mountpoint = mountpoint;
        m_unzip = true;
    }

    void setCopy(const QString& file, const QString& mountpoint,
                 const QString& target)
    {
        m_file = file;
        m_mountpoint = mountpoint;
        m_target = target;
        m_unzip = false;
    }

    Result result(void) const { return m_result; }
    QStringList installedFiles(void) const { return m_installedFiles; }
protected:
    void run(void) override
    {
        if(m_unzip) {
            ZipUtil zip(nullptr);
            connect(&zip, &ZipUtil::logProgress,
                    m_owner, &ZipInstaller::logProgress);
            connect(&zip, &ZipUtil::logItem,
                    m_owner, &ZipInstaller::logItem);

            if(!zip.open(m_file, QuaZip::mdUnzip)) {
                m_result = OpenFailed;
                return;
            }

            m_requiredSpace = zip.totalUncompressedSize(
                    Utils::filesystemClusterSize(m_mountpoint));
            if((qint64)Utils::filesystemFree(m_mountpoint)
                    < m_requiredSpace + 1000000) {
                zip.close();
                m_result = NotEnoughSpace;
                return;
            }

            m_installedFiles = zip.files();
            if(!zip.extractArchive(m_mountpoint)) {
                m_result = ExtractionFailed;
                zip.close();
                return;
            }
            zip.close();
        }
        else {
            QString destfile = m_mountpoint + "/" + m_target;
            QString path = QFileInfo(destfile).absolutePath();
            if(!QDir().mkpath(path)) {
                m_result = CopyFailed;
                return;
            }
            QFile(destfile).remove();
            if(!QFile::copy(m_file, destfile)) {
                m_result = CopyFailed;
                return;
            }
            m_installedFiles.append(m_target);
        }

        m_result = Success;
    }

private:
    ZipInstaller *m_owner;
    QString m_file;
    QString m_mountpoint;
    QString m_target;
    QStringList m_installedFiles;
    bool m_unzip;
    Result m_result;
    qint64 m_requiredSpace;
};

ZipInstaller::ZipInstaller(QObject* parent) :
    QObject(parent),
    m_unzip(true), m_usecache(false), m_getter(nullptr),
    m_installThread(nullptr)
{
}


ZipInstaller::~ZipInstaller()
{
    if(m_installThread && m_installThread->isRunning()) {
        m_installThread->wait();
    }
}


void ZipInstaller::install()
{
    LOG_INFO() << "initializing installation";

    m_runner = 0;
    connect(this, &ZipInstaller::cont, this, &ZipInstaller::installContinue);
    m_url = m_urllist.at(m_runner);
    m_logsection = m_loglist.at(m_runner);
    m_logver = m_verlist.at(m_runner);
    installStart();
}


void ZipInstaller::abort()
{
    LOG_INFO() << "Aborted";
    emit internalAborted();
}


void ZipInstaller::installContinue()
{
    LOG_INFO() << "continuing installation";

    m_runner++; // this gets called when a install finished, so increase first.
    LOG_INFO() << "runner done:" << m_runner << "/" << m_urllist.size();
    if(m_runner < m_urllist.size()) {
        emit logItem(tr("done."), LOGOK);
        m_url = m_urllist.at(m_runner);
        m_logsection = m_loglist.at(m_runner);
        if(m_runner < m_verlist.size()) m_logver = m_verlist.at(m_runner);
        else m_logver = "";
        installStart();
    }
    else {
        emit logItem(tr("Package installation finished successfully."), LOGOK);
        emit done(false);
        return;
    }
}


void ZipInstaller::installStart()
{
    LOG_INFO() << "starting installation";

    emit logItem(tr("Downloading file %1.%2").arg(QFileInfo(m_url).baseName(),
            QFileInfo(m_url).completeSuffix()),LOGINFO);

    // temporary file needs to be opened to get the filename
    // make sure to get a fresh one on each run.
    // making this a parent of the temporary file ensures the file gets deleted
    // after the class object gets destroyed.
    m_downloadFile = new QTemporaryFile(this);
    m_downloadFile->open();
    m_file = m_downloadFile->fileName();
    m_downloadFile->close();
    // get the real file.
    if(m_getter != nullptr) m_getter->deleteLater();
    m_getter = new HttpGet(this);
    if(m_usecache) {
        m_getter->setCache(true);
    }
    m_getter->setFile(m_downloadFile);

    connect(m_getter, &HttpGet::done, this, &ZipInstaller::downloadDone);
    connect(m_getter, &HttpGet::dataReadProgress, this, &ZipInstaller::logProgress);
    connect(this, &ZipInstaller::internalAborted, m_getter, &HttpGet::abort);

    m_getter->getFile(QUrl(m_url));
}


void ZipInstaller::downloadDone(QNetworkReply::NetworkError error)
{
    LOG_INFO() << "download done, error:" << error;
     // update progress bar

    emit logProgress(1, 1);
    if(m_getter->httpResponse() != 200 && !m_getter->isCached()) {
        emit logItem(tr("Download error: received HTTP error %1\n%2")
                    .arg(m_getter->httpResponse()).arg(m_getter->errorString()),
                         LOGERROR);
        emit done(true);
        return;
    }
    if(error != QNetworkReply::NoError) {
        emit logItem(tr("Download error: %1").arg(m_getter->errorString()), LOGERROR);
        emit done(true);
        return;
    }
    else if(m_getter->isCached()) {
        emit logItem(tr("Download finished (cache used)."), LOGOK);
    }
    else {
        emit logItem(tr("Download finished."),LOGOK);
    }
    if(m_unzip) {
        LOG_INFO() << "about to unzip" << m_file << "to" << m_mountpoint;
        emit logItem(tr("Extracting file."), LOGINFO);
        m_installThread = new ZipInstallThread(this);
        m_installThread->setArchive(m_file, m_mountpoint);
    }
    else {
        if (m_target.isEmpty())
            m_target = QUrl(m_url).fileName();
        QString destfile = m_mountpoint + "/" + m_target;
        emit logItem(tr("Installing file."), LOGINFO);
        LOG_INFO() << "saving downloaded file (no extraction) to" << destfile;
        // Keep the temporary file materialized while the worker copies it.
        m_downloadFile->open();
        m_installThread = new ZipInstallThread(this);
        m_installThread->setCopy(m_file, m_mountpoint, m_target);
    }

    connect(m_installThread, &QThread::finished,
            this, &ZipInstaller::installFinished);
    m_installThread->start(QThread::LowPriority);
}


void ZipInstaller::installFinished()
{
    ZipInstallThread *thread = m_installThread;
    m_installThread = nullptr;
    ZipInstallThread::Result result = thread->result();
    QStringList zipContents = thread->installedFiles();
    thread->deleteLater();

    emit logProgress(1, 1);
    switch(result) {
        case ZipInstallThread::Success:
            break;
        case ZipInstallThread::NotEnoughSpace:
            emit logItem(tr("Not enough disk space! Aborting."), LOGERROR);
            emit done(true);
            return;
        case ZipInstallThread::CopyFailed:
            emit logItem(tr("Installing file failed."), LOGERROR);
            emit done(true);
            return;
        case ZipInstallThread::OpenFailed:
        case ZipInstallThread::ExtractionFailed:
            emit logItem(tr("Extraction failed!"), LOGERROR);
            emit done(true);
            return;
    }

    if(m_logver.isEmpty()) {
        // if no version info is set use the timestamp of the server file.
        m_logver = m_getter->timestamp().toString(Qt::ISODate);
    }

    emit logItem(tr("Creating installation log"),LOGINFO);

    QString logpath;
    QString suffix = RbSettings::value(RbSettings::Suffix).toString();

    if (!suffix.isEmpty()) {
        logpath = m_mountpoint + suffix + "/.rockbox/rbutil.log";
    } else {
        logpath = m_mountpoint + "/.rockbox/rbutil.log";
    }

    QSettings installlog(logpath, QSettings::IniFormat, nullptr);

    installlog.beginGroup(m_logsection);
    for(int i = 0; i < zipContents.size(); i++)
    {
        installlog.setValue(zipContents.at(i), m_logver);
    }
    installlog.endGroup();
    installlog.sync();

    emit cont();
}


