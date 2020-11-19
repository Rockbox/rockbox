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
#include "ziputil.h"
#include "Logger.h"

ZipInstaller::ZipInstaller(QObject* parent) :
    QObject(parent),
    m_unzip(true), m_usecache(false), m_getter(nullptr)
{
}


void ZipInstaller::install()
{
    LOG_INFO() << "initializing installation";

    m_runner = 0;
    connect(this, SIGNAL(cont()), this, SLOT(installContinue()));
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

    connect(m_getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
    connect(m_getter, SIGNAL(dataReadProgress(int, int)), this, SIGNAL(logProgress(int, int)));
    connect(this, SIGNAL(internalAborted()), m_getter, SLOT(abort()));

    m_getter->getFile(QUrl(m_url));
}


void ZipInstaller::downloadDone(bool error)
{
    LOG_INFO() << "download done, error:" << error;
    QStringList zipContents; // needed later
     // update progress bar

    emit logProgress(1, 1);
    if(m_getter->httpResponse() != 200 && !m_getter->isCached()) {
        emit logItem(tr("Download error: received HTTP error %1\n%2")
                    .arg(m_getter->httpResponse()).arg(m_getter->errorString()),
                         LOGERROR);
        emit done(true);
        return;
    }
    if(m_getter->isCached())
        emit logItem(tr("Cached file used."), LOGINFO);
    if(error) {
        emit logItem(tr("Download error: %1").arg(m_getter->errorString()), LOGERROR);
        emit done(true);
        return;
    }
    else emit logItem(tr("Download finished."),LOGOK);
    QCoreApplication::processEvents();
    if(m_unzip) {
        // unzip downloaded file
        LOG_INFO() << "about to unzip" << m_file << "to" << m_mountpoint;

        emit logItem(tr("Extracting file."), LOGINFO);
        QCoreApplication::processEvents();

        ZipUtil zip(this);
        connect(&zip, SIGNAL(logProgress(int, int)), this, SIGNAL(logProgress(int, int)));
        connect(&zip, SIGNAL(logItem(QString, int)), this, SIGNAL(logItem(QString, int)));
        zip.open(m_file, QuaZip::mdUnzip);
        // check for free space. Make sure after installation will still be
        // some room for operating (also includes calculation mistakes due to
        // cluster sizes on the player).
        if((qint64)Utils::filesystemFree(m_mountpoint)
                < (zip.totalUncompressedSize(
                       Utils::filesystemSize(m_mountpoint, Utils::FilesystemClusterSize))
                        + 1000000)) {
            emit logItem(tr("Not enough disk space! Aborting."), LOGERROR);
            emit logProgress(1, 1);
            emit done(true);
            return;
        }
        zipContents = zip.files();
        if(!zip.extractArchive(m_mountpoint)) {
            emit logItem(tr("Extraction failed!"), LOGERROR);
            emit logProgress(1, 1);
            emit done(true);
            return;
        }
        zip.close();
    }
    else {
        if (m_target.isEmpty())
            m_target = QUrl(m_url).fileName();
        QString destfile = m_mountpoint + "/" + m_target;
        // only copy the downloaded file to the output location / name
        emit logItem(tr("Installing file."), LOGINFO);
        LOG_INFO() << "saving downloaded file (no extraction) to" << destfile;

        m_downloadFile->open(); // copy fails if file is not opened (filename issue?)
        // make sure the required path is existing
        QString path = QFileInfo(destfile).absolutePath();
        QDir p;
        p.mkpath(path);
        // QFile::copy() doesn't overwrite files, so remove old one first
        // TODO: compare old and new file and fail if those are different.
        QFile(destfile).remove();
        if(!m_downloadFile->copy(destfile)) {
            emit logItem(tr("Installing file failed."), LOGERROR);
            emit done(true);
            return;
        }

        // add file to log
        zipContents.append(m_target);
    }
    if(m_logver.isEmpty()) {
        // if no version info is set use the timestamp of the server file.
        m_logver = m_getter->timestamp().toString(Qt::ISODate);
    }

    emit logItem(tr("Creating installation log"),LOGINFO);
    QSettings installlog(m_mountpoint + "/.rockbox/rbutil.log", QSettings::IniFormat, nullptr);

    installlog.beginGroup(m_logsection);
    for(int i = 0; i < zipContents.size(); i++)
    {
        installlog.setValue(zipContents.at(i), m_logver);
    }
    installlog.endGroup();
    installlog.sync();

    emit cont();
}


