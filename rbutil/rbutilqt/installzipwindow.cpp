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
//#include "ui_installprogressfrm.h"


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
    // create logger
    logger = new ProgressLoggerGui(this);
    logger->show();
    
    // show dialog with error if mount point is wrong
    if(QFileInfo(ui.lineMountPoint->text()).isDir()) {
        mountPoint = ui.lineMountPoint->text();
        userSettings->setValue("defaults/mountpoint", mountPoint);
    }
    else {
        logger->addItem(tr("Mount point is wrong!"),LOGERROR);
        logger->abort();
        return;
    }

    userSettings->sync();

    // create Zip installer
    installer = new ZipInstaller(this);
   
    QString fileName = url.section('/', -1);
    installer->setFilename(fileName);
    installer->setUrl(url);
    installer->setProxy(proxy);
    installer->setLogSection(logsection);
    installer->setMountPoint(mountPoint);
    installer->install(logger);
    
    connect(installer, SIGNAL(done(bool)), this, SLOT(done(bool)));    
    
   
}

// we are done with Zip installing 
void InstallZipWindow::done(bool error)
{
    qDebug() << "Install::done, error:" << error;

    if(error)   // if there was an error
    {
        logger->abort();
        return;
    }
    
    // no error, close the window, when the logger is closed
    connect(logger,SIGNAL(closed()),this,SLOT(close()));
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
