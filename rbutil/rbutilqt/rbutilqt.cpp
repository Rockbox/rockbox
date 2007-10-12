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
#include "httpget.h"
#include "installbootloader.h"
#include "installthemes.h"
#include "uninstallwindow.h"
#include "browseof.h"

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
    // use built-in rbutil.ini if no external file in binary folder
    QString iniFile = absolutePath + "/rbutil.ini";
    if(QFileInfo(iniFile).isFile()) {
        qDebug() << "using external rbutil.ini";
        devices = new QSettings(iniFile, QSettings::IniFormat, 0);
    }
    else {
        qDebug() << "using built-in rbutil.ini";
        devices = new QSettings(":/ini/rbutil.ini", QSettings::IniFormat, 0);
    }

    ui.setupUi(this);

    // portable installation:
    // check for a configuration file in the program folder.
    QFileInfo config;
    config.setFile(absolutePath + "/RockboxUtility.ini");
    if(config.isFile()) {
        userSettings = new QSettings(absolutePath + "/RockboxUtility.ini",
            QSettings::IniFormat, 0);
        qDebug() << "config: portable";
    }
    else {
        userSettings = new QSettings(QSettings::IniFormat,
            QSettings::UserScope, "rockbox.org", "RockboxUtility");
        qDebug() << "config: system";
    }

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
    connect(ui.buttonVoice, SIGNAL(clicked()), this, SLOT(installVoice()));
    connect(ui.buttonThemes, SIGNAL(clicked()), this, SLOT(installThemes()));
    connect(ui.buttonRemoveRockbox, SIGNAL(clicked()), this, SLOT(uninstall()));
    connect(ui.buttonRemoveBootloader, SIGNAL(clicked()), this, SLOT(uninstallBootloader()));
    connect(ui.buttonDownloadManual, SIGNAL(clicked()), this, SLOT(downloadManual()));
    connect(ui.buttonSmall, SIGNAL(clicked()), this, SLOT(smallInstall()));
    connect(ui.buttonComplete, SIGNAL(clicked()), this, SLOT(completeInstall()));

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
    if(userSettings->value("offline").toBool())
        daily->setCache(userSettings->value("cachepath", QDir::tempPath()).toString());
    qDebug() << "downloading build info";
    daily->setFile(&buildInfo);
    daily->getFile(QUrl(devices->value("server_conf_url").toString()));
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
    if(userSettings->value("offline").toBool())
        bleeding->setCache(userSettings->value("cachepath", QDir::tempPath()).toString());
    bleeding->setFile(&bleedingInfo);
    bleeding->getFile(QUrl(devices->value("bleeding_info").toString()));

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
    QUrl helpurl("http://www.rockbox.org/wiki/RockboxUtilityQt");
    QDesktopServices::openUrl(helpurl);
}


void RbUtilQt::configDialog()
{
    Config *cw = new Config(this);
    cw->setUserSettings(userSettings);
    cw->setDevices(devices);
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
    platform = userSettings->value("platform").toString();
    // buttons
    devices->beginGroup(platform);
    if(devices->value("needsbootloader", "") == "no") {
        ui.buttonBootloader->setEnabled(false);
        ui.buttonRemoveBootloader->setEnabled(false);
        ui.labelBootloader->setEnabled(false);
        ui.labelRemoveBootloader->setEnabled(false);
    }
    else {
        ui.buttonBootloader->setEnabled(true);
        ui.labelBootloader->setEnabled(true);
        if(devices->value("bootloadermethod") == "fwpatcher") {
            ui.labelRemoveBootloader->setEnabled(false);
            ui.buttonRemoveBootloader->setEnabled(false);
        }
        else {
            ui.labelRemoveBootloader->setEnabled(true);
            ui.buttonRemoveBootloader->setEnabled(true);
        }
    }
    devices->endGroup();
    // displayed device info
    platform = userSettings->value("platform").toString();
    QString mountpoint = userSettings->value("mountpoint").toString();
    devices->beginGroup(platform);
    QString brand = devices->value("brand").toString();
    QString name = devices->value("name").toString();
    devices->endGroup();
    if(name.isEmpty()) name = "&lt;none&gt;";
    if(mountpoint.isEmpty()) mountpoint = "&lt;invalid&gt;";
    ui.labelDevice->setText(tr("<b>%1 %2</b> at <b>%3</b>")
            .arg(brand, name, mountpoint));
}


