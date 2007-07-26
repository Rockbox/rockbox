/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
 *   $Id: installrb.cpp 13990 2007-07-25 22:26:10Z Dominik Wenger $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
#include "installrb.h"

#include "zip/zip.h"
#include "zip/unzip.h"

RBInstaller::RBInstaller(QObject* parent): QObject(parent) 
{

}


void RBInstaller::install(QString url,QString file,QString mountpoint, QUrl proxy,Ui::InstallProgressFrm* dp)
{
    m_url=url;
    m_mountpoint = mountpoint;
    m_file = file;
    m_dp = dp;

    m_dp->listProgress->addItem(tr("Downloading file %1.%2")
            .arg(QFileInfo(m_url).baseName(), QFileInfo(m_url).completeSuffix()));

    // temporary file needs to be opened to get the filename
    downloadFile.open();
    m_file = downloadFile.fileName();
    downloadFile.close();
    // get the real file.
    getter = new HttpGet(this);
    getter->setProxy(proxy);
    getter->setFile(&downloadFile);
    getter->getFile(QUrl(url));

    connect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
    connect(getter, SIGNAL(downloadDone(int, bool)), this, SLOT(downloadRequestFinished(int, bool)));
    connect(getter, SIGNAL(dataReadProgress(int, int)), this, SLOT(updateDataReadProgress(int, int)));
    
}

void RBInstaller::downloadRequestFinished(int id, bool error)
{
    qDebug() << "Install::downloadRequestFinished" << id << error;
    qDebug() << "error:" << getter->errorString();

    downloadDone(error);
}

void RBInstaller::downloadDone(bool error)
{
    qDebug() << "Install::downloadDone, error:" << error;

    
     // update progress bar
    int max = m_dp->progressBar->maximum();
    if(max == 0) {
        max = 100;
        m_dp->progressBar->setMaximum(max);
    }
    m_dp->progressBar->setValue(max);
    if(getter->httpResponse() != 200) {
        m_dp->listProgress->addItem(tr("Download error: received HTTP error %1.").arg(getter->httpResponse()));
        m_dp->buttonAbort->setText(tr("&Ok"));
        emit done(true);
        return;
    }
    if(error) {
        m_dp->listProgress->addItem(tr("Download error: %1").arg(getter->errorString()));
        m_dp->buttonAbort->setText(tr("&Ok"));
        emit done(true);
        return;
    }
    else m_dp->listProgress->addItem(tr("Download finished."));

    // unzip downloaded file
    qDebug() << "about to unzip the downloaded file" << m_file << "to" << m_mountpoint;

    m_dp->listProgress->addItem(tr("Extracting file."));
    
    qDebug() << "file to unzip: " << m_file;
    UnZip::ErrorCode ec;
    UnZip uz;
    ec = uz.openArchive(m_file);
    if(ec != UnZip::Ok) {
        m_dp->listProgress->addItem(tr("Opening archive failed: %1.")
            .arg(uz.formatError(ec)));
        m_dp->buttonAbort->setText(tr("&Ok"));
        emit done(false);
        return;
    }
    
    ec = uz.extractAll(m_mountpoint);
    if(ec != UnZip::Ok) {
        m_dp->listProgress->addItem(tr("Extracting failed: %1.")
            .arg(uz.formatError(ec)));
        m_dp->buttonAbort->setText(tr("&Ok"));
        emit done(false);
        return;
    }

    m_dp->listProgress->addItem(tr("creating installation log"));
    
    QStringList zipContents = uz.fileList();
       
    QSettings installlog(m_mountpoint + "/.rockbox/rbutil.log", QSettings::IniFormat, 0); 

    installlog.beginGroup("rockboxbase");
    for(int i = 0; i < zipContents.size(); i++)
    {
        installlog.setValue(zipContents.at(i),installlog.value(zipContents.at(i),0).toInt()+1);
    }
    installlog.endGroup();
  
    // remove temporary file
    downloadFile.remove();

    m_dp->listProgress->addItem(tr("Extraction finished successfully."));
    m_dp->buttonAbort->setText(tr("&Ok"));
    
    emit done(false);
}

void RBInstaller::updateDataReadProgress(int read, int total)
{
    m_dp->progressBar->setMaximum(total);
    m_dp->progressBar->setValue(read);
    qDebug() << "progress:" << read << "/" << total;

}

