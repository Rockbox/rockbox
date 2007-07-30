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
#include "installzipwindow.h"

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
    initDeviceNames();

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

    userSettings->beginGroup("defaults");
    platform = userSettings->value("platform").toString();
    userSettings->endGroup();
    ui.comboBoxDevice->setCurrentIndex(ui.comboBoxDevice->findData(platform));
    updateDevice(ui.comboBoxDevice->currentIndex());

    connect(ui.actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(ui.action_About, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui.action_Configure, SIGNAL(triggered()), this, SLOT(configDialog()));
    connect(ui.comboBoxDevice, SIGNAL(currentIndexChanged(int)), this, SLOT(updateDevice(int)));
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
    ui.buttonDetect->setEnabled(false);

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
    errorString = tr("Network error: %1. Please check your network and proxy settings.").arg(daily->errorString());
    if(error) QMessageBox::about(this, "Network Error", errorString);
    qDebug() << "downloadDone:" << id << error;
}


void RbUtilQt::about()
{
    QDialog *window = new QDialog;
    Ui::aboutBox about;
    about.setupUi(window);

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
    cw->show();
    connect(cw, SIGNAL(settingsUpdated()), this, SLOT(downloadInfo()));
}


void RbUtilQt::initDeviceNames()
{
    qDebug() << "RbUtilQt::initDeviceNames()";
    devices->beginGroup("platforms");
    QStringList a = devices->childKeys();
    devices->endGroup();

    for(int it = 0; it < a.size(); it++) {
        QString curdev;
        devices->beginGroup("platforms");
        curdev = devices->value(a.at(it), "null").toString();
        devices->endGroup();
        QString curname;
        devices->beginGroup(curdev);
        curname = devices->value("name", "null").toString();
        devices->endGroup();
        ui.comboBoxDevice->addItem(curname, curdev);
    }
}


void RbUtilQt::updateDevice(int index)
{
    platform = ui.comboBoxDevice->itemData(index).toString();
    userSettings->setValue("defaults/platform", platform);
    userSettings->sync();

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

    qDebug() << "new device selected:" << platform;
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
    installWindow->setMountPoint(userSettings->value("defaults/mountpoint").toString());

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
    InstallZipWindow* installWindow = new InstallZipWindow(this);
    installWindow->setUserSettings(userSettings);
    installWindow->setDeviceSettings(devices);
    if(userSettings->value("defaults/proxytype") == "manual")
        installWindow->setProxy(QUrl(userSettings->value("defaults/proxy").toString()));
#ifdef __linux
    else if(userSettings->value("defaults/proxytype") == "system")
        installWindow->setProxy(QUrl(getenv("http_proxy")));
#endif
    installWindow->setMountPoint(userSettings->value("defaults/mountpoint").toString());
    installWindow->setLogSection("Fonts");
    installWindow->setUrl(devices->value("font_url").toString());
    installWindow->setWindowTitle("Font Installation");
    installWindow->show();

}

void RbUtilQt::installDoom()
{
    InstallZipWindow* installWindow = new InstallZipWindow(this);
    installWindow->setUserSettings(userSettings);
    installWindow->setDeviceSettings(devices);
    if(userSettings->value("defaults/proxytype") == "manual")
        installWindow->setProxy(QUrl(userSettings->value("defaults/proxy").toString()));
#ifdef __linux
    else if(userSettings->value("defaults/proxytype") == "system")
        installWindow->setProxy(QUrl(getenv("http_proxy")));
#endif
    installWindow->setMountPoint(userSettings->value("defaults/mountpoint").toString());
    installWindow->setLogSection("Doom");
    installWindow->setUrl(devices->value("doom_url").toString());
    installWindow->setWindowTitle("Doom Installation");
    installWindow->show();

}