void RbUtilQt::updateManual()
{
    if(userSettings->value("platform").toString() != "")
    {
        devices->beginGroup(userSettings->value("platform").toString());
        QString manual;
        manual = devices->value("manualname", "").toString();

        if(manual == "")
            manual = "rockbox-" + devices->value("platform").toString();
        devices->endGroup();
        QString pdfmanual;
        pdfmanual = devices->value("manual_url").toString() + "/" + manual + ".pdf";
        QString htmlmanual;
        htmlmanual = devices->value("manual_url").toString() + "/" + manual + "/rockbox-build.html";
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
    QString mountpoint = userSettings->value("mountpoint").toString();
    // show dialog with error if mount point is wrong
    if(!QFileInfo(mountpoint).isDir()) {
        logger->addItem(tr("Mount point is wrong!"),LOGERROR);
        logger->abort();
        return true;
    }
    // Bootloader
    devices->beginGroup(userSettings->value("platform").toString());
    if(devices->value("needsbootloader", "") == "yes") {
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
    devices->endGroup();

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
            .arg(devices->value("bleeding_url").toString(),
        userSettings->value("platform").toString());

    buildInfo.open();
    QSettings info(buildInfo.fileName(), QSettings::IniFormat, this);
    buildInfo.close();

    devices->beginGroup(platform);
    QString released = devices->value("released").toString();
    devices->endGroup();
    if(released == "yes") {
        // only set the keys if needed -- querying will yield an empty string
        // if not set.
        versmap.insert("rel_rev", devices->value("last_release").toString());
        versmap.insert("rel_date", ""); // FIXME: provide the release timestamp
    }

    QString myversion = "r" + versmap.value("bleed_rev");

    ZipInstaller* installer = new ZipInstaller(this);
    installer->setUrl(file);
    installer->setProxy(proxy());
    installer->setLogSection("Rockbox (Base)");
    installer->setLogVersion(myversion);
    if(!userSettings->value("cachedisable").toBool())
        installer->setCache(userSettings->value("cachepath", QDir::tempPath()).toString());
    installer->setMountPoint(userSettings->value("mountpoint").toString());
    installer->install(logger);

    connect(installer, SIGNAL(done(bool)), this, SLOT(installdone(bool)));

    return true;
}

void RbUtilQt::install()
{
    Install *installWindow = new Install(this);
    installWindow->setUserSettings(userSettings);
    installWindow->setDeviceSettings(devices);
    installWindow->setProxy(proxy());

    buildInfo.open();
    QSettings info(buildInfo.fileName(), QSettings::IniFormat, this);
    buildInfo.close();

    devices->beginGroup(platform);
    QString released = devices->value("released").toString();
    devices->endGroup();
    if(released == "yes") {
        // only set the keys if needed -- querying will yield an empty string
        // if not set.
        versmap.insert("rel_rev", devices->value("last_release").toString());
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
    QString platform = userSettings->value("platform").toString();

    // create installer
    blinstaller  = new BootloaderInstaller(this);

    blinstaller->setMountPoint(userSettings->value("mountpoint").toString());

    blinstaller->setProxy(proxy());
    blinstaller->setDevice(platform);
    blinstaller->setBootloaderMethod(devices->value(platform + "/bootloadermethod").toString());
    blinstaller->setBootloaderName(devices->value(platform + "/bootloadername").toString());
    blinstaller->setBootloaderBaseUrl(devices->value("bootloader_url").toString());
    blinstaller->setBootloaderInfoUrl(devices->value("bootloader_info_url").toString());
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
                           tr("It seem your Bootloader is already up to date.\n"
                              "Do really want to install it?"),
                           QMessageBox::Ok | QMessageBox::Ignore  |QMessageBox::Cancel,
                           QMessageBox::Cancel);
        if(ret == QMessageBox::Cancel)
        {
            logger->addItem(tr("Bootloader installation canceled!"),LOGERROR);
            logger->abort();
            m_error = true;
            return;
        }
        else if(ret == QMessageBox::Ignore)
        {
            logger->addItem(tr("Bootloader installation skipped!"),LOGERROR);
            logger->abort();
            m_installed = true;
            return;
        }
    }

    // if fwpatcher , ask for extra file
    QString offirmware;
    if(devices->value(platform + "/bootloadermethod").toString() == "fwpatcher")
    {
        BrowseOF ofbrowser(this);
        ofbrowser.setFile(userSettings->value("ofpath").toString());
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
                userSettings->setValue("ofpath",offirmware);
                userSettings->sync();
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

    installer->setUrl(devices->value("font_url").toString());
    installer->setProxy(proxy());
    installer->setLogSection("Fonts");
    installer->setLogVersion(versmap.value("arch_date"));
    installer->setMountPoint(userSettings->value("mountpoint").toString());
    if(!userSettings->value("cachedisable").toBool())
        installer->setCache(userSettings->value("cachepath", QDir::tempPath()).toString());
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

    QString voiceurl = devices->value("voice_url").toString() + "/" +
        userSettings->value("platform").toString() + "-" +
        versmap.value("arch_date") + "-english.voice";
    qDebug() << voiceurl;

    installer->setProxy(proxy());
    installer->setUrl(voiceurl);
    installer->setLogSection("Voice");
    installer->setLogVersion(versmap.value("arch_date"));
    installer->setMountPoint(userSettings->value("mountpoint").toString());
    installer->setTarget("/.rockbox/langs/english.voice");
    if(!userSettings->value("cachedisable").toBool())
        installer->setCache(userSettings->value("cachepath", QDir::tempPath()).toString());
    installer->install(logger);

    //connect(installer, SIGNAL(done(bool)), this, SLOT(done(bool)));
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
    QFile doomrock(userSettings->value("mountpoint").toString()+"/.rockbox/rocks/games/doom.rock");
    return doomrock.exists();
}

void RbUtilQt::installDoom()
{
    // create zip installer
    installer = new ZipInstaller(this);

    installer->setUrl(devices->value("doom_url").toString());
    installer->setProxy(proxy());
    installer->setLogSection("Game Addons");
    installer->setLogVersion(versmap.value("arch_date"));
    installer->setMountPoint(userSettings->value("mountpoint").toString());
    if(!userSettings->value("cachedisable").toBool())
        installer->setCache(userSettings->value("cachepath", QDir::tempPath()).toString());
    installer->install(logger);

}


void RbUtilQt::installThemes()
{
    if(chkConfig(true)) return;
    ThemesInstallWindow* tw = new ThemesInstallWindow(this);
    tw->setDeviceSettings(devices);
    tw->setUserSettings(userSettings);
    tw->setProxy(proxy());
    tw->setModal(true);
    tw->show();
}


void RbUtilQt::createTalkFiles(void)
{
    if(chkConfig(true)) return;
    InstallTalkWindow *installWindow = new InstallTalkWindow(this);
    installWindow->setUserSettings(userSettings);
    installWindow->setDeviceSettings(devices);
    installWindow->show();

}

void RbUtilQt::uninstall(void)
{
    if(chkConfig(true)) return;
    UninstallWindow *uninstallWindow = new UninstallWindow(this);
    uninstallWindow->setUserSettings(userSettings);
    uninstallWindow->setDeviceSettings(devices);
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

    QString plattform = userSettings->value("platform").toString();
    BootloaderInstaller blinstaller(this);
    blinstaller.setProxy(proxy());
    blinstaller.setMountPoint(userSettings->value("mountpoint").toString());
    blinstaller.setDevice(userSettings->value("platform").toString());
    blinstaller.setBootloaderMethod(devices->value(plattform + "/bootloadermethod").toString());
    blinstaller.setBootloaderName(devices->value(plattform + "/bootloadername").toString());
    blinstaller.setBootloaderBaseUrl(devices->value("bootloader_url").toString());
    blinstaller.setBootloaderInfoUrl(devices->value("bootloader_info_url").toString());
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

    devices->beginGroup(userSettings->value("platform").toString());
    QString manual;
    manual = devices->value("manualname", "rockbox-" + devices->value("platform").toString()).toString();
    devices->endGroup();

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
    manualurl = devices->value("manual_url").toString() + "/" + target;
    qDebug() << "manualurl =" << manualurl;

    ProgressLoggerGui* logger = new ProgressLoggerGui(this);
    logger->show();
    installer = new ZipInstaller(this);
    installer->setMountPoint(userSettings->value("mountpoint").toString());
    if(!userSettings->value("cachedisable").toBool())
        installer->setCache(userSettings->value("cachepath", QDir::tempPath()).toString());
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
    logger->show();
    logger->addItem(tr("Installing Rockbox Utility"), LOGINFO);

    // check mountpoint
    if(!QFileInfo(userSettings->value("mountpoint").toString()).isDir()) {
        logger->addItem(tr("Mount point is wrong!"),LOGERROR);
        logger->abort();
        return;
    }

    // remove old files first.
    QFile::remove(userSettings->value("mountpoint").toString() + "/RockboxUtility.exe");
    QFile::remove(userSettings->value("mountpoint").toString() + "/RockboxUtility.ini");
    // copy currently running binary and currently used settings file
    if(!QFile::copy(qApp->applicationFilePath(), userSettings->value("mountpoint").toString() + "/RockboxUtility.exe")) {
        logger->addItem(tr("Error installing Rockbox Utility"), LOGERROR);
        logger->abort();
        return;
    }
    logger->addItem(tr("Installing user configuration"), LOGINFO);
    if(!QFile::copy(userSettings->fileName(), userSettings->value("mountpoint").toString() + "/RockboxUtility.ini")) {
        logger->addItem(tr("Error installing user configuration"), LOGERROR);
        logger->abort();
        return;
    }
    logger->addItem(tr("Successfully installed Rockbox Utility."), LOGOK);
    logger->abort();

}


void RbUtilQt::updateInfo()
{
    qDebug() << "RbUtilQt::updateInfo()";

    QSettings log(userSettings->value("mountpoint").toString() + "/.rockbox/rbutil.log", QSettings::IniFormat, this);
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
            file = userSettings->value("mountpoint").toString() + "/" + keys.at(b);
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
    if(userSettings->value("proxytype", "system").toString() == "manual")
        return QUrl(userSettings->value("proxy").toString());
#if defined(Q_OS_LINUX)
    else if(userSettings->value("proxytype", "system").toString() == "system")
        return QUrl(getenv("http_proxy"));
#endif
#if defined(Q_OS_WIN32)
    HKEY hk;
    wchar_t proxyval[80];
    DWORD buflen = 80;
    long ret;

    ret = RegOpenKeyEx(HKEY_CURRENT_USER, _TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"),
        0, KEY_QUERY_VALUE, &hk);
    if(ret != ERROR_SUCCESS) return QUrl("");

    ret = RegQueryValueEx(hk, _TEXT("ProxyServer"), NULL, NULL, (LPBYTE)proxyval, &buflen);
    if(ret != ERROR_SUCCESS) return QUrl("");

    RegCloseKey(hk);
    qDebug() << QString::fromWCharArray(proxyval);
    return QUrl("http://" + QString::fromWCharArray(proxyval));

#endif
    return QUrl("");
}


bool RbUtilQt::chkConfig(bool warn)
{
    bool error = false;
    if(userSettings->value("platform").toString().isEmpty()
        || userSettings->value("mountpoint").toString().isEmpty()
        || !QFileInfo(userSettings->value("mountpoint").toString()).isWritable()) {
        error = true;

        if(warn) QMessageBox::critical(this, tr("Configuration error"),
            tr("Your configuration is invalid. Please go to the configuration "
                "dialog and make sure the selected values are correct."));
    }
    return error;
}

