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

#include <QtCore>
#include "zipinstaller.h"
#include "rbunzip.h"
#include "utils.h"

ZipInstaller::ZipInstaller(QObject* parent): QObject(parent)
{
    m_unzip = true;
    m_usecache = false;
}


void ZipInstaller::install()
{
    qDebug() << "[ZipInstall] initializing installation";

    runner = 0;
    connect(this, SIGNAL(cont()), this, SLOT(installContinue()));
    m_url = m_urllist.at(runner);
    m_logsection = m_loglist.at(runner);
    m_logver = m_verlist.at(runner);
    installStart();
}


void ZipInstaller::abort()
{
    qDebug() << "[ZipInstall] Aborted";
    emit internalAborted();
}


void ZipInstaller::installContinue()
{
    qDebug() << "[ZipInstall] continuing installation";

    runner++; // this gets called when a install finished, so increase first.
    qDebug() << "[ZipInstall] runner done:" << runner << "/" << m_urllist.size();
    if(runner < m_urllist.size()) {
        emit logItem(tr("Done"), LOGOK);
        m_url = m_urllist.at(runner);
        m_logsection = m_loglist.at(runner);
        if(runner < m_verlist.size()) m_logver = m_verlist.at(runner);
        else m_logver = "0";
        installStart();
    }
    else {
        emit logItem(tr("Installation finished successfully"), LOGOK);

        emit done(false);
        return;
    }

}


void ZipInstaller::installStart()
{
    qDebug() << "[ZipInstall] starting installation";

    emit logItem(tr("Downloading file %1.%2").arg(QFileInfo(m_url).baseName(),
            QFileInfo(m_url).completeSuffix()),LOGINFO);

    // temporary file needs to be opened to get the filename
    // make sure to get a fresh one on each run.
    // making this a parent of the temporary file ensures the file gets deleted
    // after the class object gets destroyed.
    downloadFile = new QTemporaryFile(this);
    downloadFile->open();
    m_file = downloadFile->fileName();
    downloadFile->close();
    // get the real file.
    getter = new HttpGet(this);
    if(m_usecache) {
        getter->setCache(true);
    }
    getter->setFile(downloadFile);

    connect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
    connect(getter, SIGNAL(dataReadProgress(int, int)), this, SIGNAL(logProgress(int, int)));
    connect(this, SIGNAL(internalAborted()), getter, SLOT(abort()));

    getter->getFile(QUrl(m_url));
}


void ZipInstaller::downloadDone(bool error)
{
    qDebug() << "[ZipInstall] download done, error:" << error;
    QStringList zipContents; // needed later
     // update progress bar

    emit logProgress(1, 1);
    if(getter->httpResponse() != 200 && !getter->isCached()) {
        emit logItem(tr("Download error: received HTTP error %1.")
                    .arg(getter->httpResponse()),LOGERROR);
        emit done(true);
        return;
    }
    if(getter->isCached())
        emit logItem(tr("Cached file used"), LOGINFO);
    if(error) {
        emit logItem(tr("Download error: %1").arg(getter->errorString()), LOGERROR);
        emit done(true);
        return;
    }
    else emit logItem(tr("Download finished"),LOGOK);
    QCoreApplication::processEvents();
    if(m_unzip) {
        // unzip downloaded file
        qDebug() << "[ZipInstall] about to unzip " << m_file << "to" << m_mountpoint;

        emit logItem(tr("Extracting file"), LOGINFO);
        QCoreApplication::processEvents();

        UnZip::ErrorCode ec;
        RbUnZip uz;
        connect(&uz, SIGNAL(unzipProgress(int, int)), this, SIGNAL(logProgress(int, int)));
        connect(this, SIGNAL(internalAborted()), &uz, SLOT(abortUnzip()));
        ec = uz.openArchive(m_file);
        if(ec != UnZip::Ok) {
            emit logItem(tr("Opening archive failed: %1.")
                .arg(uz.formatError(ec)),LOGERROR);
            emit logProgress(1, 1);
            emit done(true);
            return;
        }

        // check for free space. Make sure after installation will still be
        // some room for operating (also includes calculation mistakes due to
        // cluster sizes on the player).
        if(filesystemFree(m_mountpoint) < (uz.totalSize() + 1000000)) {
            emit logItem(tr("Not enough disk space! Aborting."), LOGERROR);
            emit logProgress(1, 1);
            emit done(true);
            return;
        }
        ec = uz.extractArchive(m_mountpoint);
        // TODO: better handling of aborted unzip operation.
        if(ec != UnZip::Ok) {
            emit logItem(tr("Extracting failed: %1.")
                .arg(uz.formatError(ec)),LOGERROR);
            emit logProgress(1, 1);
            emit done(true);
            return;
        }
        // prepare file list for log
        zipContents = uz.fileList();
    }
    else {
        // only copy the downloaded file to the output location / name
        emit logItem(tr("Installing file."), LOGINFO);
        qDebug() << "[ZipInstall] saving downloaded file (no extraction)";

        downloadFile->open(); // copy fails if file is not opened (filename issue?)
        // make sure the required path is existing
        QString path = QFileInfo(m_mountpoint + m_target).absolutePath();
        QDir p;
        p.mkpath(path);
        // QFile::copy() doesn't overwrite files, so remove old one first
        QFile(m_mountpoint + m_target).remove();
        if(!downloadFile->copy(m_mountpoint + m_target)) {
            emit logItem(tr("Installing file failed."), LOGERROR);
            emit done(true);
            return;
        }

        // add file to log
        zipContents.append( m_target);
    }

    emit logItem(tr("Creating installation log"),LOGINFO);
    QSettings installlog(m_mountpoint + "/.rockbox/rbutil.log", QSettings::IniFormat, 0);

    installlog.beginGroup(m_logsection);
    for(int i = 0; i < zipContents.size(); i++)
    {
        installlog.setValue(zipContents.at(i), m_logver);
    }
    installlog.endGroup();
    installlog.sync();

    emit cont();
}


