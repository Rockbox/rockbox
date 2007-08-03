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
#include "installbl.h"
#include "httpget.h"
#include "installbootloader.h"

#ifdef __linux
#include <stdio.h>
#endif

RbUtilQt::RbUtilQt(QWidget *parent) : QMainWindow(parent)
{
    QString programPath = qApp->arguments().at(0);
    absolutePath = QFileInfo(programPath).absolutePath() + "/";
    // use built-in rbutil.ini if no external file in binary folder
    QString iniFile = absolutePath + "rbutil.ini";
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
    config.setFile(absolutePath + "RockboxUtility.ini");
    if(config.isFile()) {
        userSettings = new QSettings(absolutePath + "RockboxUtility.ini",
            QSettings::IniFormat, 0);
        qDebug() << "config: portable";
    }
    else {
        userSettings = new QSettings(QSettings::IniFormat,
            QSettings::UserScope, "rockbox.org", "RockboxUtility");
        qDebug() << "config: system";
    }
    
    // manual tab
    ui.buttonDownloadManual->setEnabled(false);
    updateManual();
    updateDevice();

    connect(ui.actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(ui.action_About, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui.action_Configure, SIGNAL(triggered()), this, SLOT(configDialog()));
    connect(ui.buttonChangeDevice, SIGNAL(clicked()), this, SLOT(configDialog()));
    connect(ui.buttonRockbox, SIGNAL(clicked()), this, SLOT(install()));
    connect(ui.buttonBootloader, SIGNAL(clicked()), this, SLOT(installBl()));
    connect(ui.buttonFonts, SIGNAL(clicked()), this, SLOT(installFonts()));
    connect(ui.buttonGames, SIGNAL(clicked()), this, SLOT(installDoom()));

    // disable unimplemented stuff
    ui.buttonThemes->setEnabled(false);
    ui.buttonSmall->setEnabled(false);
    ui.buttonRemoveRockbox->setEnabled(false);
    ui.buttonRemoveBootloader->setEnabled(false);
    ui.buttonComplete->setEnabled(false);

    initIpodpatcher();
    downloadInfo();

}


void RbUtilQt::downloadInfo()
{
    // try to get the current build information
    daily = new HttpGet(this);
    connect(daily, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
    connect(daily, SIGNAL(requestFinished(int, bool)), this, SLOT(downloadDone(int, bool)));
    if(userSettings->value("defaults/proxytype") == "manual")
        daily->setProxy(QUrl(userSettings->value("defaults/proxy").toString()));
#ifdef __linux
    else if(userSettings->value("defaults/proxytype") == "system")
        daily->setProxy(QUrl(getenv("http_proxy")));
#endif

    qDebug() << "downloading build info";
    daily->setFile(&buildInfo);
    daily->getFile(QUrl(devices->value("server_conf_url").toString()));
}


void RbUtilQt::downloadDone(bool error)
{
    if(error) qDebug() << "network error:" << daily->error();
    qDebug() << "network status:" << daily->error();

}


void RbUtilQt::downloadDone(int id, bool error)
{
    QString errorString;
    errorString = tr("Network error: %1. Please check your network and proxy settings.")
        .arg(daily->errorString());
    if(error) QMessageBox::about(this, "Network Error", errorString);
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
    QString title = QString("<b>The Rockbox Utility</b> Version %1").arg(VERSION);
    about.labelTitle->setText(title);
    about.labelHomepage->setText("<a href='http://www.rockbox.org'>http://www.rockbox.org</a>");

    window->show();

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
    platform = userSettings->value("defaults/platform").toString();
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
    platform = userSettings->value("defaults/platform").toString();
    QString mountpoint = userSettings->value("defaults/mountpoint").toString();
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
    if(userSettings->value("defaults/platform").toString() != "")
    {
        devices->beginGroup(userSettings->value("defaults/platform").toString());
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


void RbUtilQt::install()
{
    Install *installWindow = new Install(this);
    installWindow->setUserSettings(userSettings);
    installWindow->setDeviceSettings(devices);
    if(userSettings->value("defaults/proxytype") == "manual")
        installWindow->setProxy(QUrl(userSettings->value("defaults/proxy").toString()));
#ifdef __linux
    else if(userSettings->value("defaults/proxytype") == "system")
        installWindow->setProxy(QUrl(getenv("http_proxy")));
#endif

    buildInfo.open();
    QSettings info(buildInfo.fileName(), QSettings::IniFormat, this);
    buildInfo.close();
    installWindow->setArchivedString(info.value("dailies/date").toString());

    devices->beginGroup(platform);
    QString released = devices->value("released").toString();
    devices->endGroup();
    if(released == "yes")
        installWindow->setReleased(devices->value("last_release", "").toString());
    else
        installWindow->setReleased(0);

    installWindow->show();
}


void RbUtilQt::installBl()
{
    InstallBl *installWindow = new InstallBl(this);
    installWindow->setUserSettings(userSettings);
    installWindow->setDeviceSettings(devices);
    if(userSettings->value("defaults/proxytype") == "manual")
        installWindow->setProxy(QUrl(userSettings->value("defaults/proxy").toString()));
#ifdef __linux
    else if(userSettings->value("defaults/proxytype") == "system")
        installWindow->setProxy(QUrl(getenv("http_proxy")));
#endif
    installWindow->setMountPoint(userSettings->value("defaults/mountpoint").toString());

    installWindow->show();
}


void RbUtilQt::installFonts()
{
    if(QMessageBox::question(this, tr("Confirm Installation"),
        tr("Do you really want to install the fonts package?"),
           QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    // create logger
    logger = new ProgressLoggerGui(this);
    logger->show();
    
    // create zip installer
    installer = new ZipInstaller(this);
   
    installer->setUrl(devices->value("font_url").toString());
    if(userSettings->value("defaults/proxytype") == "manual")
        installer->setProxy(QUrl(userSettings->value("defaults/proxy").toString()));
#ifdef __linux
    else if(userSettings->value("defaults/proxytype") == "system")
        installer->setProxy(QUrl(getenv("http_proxy")));
#endif

    installer->setLogSection("Fonts");
    installer->setMountPoint(userSettings->value("defaults/mountpoint").toString());
    installer->install(logger);
    
    connect(installer, SIGNAL(done(bool)), this, SLOT(done(bool)));
}


void RbUtilQt::installDoom()
{
    if(QMessageBox::question(this, tr("Confirm Installation"),
       tr("Do you really want to install the game addon files?"),
       QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    // create logger
    logger = new ProgressLoggerGui(this);
    logger->show();
    
    // create zip installer
    installer = new ZipInstaller(this);
   
    installer->setUrl(devices->value("doom_url").toString());
    if(userSettings->value("defaults/proxytype") == "manual")
        installer->setProxy(QUrl(userSettings->value("defaults/proxy").toString()));
#ifdef __linux
    else if(userSettings->value("defaults/proxytype") == "system")
        installer->setProxy(QUrl(getenv("http_proxy")));
#endif

    installer->setLogSection("Game Addons");
    installer->setMountPoint(userSettings->value("defaults/mountpoint").toString());
    installer->install(logger);
    
    connect(installer, SIGNAL(done(bool)), this, SLOT(done(bool)));

}

