/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: multinstaller.cpp 14462 2007-08-26 16:44:23Z domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtGui>
#include <QtNetwork>
        
#include "multiinstaller.h"

#include "installbootloader.h"
#include "progressloggergui.h"
#include "installzip.h"
#include "browseof.h"
#include "installthemes.h"

MultiInstaller::MultiInstaller(QObject* parent): QObject(parent) 
{
    
}

void MultiInstaller::setUserSettings(QSettings* user)
{
    userSettings = user;
}
void MultiInstaller::setDeviceSettings(QSettings* dev)
{
    devices = dev;
}

void MultiInstaller::setProxy(QUrl proxy)
{
    m_proxy = proxy;
}

void MultiInstaller::installSmall()
{
    // create logger
    logger = new ProgressLoggerGui(this);
    logger->show();
              
    m_plattform = userSettings->value("defaults/platform").toString();
    m_mountpoint = userSettings->value("defaults/mountpoint").toString();
    // show dialog with error if mount point is wrong
    if(!QFileInfo(m_mountpoint).isDir()) {
        logger->addItem(tr("Mount point is wrong!"),LOGERROR);
        logger->abort();
        return;
    }  
    
    // Bootloader
    m_error = false;
    installed = false;
    if(!installBootloader())
        return;
    else
    {
        // wait for boot loader installation finished
        while(!installed)
            QApplication::processEvents();
    }
    if(m_error) return;
    logger->undoAbort();
       
    // Rockbox
    m_error = false;
    installed = false;
    if(!installRockbox())
         return;
    else
    {
        // wait for boot loader installation finished
        while(!installed)
            QApplication::processEvents();
    }       
}
    
void MultiInstaller::installComplete()
{    
    // create logger
    logger = new ProgressLoggerGui(this);
    logger->show();
                 
    m_plattform = userSettings->value("defaults/platform").toString();
    m_mountpoint = userSettings->value("defaults/mountpoint").toString();
    // show dialog with error if mount point is wrong
    if(!QFileInfo(m_mountpoint).isDir()) {
         logger->addItem(tr("Mount point is wrong!"),LOGERROR);
         logger->abort();
         return;
    }  
    // Bootloader
    m_error = false;
    installed = false;
    if(!installBootloader())
        return;
    else
    {
        // wait for boot loader installation finished
        while(!installed)
            QApplication::processEvents();
    }
    if(m_error) return;
    logger->undoAbort();
    
    // Rockbox
    m_error = false;
    installed = false;
    if(!installRockbox())
        return;
    else
    {
        // wait for boot loader installation finished
        while(!installed)
            QApplication::processEvents();
    }
    if(m_error) return;
    logger->undoAbort();
    
    // Fonts
    m_error = false;
    installed = false;
    if(!installFonts())
          return;
    else
    {
        // wait for boot loader installation finished
        while(!installed)
          QApplication::processEvents();
    }
    if(m_error) return;
    logger->undoAbort();
    
    // Doom
    m_error = false;
    installed = false;
    if(!installDoom())
          return;
    else
    {
        // wait for boot loader installation finished
        while(!installed)
          QApplication::processEvents();
    }
    if(m_error) return;
    
    
    // theme
    // this is a window
    // it has its own logger window,so close our.
    logger->close();
    installThemes();
    
}


void MultiInstaller::installdone(bool error)
{
    installed = true;
    m_error = error;
}


void MultiInstaller::setVersionStrings(QMap<QString, QString> ver)
{
    version = ver;
    // version strings map is as following:
    // rel_rev release version revision id
    // rel_date release version release date
    // same for arch_* and bleed_*
    
   qDebug() << "Install::setVersionStrings" << version;
}

