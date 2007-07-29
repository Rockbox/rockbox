/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: installbl.cpp 14027 2007-07-27 17:42:49Z domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "installbl.h"
#include "ui_installprogressfrm.h"


InstallBl::InstallBl(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    connect(ui.buttonBrowse, SIGNAL(clicked()), this, SLOT(browseFolder()));
    connect(ui.buttonBrowseOF, SIGNAL(clicked()), this, SLOT(browseOF()));
    
}

void InstallBl::setProxy(QUrl proxy_url)
{
    proxy = proxy_url;
    qDebug() << "Install::setProxy" << proxy;
}

void InstallBl::setMountPoint(QString mount)
{
    QFileInfo m(mount);
    if(m.isDir()) {
        ui.lineMountPoint->clear();
        ui.lineMountPoint->insert(mount);
    }
}

void InstallBl::setOFPath(QString path)
{
    QFileInfo m(path);
    if(m.exists()) {
        ui.lineOriginalFirmware->clear();
        ui.lineOriginalFirmware->insert(path);
    }
}

void InstallBl::browseFolder()
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

void InstallBl::browseOF()
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

void InstallBl::accept()
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
    
    if(QFileInfo(ui.lineOriginalFirmware->text()).exists())
    {
        m_OrigFirmware = ui.lineOriginalFirmware->text();
    }
    else
    {
        dp.listProgress->addItem(tr("Original Firmware Path is wrong!"));
        dp.buttonAbort->setText(tr("&Ok"));
        downloadProgress->show();
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
    
    binstaller->install(&dp);
    
    connect(binstaller, SIGNAL(done(bool)), this, SLOT(done(bool)));    
    
    downloadProgress->show();
}


void InstallBl::done(bool error)
{
    qDebug() << "Install::done, error:" << error;

    if(error)
    {
        connect(dp.buttonAbort, SIGNAL(clicked()), downloadProgress, SLOT(close()));
        return;
    }
      
    connect(dp.buttonAbort, SIGNAL(clicked()), this, SLOT(close()));
    connect(dp.buttonAbort, SIGNAL(clicked()),downloadProgress, SLOT(close()));   
}

void InstallBl::setDeviceSettings(QSettings *dev)
{
    devices = dev;
    
    if(userSettings->value("defaults/platform").toString() == "h100" ||
        userSettings->value("defaults/platform").toString() == "h120" ||
        userSettings->value("defaults/platform").toString() == "h300")
    {
        ui.buttonBrowseOF->show();
        ui.lineOriginalFirmware->show();
        ui.label_3->show();
    }
    else
    {
        ui.buttonBrowseOF->hide();
        ui.lineOriginalFirmware->hide();
        ui.label_3->hide();
    }
    qDebug() << "Install::setDeviceSettings:" << devices;
}

void InstallBl::setUserSettings(QSettings *user)
{
    userSettings = user;
}
