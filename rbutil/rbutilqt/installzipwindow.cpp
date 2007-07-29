/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: installzipwindow.cpp 14027 2007-07-27 17:42:49Z domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "installzipwindow.h"
#include "ui_installprogressfrm.h"


InstallZipWindow::InstallZipWindow(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    connect(ui.buttonBrowse, SIGNAL(clicked()), this, SLOT(browseFolder()));   
}

void InstallZipWindow::setProxy(QUrl proxy_url)
{
    proxy = proxy_url;
    qDebug() << "Install::setProxy" << proxy;
}

void InstallZipWindow::setMountPoint(QString mount)
{
    QFileInfo m(mount);
    if(m.isDir()) {
        ui.lineMountPoint->clear();
        ui.lineMountPoint->insert(mount);
    }
}

void InstallZipWindow::setUrl(QString path)
{
   url = path;
}

void InstallZipWindow::browseFolder()
{
    QFileDialog browser(this);
    if(QFileInfo(ui.lineMountPoint->text()).isDir())
        browser.setDirectory(ui.lineMountPoint->text());
    else
        browser.setDirectory("/media");
    browser.setReadOnly(true);
    browser.setFileMode(QFileDialog::DirectoryOnly);
    browser.setAcceptMode(QFileDialog::AcceptOpen);
    if(browser.exec()) {
        qDebug() << browser.directory();
        QStringList files = browser.selectedFiles();
        setMountPoint(files.at(0));
    }
}

void InstallZipWindow::accept()
{
    downloadProgress = new QDialog(this);
    dp.setupUi(downloadProgress);
   
    // show dialog with error if mount point is wrong
    if(QFileInfo(ui.lineMountPoint->text()).isDir()) {
        mountPoint = ui.lineMountPoint->text();
        userSettings->setValue("defaults/mountpoint", mountPoint);
    }
    else {
        dp.listProgress->addItem(tr("Mount point is wrong!"));
        dp.buttonAbort->setText(tr("&Ok"));
        downloadProgress->show();
        return;
    }

    userSettings->sync();

	installer = new ZipInstaller(this);
	  
	QString fileName = url.section('/', -1);
	  
    installer->setFilename(fileName);
    installer->setUrl(url);
    installer->setProxy(proxy);
    installer->setLogSection(logsection);
    installer->setMountPoint(mountPoint);
    installer->install(&dp);
    
    connect(installer, SIGNAL(done(bool)), this, SLOT(done(bool)));    
    
    downloadProgress->show();

}


void InstallZipWindow::done(bool error)
{
    qDebug() << "Install::done, error:" << error;

    if(error)
    {
        // connect close button now as it's needed if we break upon an error
        connect(dp.buttonAbort, SIGNAL(clicked()), downloadProgress, SLOT(close()));
        return;
    }
      
    connect(dp.buttonAbort, SIGNAL(clicked()), this, SLOT(close()));
    connect(dp.buttonAbort, SIGNAL(clicked()),downloadProgress, SLOT(close()));   
}

void InstallZipWindow::setDeviceSettings(QSettings *dev)
{
    devices = dev;
    qDebug() << "Install::setDeviceSettings:" << devices;
}

void InstallZipWindow::setUserSettings(QSettings *user)
{
    userSettings = user;
}
