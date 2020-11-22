/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QMainWindow>
#include <QMessageBox>

#include "version.h"
#include "rbutilqt.h"
#include "ui_rbutilqtfrm.h"
#include "ui_aboutbox.h"
#include "configure.h"
#include "installtalkwindow.h"
#include "createvoicewindow.h"
#include "httpget.h"
#include "themesinstallwindow.h"
#include "uninstallwindow.h"
#include "utils.h"
#include "rockboxinfo.h"
#include "sysinfo.h"
#include "system.h"
#include "systrace.h"
#include "rbsettings.h"
#include "serverinfo.h"
#include "systeminfo.h"
#include "ziputil.h"
#include "infowidget.h"
#include "selectiveinstallwidget.h"
#include "backupdialog.h"
#include "changelog.h"

#include "progressloggerinterface.h"

#include "bootloaderinstallbase.h"
#include "bootloaderinstallhelper.h"

#include "Logger.h"

#if defined(Q_OS_LINUX)
#include <stdio.h>
#endif
#if defined(Q_OS_WIN32)
#if !defined(_UNICODE)
#define _UNICODE
#endif
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#endif

QList<QTranslator*> RbUtilQt::translators;

RbUtilQt::RbUtilQt(QWidget *parent) : QMainWindow(parent)
{
    // startup log
    LOG_INFO() << "======================================";
    LOG_INFO() << "Rockbox Utility" << VERSION;
    LOG_INFO() << "Qt version:" << qVersion();
#if defined(__clang__)
    LOG_INFO("compiled using clang %i.%i.%i",
             __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined(__GNUC__)
    LOG_INFO("compiled using gcc %i.%i.%i",
             __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
    LOG_INFO() << "compiled using MSVC" << _MSC_FULL_VER;
#endif
    LOG_INFO() << "======================================";

    absolutePath = qApp->applicationDirPath();

    QString c = RbSettings::value(RbSettings::CachePath).toString();
    HttpGet::setGlobalCache(c.isEmpty() ? QDir::tempPath() : c);
    HttpGet::setGlobalUserAgent("rbutil/" VERSION);
    HttpGet::setGlobalProxy(proxy());
    // init startup & autodetection
    ui.setupUi(this);
#if defined(Q_OS_LINUX)
    QIcon windowIcon(":/icons/rockbox-clef.svg");
    this->setWindowIcon(windowIcon);
#endif
#if defined(Q_OS_WIN32)
    QIcon windowIcon = QIcon();
    windowIcon.addFile(":/icons/rockbox-16.png");
    windowIcon.addFile(":/icons/rockbox-32.png");
    windowIcon.addFile(":/icons/rockbox-48.png");
    windowIcon.addFile(":/icons/rockbox-64.png");
    windowIcon.addFile(":/icons/rockbox-128.png");
    windowIcon.addFile(":/icons/rockbox-256.png");
    this->setWindowIcon(windowIcon);
#endif
#if defined(Q_OS_MACX)
    // don't translate menu entries that are handled specially on OS X
    // (Configure, Quit). Qt handles them for us if they use english string.
    ui.action_Configure->setText("Configure");
    ui.actionE_xit->setText("Quit");
#endif
#if defined(Q_OS_WIN32)
    long ret;
    HKEY hk;
    ret = RegOpenKeyEx(HKEY_CURRENT_USER, _TEXT("Software\\Wine"),
        0, KEY_QUERY_VALUE, &hk);
    if(ret == ERROR_SUCCESS) {
        QMessageBox::warning(this, tr("Wine detected!"),
                tr("It seems you are trying to run this program under Wine. "
                    "Please don't do this, running under Wine will fail. "
                    "Use the native Linux binary instead."),
                QMessageBox::Ok, QMessageBox::Ok);
        LOG_WARNING() << "WINE DETECTED!";
        RegCloseKey(hk);
    }
#endif

#if !defined(Q_OS_WIN32) && !defined(Q_OS_MACX)
    /* eject funtionality is not available on Linux right now. */
    ui.buttonEject->setEnabled(false);
#endif
    updateDevice();
    downloadInfo();

    m_gotInfo = false;
    m_auto = false;

    // selective "install" tab.
    QGridLayout *selectivetablayout = new QGridLayout(this);
    ui.selective->setLayout(selectivetablayout);
    selectiveinstallwidget = new SelectiveInstallWidget(this);
    selectivetablayout->addWidget(selectiveinstallwidget);
    connect(ui.buttonChangeDevice, SIGNAL(clicked()), selectiveinstallwidget, SLOT(saveSettings()));

    // info tab
    QGridLayout *infotablayout = new QGridLayout(this);
    ui.info->setLayout(infotablayout);
    info = new InfoWidget(this);
    infotablayout->addWidget(info);

    connect(ui.tabWidget, SIGNAL(currentChanged(int)), this, SLOT(updateTabs(int)));
    connect(ui.actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(ui.action_About, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui.action_Help, SIGNAL(triggered()), this, SLOT(help()));
    connect(ui.action_Configure, SIGNAL(triggered()), this, SLOT(configDialog()));
    connect(ui.actionE_xit, SIGNAL(triggered()), this, SLOT(shutdown()));
    connect(ui.buttonChangeDevice, SIGNAL(clicked()), this, SLOT(configDialog()));
    connect(ui.buttonEject, SIGNAL(clicked()), this, SLOT(eject()));
    connect(ui.buttonTalk, SIGNAL(clicked()), this, SLOT(createTalkFiles()));
    connect(ui.buttonCreateVoice, SIGNAL(clicked()), this, SLOT(createVoiceFile()));
    connect(ui.buttonRemoveRockbox, SIGNAL(clicked()), this, SLOT(uninstall()));
    connect(ui.buttonRemoveBootloader, SIGNAL(clicked()), this, SLOT(uninstallBootloader()));
    connect(ui.buttonBackup, SIGNAL(clicked()), this, SLOT(backup()));

    // actions accessible from the menu
    connect(ui.actionCreate_Voice_File, SIGNAL(triggered()), this, SLOT(createVoiceFile()));
    connect(ui.actionCreate_Talk_Files, SIGNAL(triggered()), this, SLOT(createTalkFiles()));
    connect(ui.actionRemove_bootloader, SIGNAL(triggered()), this, SLOT(uninstallBootloader()));
    connect(ui.actionUninstall_Rockbox, SIGNAL(triggered()), this, SLOT(uninstall()));
    connect(ui.action_System_Info, SIGNAL(triggered()), this, SLOT(sysinfo()));
    connect(ui.action_Trace, SIGNAL(triggered()), this, SLOT(trace()));
    connect(ui.actionShow_Changelog, SIGNAL(triggered()), this, SLOT(changelog()));

#if !defined(STATIC)
    ui.actionInstall_Rockbox_Utility_on_player->setEnabled(false);
#else
    connect(ui.actionInstall_Rockbox_Utility_on_player, SIGNAL(triggered()), this, SLOT(installPortable()));
#endif
    Utils::findRunningProcess(QStringList("iTunes"));

}


void RbUtilQt::shutdown(void)
{
    this->close();
}


void RbUtilQt::trace(void)
{
    SysTrace wnd(this);
    wnd.exec();
}

void RbUtilQt::sysinfo(void)
{
    Sysinfo sysinfo(this);
    sysinfo.exec();
}

void RbUtilQt::changelog(void)
{

    Changelog cl(this);
    cl.exec();
}


void RbUtilQt::updateTabs(int count)
{
    if(count == ui.tabWidget->indexOf(info->parentWidget()))
        info->updateInfo();
}


void RbUtilQt::downloadInfo()
{
    // try to get the current build information
    daily = new HttpGet(this);
    connect(daily, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
    connect(qApp, SIGNAL(lastWindowClosed()), daily, SLOT(abort()));
    daily->setCache(false);
    ui.statusbar->showMessage(tr("Downloading build information, please wait ..."));
    LOG_INFO() << "downloading build info";
    daily->setFile(&buildInfo);
    daily->getFile(QUrl(SystemInfo::value(SystemInfo::BuildInfoUrl).toString()));
}


void RbUtilQt::downloadDone(bool error)
{
    if(error) {
        LOG_INFO() << "network error:" << daily->errorString();
        ui.statusbar->showMessage(tr("Can't get version information!"));
        QMessageBox::critical(this, tr("Network error"),
                tr("Can't get version information.\n"
                   "Network error: %1. Please check your network and proxy settings.")
                    .arg(daily->errorString()));
        return;
    }
    LOG_INFO() << "network status:" << daily->errorString();

    // read info into ServerInfo object
    buildInfo.open();
    ServerInfo::instance()->readBuildInfo(buildInfo.fileName());
    buildInfo.close();

    ui.statusbar->showMessage(tr("Download build information finished."), 5000);
    if(RbSettings::value(RbSettings::RbutilVersion) != PUREVERSION
            || RbSettings::value(RbSettings::ShowChangelog).toBool()) {
        changelog();
    }
    updateSettings();
    m_gotInfo = true;

    //start check for updates
    checkUpdate();

}


void RbUtilQt::about()
{
    QDialog *window = new QDialog(this);
    Ui::aboutBox about;
    about.setupUi(window);
    window->setLayoutDirection(Qt::LeftToRight);
    window->setModal(true);

    QFile licence(":/docs/gpl-2.0.html");
    licence.open(QIODevice::ReadOnly);
    QTextStream c(&licence);
    about.browserLicense->insertHtml(c.readAll());
    about.browserLicense->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
    licence.close();

    QString html = "<p>" + tr("Libraries used") + "</p>";
    html += "<ul>";
    html += "<li>Speex: <a href='#speex'>Speex License</a></li>";
    html += "<li>bspatch: <a href='#bspatch'>bspatch License</a></li>";
    html += "<li>bzip2: <a href='#bzip2'>bzip2 License</a></li>";
    html += "<li>mspack: <a href='#lgpl2'>LGPL v2.1 License</a></li>";
    html += "<li>quazip: <a href='#lgpl2'>LGPL v2.1 License</a></li>";
    html += "<li>tomcrypt: <a href='#tomcrypt'>Tomcrypt License</a></li>";
    html += "<li>CuteLogger: <a href='#lgpl2'>LGPL v2.1 License</a></li>";
    html += "</ul>";
    about.browserLicenses->insertHtml(html);

    QMap<QString, QString> licenses;
    licenses[":/docs/COPYING.SPEEX"] = "<a id='speex'>Speex License</a>";
    licenses[":/docs/lgpl-2.1.txt"] = "<a id='lgpl2'>LGPL v2.1</a>";
    licenses[":/docs/LICENSE.TOMCRYPT"] = "<a id='tomcrypt'>Tomcrypt License</a>";
    licenses[":/docs/LICENSE.BZIP2"] = "<a id='bzip2'>bzip2 License</a>";
    licenses[":/docs/LICENSE.BSPATCH"] = "<a id='bspatch'>bspatch License</a>";

    for (int i = 0; i < licenses.size(); i++) {
        QString key = licenses.keys().at(i);
        QFile license(key);
        license.open(QIODevice::ReadOnly);
        QTextStream s(&license);
        about.browserLicenses->insertHtml("<hr/><h2>" + licenses[key] + "</h2><br/>\n");
        about.browserLicenses->insertHtml("<pre>" + s.readAll() + "</pre>");
        license.close();
    }
    about.browserLicenses->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);

    QFile credits(":/docs/CREDITS");
    credits.open(QIODevice::ReadOnly);
    QTextStream r(&credits);
#if QT_VERSION < 0x060000
    r.setCodec(QTextCodec::codecForName("UTF-8"));
#else
    r.setEncoding(QStringConverter::Utf8);
#endif
    while(!r.atEnd()) {
        QString line = r.readLine();
        // filter out header.
        line.remove(QRegExp("^ +.*"));
        line.remove(QRegExp("^People.*"));
        about.browserCredits->append(line);
    }
    credits.close();
    about.browserCredits->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
    QString title = QString("<b>The Rockbox Utility</b><br/>Version %1").arg(FULLVERSION);
    about.labelTitle->setText(title);

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
    connect(cw, SIGNAL(settingsUpdated()), this, SLOT(updateSettings()));
    cw->show();
}


void RbUtilQt::updateSettings()
{
    LOG_INFO() << "updating current settings";
    updateDevice();
    QString c = RbSettings::value(RbSettings::CachePath).toString();
    HttpGet::setGlobalCache(c.isEmpty() ? QDir::tempPath() : c);
    HttpGet::setGlobalProxy(proxy());

    if(RbSettings::value(RbSettings::RbutilVersion) != PUREVERSION) {
        QApplication::processEvents();
        QMessageBox::information(this, tr("New installation"),
            tr("This is a new installation of Rockbox Utility, or a new version. "
                "The configuration dialog will now open to allow you to setup the program, "
                " or review your settings."));
        configDialog();
    }
    else if(chkConfig(nullptr)) {
        QApplication::processEvents();
        QMessageBox::critical(this, tr("Configuration error"),
            tr("Your configuration is invalid. This is most likely due "
                "to a changed device path. The configuration dialog will "
                "now open to allow you to correct the problem."));
        configDialog();
    }
    selectiveinstallwidget->updateVersion();
}


void RbUtilQt::updateDevice()
{
    /* TODO: We should check the flags of the bootloaderinstall classes, and not
     * just check if its != none or != "fwpatcher" */

    /* Enable bootloader installation, if possible */
    bool bootloaderInstallable =
        SystemInfo::platformValue(SystemInfo::BootloaderMethod) != "none";

    /* Enable bootloader uninstallation, if possible */
    bool bootloaderUninstallable = bootloaderInstallable &&
        SystemInfo::platformValue(SystemInfo::BootloaderMethod) != "fwpatcher";
    ui.labelRemoveBootloader->setEnabled(bootloaderUninstallable);
    ui.buttonRemoveBootloader->setEnabled(bootloaderUninstallable);
    ui.actionRemove_bootloader->setEnabled(bootloaderUninstallable);

    /* Disable the whole tab widget if configuration is invalid */
    bool configurationValid = !chkConfig(nullptr);
    ui.tabWidget->setEnabled(configurationValid);
    ui.menuA_ctions->setEnabled(configurationValid);

    // displayed device info
    QString brand = SystemInfo::platformValue(SystemInfo::Brand).toString();
    QString name
        = QString("%1 (%2)").arg(SystemInfo::platformValue(SystemInfo::Name).toString(),
            ServerInfo::instance()->statusAsString());
    ui.labelDevice->setText(QString("<b>%1 %2</b>").arg(brand, name));

    QString mountpoint = RbSettings::value(RbSettings::Mountpoint).toString();
    QString mountdisplay = QDir::toNativeSeparators(mountpoint);
    if(!mountdisplay.isEmpty()) {
        QString label = Utils::filesystemName(mountpoint);
        if(!label.isEmpty()) mountdisplay += QString(" (%1)").arg(label);
        ui.labelMountpoint->setText(QString("<b>%1</b>").arg(mountdisplay));
    }
    else {
        mountdisplay = "(unknown)";
    }

    QPixmap pm;
    QString m = SystemInfo::platformValue(SystemInfo::PlayerPicture).toString();
    pm.load(":/icons/players/" + m + "-small.png");
    pm = pm.scaledToHeight(QFontMetrics(QApplication::font()).height() * 3);
    ui.labelPlayerPic->setPixmap(pm);

}


void RbUtilQt::backup(void)
{
    backupdialog = new BackupDialog(this);
    backupdialog->show();

}


void RbUtilQt::installdone(bool error)
{
    LOG_INFO() << "install done";
    m_installed = true;
    m_error = error;
}


void RbUtilQt::createTalkFiles(void)
{
    if(chkConfig(this)) return;
    InstallTalkWindow *installWindow = new InstallTalkWindow(this);
    connect(installWindow, SIGNAL(settingsUpdated()), this, SLOT(updateSettings()));
    installWindow->show();

}

void RbUtilQt::createVoiceFile(void)
{
    if(chkConfig(this)) return;
    CreateVoiceWindow *installWindow = new CreateVoiceWindow(this);

    connect(installWindow, SIGNAL(settingsUpdated()), this, SLOT(updateSettings()));
    installWindow->show();
}

void RbUtilQt::uninstall(void)
{
    if(chkConfig(this)) return;
    UninstallWindow *uninstallWindow = new UninstallWindow(this);
    uninstallWindow->show();

}

void RbUtilQt::uninstallBootloader(void)
{
    if(chkConfig(this)) return;
    if(QMessageBox::question(this, tr("Confirm Uninstallation"),
           tr("Do you really want to uninstall the Bootloader?"),
           QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    // create logger
    ProgressLoggerGui* logger = new ProgressLoggerGui(this);
    logger->show();

    QString platform = RbSettings::value(RbSettings::Platform).toString();

    // create installer
    BootloaderInstallBase *bl
        = BootloaderInstallHelper::createBootloaderInstaller(this,
                SystemInfo::platformValue(SystemInfo::BootloaderMethod).toString());

    if(bl == nullptr) {
        logger->addItem(tr("No uninstall method for this target known."), LOGERROR);
        logger->setFinished();
        return;
    }
    QStringList blfile = SystemInfo::platformValue(SystemInfo::BootloaderFile).toStringList();
    QStringList blfilepath;
    for(int a = 0; a < blfile.size(); a++) {
        blfilepath.append(RbSettings::value(RbSettings::Mountpoint).toString()
                + blfile.at(a));
    }
    bl->setBlFile(blfilepath);
    bl->setLogfile(RbSettings::value(RbSettings::Mountpoint).toString()
            + "/.rockbox/rbutil.log");

    BootloaderInstallBase::BootloaderType currentbl = bl->installed();
    if((bl->capabilities() & BootloaderInstallBase::Uninstall) == 0) {
        logger->addItem(tr("Rockbox Utility can not uninstall the bootloader on your player. "
                           "Please perform a firmware update using your player vendors "
                           "firmware update process."), LOGERROR);
        logger->addItem(tr("Important: make sure to boot your player into the original "
                           "firmware before using the vendors firmware update process."), LOGERROR);
        logger->setFinished();
        delete bl;
        return;
    }
    if(currentbl == BootloaderInstallBase::BootloaderUnknown
            || currentbl == BootloaderInstallBase::BootloaderOther) {
        logger->addItem(tr("No Rockbox bootloader found."), LOGERROR);
        logger->setFinished();
        delete bl;
        return;
    }

    connect(bl, SIGNAL(logItem(QString, int)), logger, SLOT(addItem(QString, int)));
    connect(bl, SIGNAL(logProgress(int, int)), logger, SLOT(setProgress(int, int)));
    connect(bl, SIGNAL(done(bool)), logger, SLOT(setFinished()));
    // pass Abort button click signal to current installer
    connect(logger, SIGNAL(aborted()), bl, SLOT(progressAborted()));

    bl->uninstall();

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
    if(!QFileInfo(RbSettings::value(RbSettings::Mountpoint).toString()).isDir()) {
        logger->addItem(tr("Mount point is wrong!"),LOGERROR);
        logger->setFinished();
        return;
    }

    // remove old files first.
    QFile::remove(RbSettings::value(RbSettings::Mountpoint).toString()
            + "/RockboxUtility.exe");
    QFile::remove(RbSettings::value(RbSettings::Mountpoint).toString()
            + "/RockboxUtility.ini");
    // copy currently running binary and currently used settings file
    if(!QFile::copy(qApp->applicationFilePath(),
            RbSettings::value(RbSettings::Mountpoint).toString()
            + "/RockboxUtility.exe")) {
        logger->addItem(tr("Error installing Rockbox Utility"), LOGERROR);
        logger->setFinished();
        return;
    }
    logger->addItem(tr("Installing user configuration"), LOGINFO);
    if(!QFile::copy(RbSettings::userSettingFilename(),
            RbSettings::value(RbSettings::Mountpoint).toString()
            + "/RockboxUtility.ini")) {
        logger->addItem(tr("Error installing user configuration"), LOGERROR);
        logger->setFinished();
        return;
    }
    logger->addItem(tr("Successfully installed Rockbox Utility."), LOGOK);
    logger->setFinished();
    logger->setProgressMax(1);
    logger->setProgressValue(1);

}


QUrl RbUtilQt::proxy()
{
    QUrl proxy;
    QString proxytype = RbSettings::value(RbSettings::ProxyType).toString();
    if(proxytype == "manual") {
        proxy.setUrl(RbSettings::value(RbSettings::Proxy).toString(),
                QUrl::TolerantMode);
        QByteArray pw = QByteArray::fromBase64(proxy.password().toUtf8());
        proxy.setPassword(pw);
    }
    else if(proxytype == "system")
        proxy = System::systemProxy();

    LOG_INFO() << "Proxy is" << proxy;
    return proxy;
}


bool RbUtilQt::chkConfig(QWidget *parent)
{
    bool error = false;
    if(RbSettings::value(RbSettings::Platform).toString().isEmpty()
        || RbSettings::value(RbSettings::Mountpoint).toString().isEmpty()
        || !QFileInfo(RbSettings::value(RbSettings::Mountpoint).toString()).isWritable()) {
        error = true;

        if(parent) QMessageBox::critical(parent, tr("Configuration error"),
            tr("Your configuration is invalid. Please go to the configuration "
                "dialog and make sure the selected values are correct."));
    }
    return error;
}

void RbUtilQt::checkUpdate(void)
{
    QString url = SystemInfo::value(SystemInfo::RbutilUrl).toString();
#if defined(Q_OS_WIN32)   
    url += "win32/";
#elif defined(Q_OS_LINUX)
    url += "linux/";
#elif defined(Q_OS_MACX)
    url += "macosx/";
#endif

    update = new HttpGet(this);
    connect(update, SIGNAL(done(bool)), this, SLOT(downloadUpdateDone(bool)));
    connect(qApp, SIGNAL(lastWindowClosed()), update, SLOT(abort()));

    ui.statusbar->showMessage(tr("Checking for update ..."));
    update->getFile(QUrl(url));
}

void RbUtilQt::downloadUpdateDone(bool error)
{
    if(error) {
        LOG_INFO() << "network error:" << update->errorString();
    }
    else {
        QString toParse(update->readAll());

        QRegExp searchString("<a[^>]*>([a-zA-Z]+[^<]*)</a>");
        QStringList rbutilList;
        int pos = 0;
        while ((pos = searchString.indexIn(toParse, pos)) != -1)
        {
            rbutilList << searchString.cap(1);
            pos += searchString.matchedLength();
        }
        LOG_INFO() << "Checking for update";

        QString newVersion = "";
        QString foundVersion = "";
        // check if there is a binary with higher version in this list
        for(int i=0; i < rbutilList.size(); i++)
        {
            QString item = rbutilList.at(i);
#if defined(Q_OS_LINUX) 
#if defined(__amd64__)
            // skip if it isn't a 64 bit build
            if( !item.contains("64bit"))
                continue;
            // strip the "64bit" suffix for comparison
            item = item.remove("64bit");
#else
            //skip if it is a 64bit build
            if(item.contains("64bit"))
                continue;
#endif
#endif
            // check if it is newer, and remember newest
            if(Utils::compareVersionStrings(VERSION, item) == 1)
            {
                if(Utils::compareVersionStrings(newVersion, item) == 1)
                {
                    newVersion = item;
                    foundVersion = rbutilList.at(i);
                }
            }
        }

        // if we found something newer, display info
        if(foundVersion != "")
        {
            QString url = SystemInfo::value(SystemInfo::RbutilUrl).toString();
#if defined(Q_OS_WIN32)   
            url += "win32/";
#elif defined(Q_OS_LINUX)
            url += "linux/";
#elif defined(Q_OS_MACX)
            url += "macosx/";
#endif
            url += foundVersion;

            QMessageBox::information(this,tr("Rockbox Utility Update available"),
                        tr("<b>New Rockbox Utility version available.</b><br><br>"
                           "You are currently using version %1. "
                           "Get version %2 at <a href='%3'>%3</a>")
                           .arg(VERSION).arg(Utils::trimVersionString(foundVersion)).arg(url));
            ui.statusbar->showMessage(tr("New version of Rockbox Utility available."));
        }
        else {
            ui.statusbar->showMessage(tr("Rockbox Utility is up to date."), 5000);
        }
    }
}


void RbUtilQt::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange) {
        ui.retranslateUi(this);
        buildInfo.open();
        ServerInfo::instance()->readBuildInfo(buildInfo.fileName());
        buildInfo.close();
        updateDevice();
    } else {
        QMainWindow::changeEvent(e);
    }
}

void RbUtilQt::eject(void)
{
    QString mountpoint = RbSettings::value(RbSettings::Mountpoint).toString();
    if(Utils::ejectDevice(mountpoint)) {
        QMessageBox::information(this, tr("Device ejected"),
                tr("Device successfully ejected. "
                   "You may now disconnect the player from the PC."));
    }
    else {
        QMessageBox::critical(this, tr("Ejecting failed"),
                tr("Ejecting the device failed. Please make sure no programs "
                   "are accessing files on the device. If ejecting still "
                   "fails please use your computers eject funtionality."));
    }
}

