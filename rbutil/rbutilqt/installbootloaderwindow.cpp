/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: installbootloaderwindow.cpp 14027 2007-07-27 17:42:49Z domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "installbootloaderwindow.h"
#include "ui_installprogressfrm.h"


InstallBootloaderWindow::InstallBootloaderWindow(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    connect(ui.buttonBrowse, SIGNAL(clicked()), this, SLOT(browseFolder()));
    connect(ui.buttonBrowseOF, SIGNAL(clicked()), this, SLOT(browseOF()));
    
}

void InstallBootloaderWindow::setProxy(QUrl proxy_url)
{
    proxy = proxy_url;
    qDebug() << "Install::setProxy" << proxy;
}

void InstallBootloaderWindow::setMountPoint(QString mount)
{
    QFileInfo m(mount);
    if(m.isDir()) {
        ui.lineMountPoint->clear();
        ui.lineMountPoint->insert(mount);
    }
}

void InstallBootloaderWindow::setOFPath(QString path)
{
    QFileInfo m(path);
    if(m.exists()) {
        ui.lineOriginalFirmware->clear();
        ui.lineOriginalFirmware->insert(path);
    }
}

void InstallBootloaderWindow::browseFolder()
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

void InstallBootloaderWindow::browseOF()
{
    QFileDialog browser(this);
    if(QFileInfo(ui.lineOriginalFirmware->text()).exists())
        browser.setDirectory(ui.lineOriginalFirmware->text());
    else
        browser.setDirectory("/media");
    browser.setReadOnly(true);
    browser.setAcceptMode(QFileDialog::AcceptOpen);
    if(browser.exec()) {
        qDebug() << browser.directory();
        QStringList files = browser.selectedFiles();
        setOFPath(files.at(0));
    }
}

void InstallBootloaderWindow::accept()
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
    
    if(QFileInfo(ui.lineOriginalFirmware->text()).exists())
    {
        m_OrigFirmware = ui.lineOriginalFirmware->text();
    }
    else if(needextrafile)
    {
        logger->addItem(tr("Original Firmware Path is wrong!"),LOGERROR);
        logger->abort();
        return;
    }
    userSettings->sync();

    binstaller = new BootloaderInstaller(this);
    
    binstaller->setMountPoint(mountPoint);
    binstaller->setProxy(proxy);
    QString plattform = userSettings->value("defaults/platform").toString();
    
    binstaller->setDevice(plattform);
    binstaller->setBootloaderMethod(devices->value(plattform + "/bootloadermethod").toString());
    binstaller->setBootloaderName(devices->value(plattform + "/bootloadername").toString());
    binstaller->setBootloaderBaseUrl(devices->value("bootloader_url").toString());
    binstaller->setOrigFirmwarePath(m_OrigFirmware);
    
    binstaller->install(logger);
    
    connect(binstaller, SIGNAL(done(bool)), this, SLOT(done(bool)));    
    
}


void InstallBootloaderWindow::done(bool error)
{
    qDebug() << "Install::done, error:" << error;

    if(error)
    {
        logger->abort();
        return;
    }
    
    // no error, close the window, when the logger is closed
    connect(logger,SIGNAL(closed()),this,SLOT(close()));
   
}

void InstallBootloaderWindow::setDeviceSettings(QSettings *dev)
{
    devices = dev;
    qDebug() << "Install::setDeviceSettings:" << devices;
}

void InstallBootloaderWindow::setUserSettings(QSettings *user)
{
    userSettings = user;
    if(userSettings->value("defaults/platform").toString() == "h100" ||
           userSettings->value("defaults/platform").toString() == "h120" ||
           userSettings->value("defaults/platform").toString() == "h300")
   {
      ui.buttonBrowseOF->show();
      ui.lineOriginalFirmware->show();
      ui.label_3->show();
      needextrafile = true;
   }
   else
   {
      ui.buttonBrowseOF->hide();
      ui.lineOriginalFirmware->hide();
      ui.label_3->hide();
      needextrafile = false;
   }
}
