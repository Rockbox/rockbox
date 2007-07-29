/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: installzip.cpp 13990 2007-07-25 22:26:10Z Dominik Wenger $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
#include "installzip.h"

#include "zip/zip.h"
#include "zip/unzip.h"

ZipInstaller::ZipInstaller(QObject* parent): QObject(parent) 
{

}


void ZipInstaller::install(ProgressloggerInterface* dp)
{
    m_dp = dp;

    m_dp->addItem(tr("Downloading file %1.%2")
            .arg(QFileInfo(m_url).baseName(), QFileInfo(m_url).completeSuffix()));

    // temporary file needs to be opened to get the filename
    downloadFile.open();
    m_file = downloadFile.fileName();
    downloadFile.close();
    // get the real file.
    getter = new HttpGet(this);
    getter->setProxy(m_proxy);
    getter->setFile(&downloadFile);
    getter->getFile(QUrl(m_url));

    connect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
    connect(getter, SIGNAL(downloadDone(int, bool)), this, SLOT(downloadRequestFinished(int, bool)));
    connect(getter, SIGNAL(dataReadProgress(int, int)), this, SLOT(updateDataReadProgress(int, int)));
    connect(m_dp, SIGNAL(aborted()), getter, SLOT(abort()));
}

void ZipInstaller::downloadRequestFinished(int id, bool error)
{
    qDebug() << "Install::downloadRequestFinished" << id << error;
    qDebug() << "error:" << getter->errorString();

    downloadDone(error);
}

void ZipInstaller::downloadDone(bool error)
{
    qDebug() << "Install::downloadDone, error:" << error;

     // update progress bar
     
    int max = m_dp->getProgressMax();
    if(max == 0) {
        max = 100;
        m_dp->setProgressMax(max);
    }
    m_dp->setProgressValue(max);
    if(getter->httpResponse() != 200) {
        m_dp->addItem(tr("Download error: received HTTP error %1.").arg(getter->httpResponse()));
        m_dp->abort();
        emit done(true);
        return;
    }
    if(error) {
        m_dp->addItem(tr("Download error: %1").arg(getter->errorString()));
        m_dp->abort();
        emit done(true);
        return;
    }
    else m_dp->addItem(tr("Download finished."));

    // unzip downloaded file
    qDebug() << "about to unzip the downloaded file" << m_file << "to" << m_mountpoint;

    m_dp->addItem(tr("Extracting file."));
    
    qDebug() << "file to unzip: " << m_file;
    UnZip::ErrorCode ec;
    UnZip uz;
    ec = uz.openArchive(m_file);
    if(ec != UnZip::Ok) {
        m_dp->addItem(tr("Opening archive failed: %1.")
            .arg(uz.formatError(ec)));
        m_dp->abort();
        emit done(false);
        return;
    }
    
    ec = uz.extractAll(m_mountpoint);
    if(ec != UnZip::Ok) {
        m_dp->addItem(tr("Extracting failed: %1.")
            .arg(uz.formatError(ec)));
        m_dp->abort();
        emit done(false);
        return;
    }

    m_dp->addItem(tr("creating installation log"));
    
    QStringList zipContents = uz.fileList();
       
    QSettings installlog(m_mountpoint + "/.rockbox/rbutil.log", QSettings::IniFormat, 0); 

    installlog.beginGroup(m_logsection);
    for(int i = 0; i < zipContents.size(); i++)
    {
        installlog.setValue(zipContents.at(i),installlog.value(zipContents.at(i),0).toInt()+1);
    }
    installlog.endGroup();
  
    // remove temporary file
    downloadFile.remove();

    m_dp->addItem(tr("Extraction finished successfully."));
    m_dp->abort();
    emit done(false);
}

void ZipInstaller::updateDataReadProgress(int read, int total)
{
    m_dp->setProgressMax(total);
    m_dp->setProgressValue(read);
    qDebug() << "progress:" << read << "/" << total;

}

