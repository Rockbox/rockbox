/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtGui>

#include "version.h"
#include "rbutilqt.h"
#include "ui_rbutilqtfrm.h"
#include "ui_aboutbox.h"
#include "configure.h"
#include "install.h"
#include "installtalkwindow.h"
#include "createvoicewindow.h"
#include "httpget.h"
#include "installbootloader.h"
#include "installthemes.h"
#include "uninstallwindow.h"
#include "browseof.h"
#include "utils.h"

#if defined(Q_OS_LINUX)
#include <stdio.h>
#endif
#if defined(Q_OS_WIN32)
#if defined(UNICODE)
#define _UNICODE
#endif
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#endif

RbUtilQt::RbUtilQt(QWidget *parent) : QMainWindow(parent)
{
    absolutePath = qApp->applicationDirPath();
  
    ui.setupUi(this);
    
    settings = new RbSettings();
    settings->open();
    
    // manual tab
    updateManual();
    updateDevice();
    ui.radioPdf->setChecked(true);

    // info tab
    ui.treeInfo->setAlternatingRowColors(true);
    ui.treeInfo->setHeaderLabels(QStringList() << tr("File") << tr("Version"));
    ui.treeInfo->expandAll();
    ui.treeInfo->setColumnCount(2);

    connect(ui.tabWidget, SIGNAL(currentChanged(int)), this, SLOT(updateTabs(int)));
    connect(ui.actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(ui.action_About, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui.action_Help, SIGNAL(triggered()), this, SLOT(help()));
    connect(ui.action_Configure, SIGNAL(triggered()), this, SLOT(configDialog()));
    connect(ui.buttonChangeDevice, SIGNAL(clicked()), this, SLOT(configDialog()));
    connect(ui.buttonRockbox, SIGNAL(clicked()), this, SLOT(installBtn()));
    connect(ui.buttonBootloader, SIGNAL(clicked()), this, SLOT(installBootloaderBtn()));
    connect(ui.buttonFonts, SIGNAL(clicked()), this, SLOT(installFontsBtn()));
    connect(ui.buttonGames, SIGNAL(clicked()), this, SLOT(installDoomBtn()));
    connect(ui.buttonTalk, SIGNAL(clicked()), this, SLOT(createTalkFiles()));
    connect(ui.buttonCreateVoice, SIGNAL(clicked()), this, SLOT(createVoiceFile()));
    connect(ui.buttonVoice, SIGNAL(clicked()), this, SLOT(installVoice()));
    connect(ui.buttonThemes, SIGNAL(clicked()), this, SLOT(installThemes()));
    connect(ui.buttonRemoveRockbox, SIGNAL(clicked()), this, SLOT(uninstall()));
    connect(ui.buttonRemoveBootloader, SIGNAL(clicked()), this, SLOT(uninstallBootloader()));
    connect(ui.buttonDownloadManual, SIGNAL(clicked()), this, SLOT(downloadManual()));
    connect(ui.buttonSmall, SIGNAL(clicked()), this, SLOT(smallInstall()));
    connect(ui.buttonComplete, SIGNAL(clicked()), this, SLOT(completeInstall()));

    // actions accessible from the menu
    connect(ui.actionComplete_Installation, SIGNAL(triggered()), this, SLOT(completeInstall()));
    connect(ui.actionSmall_Installation, SIGNAL(triggered()), this, SLOT(smallInstall()));
    connect(ui.actionInstall_Bootloader, SIGNAL(triggered()), this, SLOT(installBootloaderBtn()));
    connect(ui.actionInstall_Rockbox, SIGNAL(triggered()), this, SLOT(installBtn()));
    connect(ui.actionFonts_Package, SIGNAL(triggered()), this, SLOT(installFontsBtn()));
    connect(ui.actionInstall_Themes, SIGNAL(triggered()), this, SLOT(installThemes()));
    connect(ui.actionInstall_Game_Files, SIGNAL(triggered()), this, SLOT(installDoomBtn()));
    connect(ui.actionInstall_Voice_File, SIGNAL(triggered()), this, SLOT(installVoice()));
    connect(ui.actionCreate_Voice_File, SIGNAL(triggered()), this, SLOT(createVoiceFile()));
    connect(ui.actionCreate_Talk_Files, SIGNAL(triggered()), this, SLOT(createTalkFiles()));
    connect(ui.actionRemove_bootloader, SIGNAL(triggered()), this, SLOT(uninstallBootloader()));
    connect(ui.actionUninstall_Rockbox, SIGNAL(triggered()), this, SLOT(uninstall()));

#if !defined(STATIC)
    ui.actionInstall_Rockbox_Utility_on_player->setEnabled(false);
#else
    connect(ui.actionInstall_Rockbox_Utility_on_player, SIGNAL(triggered()), this, SLOT(installPortable()));
#endif

    initIpodpatcher();
    initSansapatcher();
    downloadInfo();

}


void RbUtilQt::updateTabs(int count)
{
    switch(count) {
        case 6:
            updateInfo();
            break;
        default:
            break;
    }
}


void RbUtilQt::downloadInfo()
{
    // try to get the current build information
    daily = new HttpGet(this);
    connect(daily, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
    connect(daily, SIGNAL(requestFinished(int, bool)), this, SLOT(downloadDone(int, bool)));
    connect(qApp, SIGNAL(lastWindowClosed()), daily, SLOT(abort()));
    daily->setProxy(proxy());
    if(settings->cacheOffline())
        daily->setCache(settings->cachePath());
    qDebug() << "downloading build info";
    daily->setFile(&buildInfo);
    daily->getFile(QUrl(settings->serverConfUrl()));
}


void RbUtilQt::downloadDone(bool error)
{
    if(error) {
        qDebug() << "network error:" << daily->error();
        return;
    }
    qDebug() << "network status:" << daily->error();

    buildInfo.open();
    QSettings info(buildInfo.fileName(), QSettings::IniFormat, this);
    buildInfo.close();
    versmap.insert("arch_rev", info.value("dailies/rev").toString());
    versmap.insert("arch_date", info.value("dailies/date").toString());

    bleeding = new HttpGet(this);
    connect(bleeding, SIGNAL(done(bool)), this, SLOT(downloadBleedingDone(bool)));
    connect(bleeding, SIGNAL(requestFinished(int, bool)), this, SLOT(downloadDone(int, bool)));
    connect(qApp, SIGNAL(lastWindowClosed()), daily, SLOT(abort()));
    bleeding->setProxy(proxy());
    if(settings->cacheOffline())
        bleeding->setCache(settings->cachePath());
    bleeding->setFile(&bleedingInfo);
    bleeding->getFile(QUrl(settings->bleedingInfo()));

    if(chkConfig(false)) {
        QApplication::processEvents();
        QMessageBox::critical(this, tr("Configuration error"),
            tr("Your configuration is invalid. This is most likely due "
                "to a new installation of Rockbox Utility or a changed device "
                "path. The configuation dialog will now open to allow you "
                "correcting the problem."));
        configDialog();
    }
}


void RbUtilQt::downloadBleedingDone(bool error)
{
    if(error) qDebug() << "network error:" << bleeding->error();

    bleedingInfo.open();
    QSettings info(bleedingInfo.fileName(), QSettings::IniFormat, this);
    bleedingInfo.close();
    versmap.insert("bleed_rev", info.value("bleeding/rev").toString());
    versmap.insert("bleed_date", info.value("bleeding/timestamp").toString());
    qDebug() << "versmap =" << versmap;
}


void RbUtilQt::downloadDone(int id, bool error)
{
    QString errorString;
    errorString = tr("Network error: %1. Please check your network and proxy settings.")
        .arg(daily->errorString());
    if(error) {
        QMessageBox::about(this, "Network Error", errorString);
    }
    qDebug() << "downloadDone:" << id << error;
}


void RbUtilQt::about()
{
    QDialog *window = new QDialog(this);
    Ui::aboutBox about;
    about.setupUi(window);
    window->setModal(true);

    QFile licence(":/docs/gpl-2.0.html");
    licence.open(QIODevice::ReadOnly);
    QTextStream c(&licence);
    QString cline = c.readAll();
    about.browserLicense->insertHtml(cline);
    about.browserLicense->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
    QFile credits(":/docs/CREDITS");
    credits.open(QIODevice::ReadOnly);
    QTextStream r(&credits);
    QString rline = r.readAll();
    about.browserCredits->insertPlainText(rline);
    about.browserCredits->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
    QString title = QString("<b>The Rockbox Utility</b><br/>Version %1").arg(VERSION);
    about.labelTitle->setText(title);
    about.labelHomepage->setText("<a href='http://www.rockbox.org'>http://www.rockbox.org</a>");

    window->show();

}


void RbUtilQt::help()
{
    QUrl helpurl("http://www.rockbox.org/wiki/RockboxUtility");
    QDesktopServices::openUrl(helpurl);
}


void RbUtilQt::configDialog()
{
    Config *cw = new Config(this);
    cw->setSettings(settings);
    cw->show();
    connect(cw, SIGNAL(settingsUpdated()), this, SLOT(downloadInfo()));
    connect(cw, SIGNAL(settingsUpdated()), this, SLOT(updateSettings()));
}


void RbUtilQt::updateSettings()
{
    qDebug() << "updateSettings()";
    updateDevice();
    updateManual();
}


void RbUtilQt::updateDevice()
{
    if(!settings->curNeedsBootloader() ) {
        ui.buttonBootloader->setEnabled(false);
        ui.buttonRemoveBootloader->setEnabled(false);
        ui.labelBootloader->setEnabled(false);
        ui.labelRemoveBootloader->setEnabled(false);
    }
    else {
        ui.buttonBootloader->setEnabled(true);
        ui.labelBootloader->setEnabled(true);
        if(settings->curBootloaderMethod() == "fwpatcher") {
            ui.labelRemoveBootloader->setEnabled(false);
            ui.buttonRemoveBootloader->setEnabled(false);
        }
        else {
            ui.labelRemoveBootloader->setEnabled(true);
            ui.buttonRemoveBootloader->setEnabled(true);
        }
    }
 
    // displayed device info
    QString mountpoint = settings->mountpoint();
    QString brand = settings->curBrand();
    QString name = settings->curName();
    if(name.isEmpty()) name = "&lt;none&gt;";
    if(mountpoint.isEmpty()) mountpoint = "&lt;invalid&gt;";
    ui.labelDevice->setText(tr("<b>%1 %2</b> at <b>%3</b>")
            .arg(brand, name, mountpoint));
}


void RbUtilQt::updateManual()
{
    if(settings->curPlatform() != "")
    {
        QString manual= settings->curManual();

        if(manual == "")
            manual = "rockbox-" + settings->curPlatform();
        QString pdfmanual;
        pdfmanual = settings->manualUrl() + "/" + manual + ".pdf";
        QString htmlmanual;
        htmlmanual = settings->manualUrl() + "/" + manual + "/rockbox-build.html";
        ui.labelPdfManual->setText(tr("<a href='%1'>PDF Manual</a>")
            .arg(pdfmanual));
        ui.labelHtmlManual->setText(tr("<a href='%1'>HTML Manual (opens in browser)</a>")
            .arg(htmlmanual));
    }
    else {
        ui.labelPdfManual->setText(tr("Select a device for a link to the correct manual"));
        ui.labelHtmlManual->setText(tr("<a href='%1'>Manual Overview</a>")
            .arg("http://www.rockbox.org/manual.shtml"));

    }
}

void RbUtilQt::completeInstall()
{
    if(chkConfig(true)) return;
    if(QMessageBox::question(this, tr("Confirm Installation"),
           tr("Do you really want to make a complete Installation?"),
              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    // create logger
    logger = new ProgressLoggerGui(this);
    logger->show();

    if(smallInstallInner())
        return;

    // Fonts
    m_error = false;
    m_installed = false;
    if(!installFontsAuto())
        return;
    else
    {
        // wait for installation finished
        while(!m_installed)
           QApplication::processEvents();
    }
    if(m_error) return;
    logger->undoAbort();

    // Doom
    if(hasDoom())
    {
        m_error = false;
        m_installed = false;
        if(!installDoomAuto())
            return;
        else
        {
            // wait for installation finished
            while(!m_installed)
               QApplication::processEvents();
        }
        if(m_error) return;
    }

    // theme
    // this is a window
    // it has its own logger window,so close our.
    logger->close();
    installThemes();

}

void RbUtilQt::smallInstall()
{
    if(chkConfig(true)) return;
    if(QMessageBox::question(this, tr("Confirm Installation"),
           tr("Do you really want to make a small Installation?"),
              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    // create logger
    logger = new ProgressLoggerGui(this);
    logger->show();

    smallInstallInner();
}

bool RbUtilQt::smallInstallInner()
{
    QString mountpoint = settings->mountpoint();
    // show dialog with error if mount point is wrong
    if(!QFileInfo(mountpoint).isDir()) {
        logger->addItem(tr("Mount point is wrong!"),LOGERROR);
        logger->abort();
        return true;
    }
    // Bootloader
    if(settings->curNeedsBootloader()) 
    {
        m_error = false;
        m_installed = false;
        if(!installBootloaderAuto())
            return true;
        else
        {
            // wait for boot loader installation finished
            while(!m_installed)
                QApplication::processEvents();
        }
        if(m_error) return true;
        logger->undoAbort();
    }    

    // Rockbox
    m_error = false;
    m_installed = false;
    if(!installAuto())
        return true;
    else
    {
       // wait for installation finished
       while(!m_installed)
          QApplication::processEvents();
    }

    return false;
}

void RbUtilQt::installdone(bool error)
{
    qDebug() << "install done";
    m_installed = true;
    m_error = error;
}

void RbUtilQt::installBtn()
{
    if(chkConfig(true)) return;
    install();
}

bool RbUtilQt::installAuto()
{
    QString file = QString("%1%2/rockbox.zip")
            .arg(settings->bleedingUrl(), settings->curPlatform());

    buildInfo.open();
    QSettings info(buildInfo.fileName(), QSettings::IniFormat, this);
    buildInfo.close();

    if(settings->curReleased()) {
        // only set the keys if needed -- querying will yield an empty string
        // if not set.
        versmap.insert("rel_rev", settings->lastRelease());
        versmap.insert("rel_date", ""); // FIXME: provide the release timestamp
    }

    QString myversion = "r" + versmap.value("bleed_rev");

    ZipInstaller* installer = new ZipInstaller(this);
    installer->setUrl(file);
    installer->setProxy(proxy());
    installer->setLogSection("Rockbox (Base)");
    installer->setLogVersion(myversion);
    if(!settings->cacheDisabled())
        installer->setCache(settings->cachePath());
    installer->setMountPoint(settings->mountpoint());
    installer->install(logger);

    connect(installer, SIGNAL(done(bool)), this, SLOT(installdone(bool)));

    return true;
}

void RbUtilQt::install()
{
    Install *installWindow = new Install(this);
    installWindow->setSettings(settings);
    installWindow->setProxy(proxy());

    buildInfo.open();
    QSettings info(buildInfo.fileName(), QSettings::IniFormat, this);
    buildInfo.close();

    if(settings->curReleased()) {
        // only set the keys if needed -- querying will yield an empty string
        // if not set.
        versmap.insert("rel_rev", settings->lastRelease());
        versmap.insert("rel_date", ""); // FIXME: provide the release timestamp
    }
    installWindow->setVersionStrings(versmap);

    installWindow->show();
}

bool RbUtilQt::installBootloaderAuto()
{
    installBootloader();
    connect(blinstaller,SIGNAL(done(bool)),this,SLOT(installdone(bool)));
    return !m_error;
}

void RbUtilQt::installBootloaderBtn()
{
    if(chkConfig(true)) return;
    if(QMessageBox::question(this, tr("Confirm Installation"),
           tr("Do you really want to install the Bootloader?"),
              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    // create logger
    logger = new ProgressLoggerGui(this);
    logger->show();

    installBootloader();
}

void RbUtilQt::installBootloader()
{
    QString platform = settings->curPlatform();

    // create installer
    blinstaller  = new BootloaderInstaller(this);

    blinstaller->setMountPoint(settings->mountpoint());

    blinstaller->setProxy(proxy());
    blinstaller->setDevice(platform);
    blinstaller->setBootloaderMethod(settings->curBootloaderMethod());
    blinstaller->setBootloaderName(settings->curBootloaderName());
    blinstaller->setBootloaderBaseUrl(settings->bootloaderUrl());
    blinstaller->setBootloaderInfoUrl(settings->bootloaderInfoUrl());
    if(!blinstaller->downloadInfo())
    {
        logger->addItem(tr("Could not get the bootloader info file!"),LOGERROR);
        logger->abort();
        m_error = true;
        return;
    }

    if(blinstaller->uptodate())
    {
        int ret = QMessageBox::question(this, tr("Bootloader Installation"),
                    tr("The bootloader is already installed and up to date.\n"
                    "Do want to replace the current bootloader?"),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if(ret == QMessageBox::No)
        {
            logger->addItem(tr("Bootloader installation skipped!"), LOGINFO);
            logger->abort();
            m_installed = true;
            return;
        }
    }

    // if fwpatcher , ask for extra file
    QString offirmware;
    if(settings->curBootloaderMethod() == "fwpatcher")
    {
        BrowseOF ofbrowser(this);
        ofbrowser.setFile(settings->ofPath());
        if(ofbrowser.exec() == QDialog::Accepted)
        {
            offirmware = ofbrowser.getFile();
            qDebug() << offirmware;
            if(!QFileInfo(offirmware).exists())
            {
                logger->addItem(tr("Original Firmware Path is wrong!"),LOGERROR);
                logger->abort();
                m_error = true;
                return;
            }
            else
            {
                settings->setOfPath(offirmware);
                settings->sync();
            }
        }
        else
        {
            logger->addItem(tr("Original Firmware selection Canceled!"),LOGERROR);
            logger->abort();
            m_error = true;
            return;
        }
    }
    blinstaller->setOrigFirmwarePath(offirmware);

    blinstaller->install(logger);
}

void RbUtilQt::installFontsBtn()
{
    if(chkConfig(true)) return;
    if(QMessageBox::question(this, tr("Confirm Installation"),
           tr("Do you really want to install the fonts package?"),
              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    // create logger
    logger = new ProgressLoggerGui(this);
    logger->show();
    installFonts();
}

bool RbUtilQt::installFontsAuto()
{
    installFonts();
    connect(installer, SIGNAL(done(bool)), this, SLOT(installdone(bool)));
    return !m_error;
}

void RbUtilQt::installFonts()
{
    // create zip installer
    installer = new ZipInstaller(this);

    installer->setUrl(settings->fontUrl());
    installer->setProxy(proxy());
    installer->setLogSection("Fonts");
    installer->setLogVersion(versmap.value("arch_date"));
    installer->setMountPoint(settings->mountpoint());
    if(!settings->cacheDisabled())
        installer->setCache(settings->cachePath());
    installer->install(logger);
}


void RbUtilQt::installVoice()
{
    if(chkConfig(true)) return;
    if(QMessageBox::question(this, tr("Confirm Installation"),
       tr("Do you really want to install the voice file?"),
       QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    // create logger
    logger = new ProgressLoggerGui(this);
    logger->show();

    // create zip installer
    installer = new ZipInstaller(this);
    installer->setUnzip(false);

    QString voiceurl = settings->voiceUrl()  + "/" ;
    
    voiceurl += settings->curVoiceName() + "-" +
        versmap.value("arch_date") + "-english.voice"; 
    qDebug() << voiceurl;

    installer->setProxy(proxy());
    installer->setUrl(voiceurl);
    installer->setLogSection("Voice");
    installer->setLogVersion(versmap.value("arch_date"));
    installer->setMountPoint(settings->mountpoint());
    installer->setTarget("/.rockbox/langs/english.voice");
    if(!settings->cacheDisabled())
        installer->setCache(settings->cachePath());
    installer->install(logger);

}

void RbUtilQt::installDoomBtn()
{
    if(chkConfig(true)) return;
    if(!hasDoom()){
        QMessageBox::critical(this, tr("Error"), tr("Your device doesn't have a doom plugin. Aborting."));
        return;
    }

    if(QMessageBox::question(this, tr("Confirm Installation"),
           tr("Do you really want to install the game addon files?"),
           QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    // create logger
    logger = new ProgressLoggerGui(this);
    logger->show();

    installDoom();
}
bool RbUtilQt::installDoomAuto()
{
    installDoom();
    connect(installer, SIGNAL(done(bool)), this, SLOT(installdone(bool)));
    return !m_error;
}

bool RbUtilQt::hasDoom()
{
    QFile doomrock(settings->mountpoint() +"/.rockbox/rocks/games/doom.rock");
    return doomrock.exists();
}

void RbUtilQt::installDoom()
{
    // create zip installer
    installer = new ZipInstaller(this);

    installer->setUrl(settings->doomUrl());
    installer->setProxy(proxy());
    installer->setLogSection("Game Addons");
    installer->setLogVersion(versmap.value("arch_date"));
    installer->setMountPoint(settings->mountpoint());
    if(!settings->cacheDisabled())
        installer->setCache(settings->cachePath());
    installer->install(logger);

}

void RbUtilQt::installThemes()
{
    if(chkConfig(true)) return;
    ThemesInstallWindow* tw = new ThemesInstallWindow(this);
    tw->setSettings(settings);
    tw->setProxy(proxy());
    tw->setModal(true);
    tw->show();
}

void RbUtilQt::createTalkFiles(void)
{
    if(chkConfig(true)) return;
    InstallTalkWindow *installWindow = new InstallTalkWindow(this);
    installWindow->setSettings(settings);
    installWindow->show();
    connect(installWindow, SIGNAL(settingsUpdated()), this, SLOT(downloadInfo()));
    connect(installWindow, SIGNAL(settingsUpdated()), this, SLOT(updateSettings()));

}

void RbUtilQt::createVoiceFile(void)
{
    if(chkConfig(true)) return;
    CreateVoiceWindow *installWindow = new CreateVoiceWindow(this);
    installWindow->setSettings(settings);
    installWindow->setProxy(proxy());
    
    installWindow->show();
    connect(installWindow, SIGNAL(settingsUpdated()), this, SLOT(downloadInfo()));
    connect(installWindow, SIGNAL(settingsUpdated()), this, SLOT(updateSettings()));
}

void RbUtilQt::uninstall(void)
{
    if(chkConfig(true)) return;
    UninstallWindow *uninstallWindow = new UninstallWindow(this);
    uninstallWindow->setSettings(settings);
    uninstallWindow->show();

}

void RbUtilQt::uninstallBootloader(void)
{
    if(chkConfig(true)) return;
    if(QMessageBox::question(this, tr("Confirm Uninstallation"),
           tr("Do you really want to uninstall the Bootloader?"),
           QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    // create logger
    ProgressLoggerGui* logger = new ProgressLoggerGui(this);
    logger->show();

    BootloaderInstaller blinstaller(this);
    blinstaller.setProxy(proxy());
    blinstaller.setMountPoint(settings->mountpoint());
    blinstaller.setDevice(settings->curPlatform());
    blinstaller.setBootloaderMethod(settings->curBootloaderMethod());
    blinstaller.setBootloaderName(settings->curBootloaderName());
    blinstaller.setBootloaderBaseUrl(settings->bootloaderUrl());
    blinstaller.setBootloaderInfoUrl(settings->bootloaderInfoUrl());
    if(!blinstaller.downloadInfo())
    {
        logger->addItem(tr("Could not get the bootloader info file!"),LOGERROR);
        logger->abort();
        return;
    }

    blinstaller.uninstall(logger);

}


void RbUtilQt::downloadManual(void)
{
    if(chkConfig(true)) return;
    if(QMessageBox::question(this, tr("Confirm download"),
       tr("Do you really want to download the manual? The manual will be saved "
            "to the root folder of your player."),
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    buildInfo.open();
    QSettings info(buildInfo.fileName(), QSettings::IniFormat, this);
    buildInfo.close();

    QString manual = settings->curManual();
   
    QString date = (info.value("dailies/date").toString());

    QString manualurl;
    QString target;
    QString section;
    if(ui.radioPdf->isChecked()) {
        target = "/" + manual + ".pdf";
        section = "Manual (PDF)";
    }
    else {
        target = "/" + manual + "-" + date + "-html.zip";
        section = "Manual (HTML)";
    }
    manualurl = settings->manualUrl() + "/" + target;
    qDebug() << "manualurl =" << manualurl;

    ProgressLoggerGui* logger = new ProgressLoggerGui(this);
    logger->show();
    installer = new ZipInstaller(this);
    installer->setMountPoint(settings->mountpoint());
    if(!settings->cacheDisabled())
        installer->setCache(settings->cachePath());
    installer->setProxy(proxy());
    installer->setLogSection(section);
    installer->setLogVersion(date);
    installer->setUrl(manualurl);
    installer->setUnzip(false);
    installer->setTarget(target);
    installer->install(logger);
}


void RbUtilQt::installPortable(void)
{
    if(QMessageBox::question(this, tr("Confirm installation"),
       tr("Do you really want to install Rockbox Utility to your player? "
        "After installation you can run it from the players hard drive."),
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    ProgressLoggerGui* logger = new ProgressLoggerGui(this);
    logger->setProgressMax(0);
    logger->setProgressValue(0);
    logger->show();
    logger->addItem(tr("Installing Rockbox Utility"), LOGINFO);

    // check mountpoint
    if(!QFileInfo(settings->mountpoint()).isDir()) {
        logger->addItem(tr("Mount point is wrong!"),LOGERROR);
        logger->abort();
        return;
    }

    // remove old files first.
    QFile::remove(settings->mountpoint() + "/RockboxUtility.exe");
    QFile::remove(settings->mountpoint() + "/RockboxUtility.ini");
    // copy currently running binary and currently used settings file
    if(!QFile::copy(qApp->applicationFilePath(), settings->mountpoint() + "/RockboxUtility.exe")) {
        logger->addItem(tr("Error installing Rockbox Utility"), LOGERROR);
        logger->abort();
        return;
    }
    logger->addItem(tr("Installing user configuration"), LOGINFO);
    if(!QFile::copy(settings->userSettingFilename(), settings->mountpoint() + "/RockboxUtility.ini")) {
        logger->addItem(tr("Error installing user configuration"), LOGERROR);
        logger->abort();
        return;
    }
    logger->addItem(tr("Successfully installed Rockbox Utility."), LOGOK);
    logger->abort();
    logger->setProgressMax(1);
    logger->setProgressValue(1);

}


void RbUtilQt::updateInfo()
{
    qDebug() << "RbUtilQt::updateInfo()";

    QSettings log(settings->mountpoint() + "/.rockbox/rbutil.log", QSettings::IniFormat, this);
    QStringList groups = log.childGroups();
    QList<QTreeWidgetItem *> items;
    QTreeWidgetItem *w, *w2;
    QString min, max;
    int olditems = 0;

    // remove old list entries (if any)
    int l = ui.treeInfo->topLevelItemCount();
    while(l--) {
        QTreeWidgetItem *m;
        m = ui.treeInfo->takeTopLevelItem(l);
        // delete childs (single level deep, no recursion here)
        int n = m->childCount();
        while(n--)
            delete m->child(n);
    }
    // get and populate new items
    for(int a = 0; a < groups.size(); a++) {
        log.beginGroup(groups.at(a));
        QStringList keys = log.allKeys();
        w = new QTreeWidgetItem;
        w->setFlags(Qt::ItemIsEnabled);
        w->setText(0, groups.at(a));
        items.append(w);
        // get minimum and maximum version information so we can hilight old files
        min = max = log.value(keys.at(0)).toString();
        for(int b = 0; b < keys.size(); b++) {
            if(log.value(keys.at(b)).toString() > max)
                max = log.value(keys.at(b)).toString();
            if(log.value(keys.at(b)).toString() < min)
                min = log.value(keys.at(b)).toString();
        }

        for(int b = 0; b < keys.size(); b++) {
            QString file;
            file = settings->mountpoint() + "/" + keys.at(b);
            if(QFileInfo(file).isDir())
                continue;
            w2 = new QTreeWidgetItem(w, QStringList() << "/"
                    + keys.at(b) << log.value(keys.at(b)).toString());
            if(log.value(keys.at(b)).toString() != max) {
                w2->setForeground(0, QBrush(QColor(255, 0, 0)));
                w2->setForeground(1, QBrush(QColor(255, 0, 0)));
                olditems++;
            }
            items.append(w2);
        }
        log.endGroup();
        if(min != max)
            w->setData(1, Qt::DisplayRole, QString("%1 / %2").arg(min, max));
        else
            w->setData(1, Qt::DisplayRole, max);
    }
    ui.treeInfo->insertTopLevelItems(0, items);
    ui.treeInfo->resizeColumnToContents(0);
}


QUrl RbUtilQt::proxy()
{
    if(settings->proxyType() == "manual")
        return QUrl(settings->proxy());
    else if(settings->proxy() == "system")
    {    
        systemProxy();
    }
    return QUrl("");
}


bool RbUtilQt::chkConfig(bool warn)
{
    bool error = false;
    if(settings->curPlatform().isEmpty()
        || settings->mountpoint().isEmpty()
        || !QFileInfo(settings->mountpoint()).isWritable()) {
        error = true;

        if(warn) QMessageBox::critical(this, tr("Configuration error"),
            tr("Your configuration is invalid. Please go to the configuration "
                "dialog and make sure the selected values are correct."));
    }
    return error;
}