bool MultiInstaller::installBootloader()
{
    BootloaderInstaller* blinstaller = new BootloaderInstaller(this);
    blinstaller->setMountPoint(m_mountpoint);
    blinstaller->setProxy(m_proxy);
    blinstaller->setDevice(m_plattform);
    blinstaller->setBootloaderMethod(devices->value(m_plattform + "/bootloadermethod").toString());
    blinstaller->setBootloaderName(devices->value(m_plattform + "/bootloadername").toString());
    blinstaller->setBootloaderBaseUrl(devices->value("bootloader_url").toString());
    blinstaller->setBootloaderInfoUrl(devices->value("bootloader_info_url").toString());
    if(!blinstaller->downloadInfo())
    {
         logger->addItem(tr("Could not get the bootloader info file!"),LOGERROR);
         logger->abort();
         return false;
    }
           
    if(blinstaller->uptodate())
    {
         int ret = QMessageBox::question(NULL, tr("Bootloader Installation"),
                               tr("It seem your Bootloader is already uptodate.\n"
                                  "Do really want to install it?"),
                               QMessageBox::Ok | QMessageBox::Ignore |
                               QMessageBox::Cancel,
                               QMessageBox::Ignore);
         if(ret == QMessageBox::Cancel)
         {
             logger->addItem(tr("Bootloader installation Canceled!"),LOGERROR);
             logger->abort();
             return false;
         }      
         else if(ret == QMessageBox::Ignore)
         {
             // skip to next install step
             logger->addItem(tr("Skipped Bootloader installation!"),LOGWARNING);
             installed = true;
             return true;
         }             
     }
           
     // if fwpatcher , ask for extra file
     QString offirmware;
     if(devices->value(m_plattform + "/bootloadermethod").toString() == "fwpatcher")
     {
         BrowseOF ofbrowser(NULL);
         ofbrowser.setFile(userSettings->value("defaults/ofpath").toString());
         if(ofbrowser.exec() == QDialog::Accepted)
         {
             offirmware = ofbrowser.getFile();
             qDebug() << offirmware;
             if(!QFileInfo(offirmware).exists())
             {
                logger->addItem(tr("Original Firmware Path is wrong!"),LOGERROR);
                logger->abort();
                return false;
             }
             else
             {
                 userSettings->setValue("defaults/ofpath",offirmware);
                 userSettings->sync();
             }
         }
         else
         {
             logger->addItem(tr("Original Firmware selection Canceled!"),LOGERROR);
             logger->abort();
             return false;
         }
     }
     blinstaller->setOrigFirmwarePath(offirmware);
         
     blinstaller->install(logger);
     connect(blinstaller,SIGNAL(done(bool)),this,SLOT(installdone(bool)));
     return true;
}

bool MultiInstaller::installRockbox()
{
    QString file = QString("%1%2/rockbox.zip")
        .arg(devices->value("bleeding_url").toString(),
    userSettings->value("defaults/platform").toString());
    QString myversion = "r" + version.value("bleed_rev");
    
    ZipInstaller* installer = new ZipInstaller(this);
    installer->setUrl(file);
    installer->setProxy(m_proxy);
    installer->setLogSection("rockboxbase");
    installer->setLogVersion(myversion);
    installer->setMountPoint(m_mountpoint);
    installer->install(logger);
       
    connect(installer, SIGNAL(done(bool)), this, SLOT(installdone(bool)));
    
    return true;
}

bool MultiInstaller::installFonts()
{       
    // create zip installer
    ZipInstaller* installer = new ZipInstaller(this);
           
    installer->setUrl(devices->value("font_url").toString());
    installer->setProxy(m_proxy);
    installer->setLogSection("Fonts");
    installer->setLogVersion(version.value("arch_date"));
    installer->setMountPoint(m_mountpoint);
    installer->install(logger);
        
    connect(installer, SIGNAL(done(bool)), this, SLOT(installdone(bool)));
    
    return true;
}

bool MultiInstaller::installDoom()
{
    // create zip installer
    ZipInstaller* installer = new ZipInstaller(this);
      
    installer->setUrl(devices->value("doom_url").toString());
    installer->setProxy(m_proxy);
    installer->setLogSection("Game Addons");
    installer->setLogVersion(version.value("arch_date"));
    installer->setMountPoint(m_mountpoint);
    installer->install(logger);
    
    connect(installer, SIGNAL(done(bool)), this, SLOT(installdone(bool)));
        
    return true;
}

bool MultiInstaller::installThemes()
{
    ThemesInstallWindow* tw = new ThemesInstallWindow(NULL);
    tw->setDeviceSettings(devices);
    tw->setUserSettings(userSettings);
    tw->setProxy(m_proxy);
    tw->setModal(true);
    tw->show();
    
    return true;
}


