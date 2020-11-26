/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2012 by Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QWidget>
#include <QMessageBox>
#include <QFileDialog>
#include "selectiveinstallwidget.h"
#include "ui_selectiveinstallwidgetfrm.h"
#include "serverinfo.h"
#include "rbsettings.h"
#include "rockboxinfo.h"
#include "systeminfo.h"
#include "progressloggergui.h"
#include "bootloaderinstallbase.h"
#include "bootloaderinstallhelper.h"
#include "themesinstallwindow.h"
#include "utils.h"
#include "Logger.h"

SelectiveInstallWidget::SelectiveInstallWidget(QWidget* parent) : QWidget(parent)
{
    ui.setupUi(this);
    ui.rockboxCheckbox->setChecked(RbSettings::value(RbSettings::InstallRockbox).toBool());
    ui.fontsCheckbox->setChecked(RbSettings::value(RbSettings::InstallFonts).toBool());
    ui.themesCheckbox->setChecked(RbSettings::value(RbSettings::InstallThemes).toBool());
    ui.gamefileCheckbox->setChecked(RbSettings::value(RbSettings::InstallGamefiles).toBool());
    ui.voiceCheckbox->setChecked(RbSettings::value(RbSettings::InstallVoice).toBool());
    ui.manualCheckbox->setChecked(RbSettings::value(RbSettings::InstallManual).toBool());

    ui.manualCombobox->addItem("PDF", "pdf");
    ui.manualCombobox->addItem("HTML (zip)", "zip");
    ui.manualCombobox->addItem("HTML", "html");

    // check if Rockbox is installed by looking after rockbox-info.txt.
    // If installed uncheck bootloader installation.
    RockboxInfo info(m_mountpoint);
    ui.bootloaderCheckbox->setChecked(!info.success());

    m_logger = nullptr;
    m_zipinstaller = nullptr;
    m_themesinstaller = nullptr;

    connect(ui.installButton, &QAbstractButton::clicked,
            this, &SelectiveInstallWidget::startInstall);
    connect(this, &SelectiveInstallWidget::installSkipped,
            this, &SelectiveInstallWidget::continueInstall);
    connect(ui.themesCustomize, &QAbstractButton::clicked,
            this, &SelectiveInstallWidget::customizeThemes);
    connect(ui.selectedVersion, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &SelectiveInstallWidget::selectedVersionChanged);
    // update version information. This also handles setting the previously
    // selected build type and bootloader disabling.
    updateVersion();
}


void SelectiveInstallWidget::selectedVersionChanged(int index)
{
    m_buildtype = static_cast<SystemInfo::BuildType>(ui.selectedVersion->itemData(index).toInt());
    bool voice = true;
    switch(m_buildtype) {
    case SystemInfo::BuildRelease:
        ui.selectedDescription->setText(tr("This is the latest stable "
                    "release available."));
        voice = true;
        break;
    case SystemInfo::BuildCurrent:
        ui.selectedDescription->setText(tr("The development version is "
                    "updated on every code change. Last update was on %1").arg(
                        ServerInfo::instance()->platformValue(ServerInfo::BleedingDate).toString()));
        voice = false;
        break;
    case SystemInfo::BuildCandidate:
        ui.selectedDescription->setText(tr("This will eventually become the "
                    "next Rockbox version. Install it to help testing."));
        voice = false;
        break;
    case SystemInfo::BuildDaily:
        ui.selectedDescription->setText(tr("Daily updated development version."));
        voice = true;
        break;
    }
    ui.voiceCheckbox->setEnabled(voice);
    ui.voiceCombobox->setEnabled(voice);
    ui.voiceLabel->setEnabled(voice);
}


void SelectiveInstallWidget::updateVersion(void)
{
    // get some configuration values globally
    m_mountpoint = RbSettings::value(RbSettings::Mountpoint).toString();
    m_target = RbSettings::value(RbSettings::CurrentPlatform).toString();
    m_blmethod = SystemInfo::platformValue(
            SystemInfo::BootloaderMethod, m_target).toString();

    if(m_logger != nullptr) {
        delete m_logger;
        m_logger = nullptr;
    }

    // re-populate all version items
    m_versions.clear();
    m_versions.insert(SystemInfo::BuildRelease, ServerInfo::instance()->platformValue(
                          ServerInfo::CurReleaseVersion).toString());
    // Don't populate RC or development selections if target has been retired.
    if (ServerInfo::instance()->platformValue(ServerInfo::CurStatus).toInt() != STATUS_RETIRED) {
        m_versions.insert(SystemInfo::BuildCurrent, ServerInfo::instance()->platformValue(
                              ServerInfo::BleedingRevision).toString());
        m_versions.insert(SystemInfo::BuildCandidate, ServerInfo::instance()->platformValue(
                              ServerInfo::RelCandidateVersion).toString());
        m_versions.insert(SystemInfo::BuildDaily, ServerInfo::instance()->platformValue(
                              ServerInfo::DailyVersion).toString());
    }

    ui.selectedVersion->clear();
    if(!m_versions[SystemInfo::BuildRelease].isEmpty()) {
        ui.selectedVersion->addItem(tr("Stable Release (Version %1)").arg(
                    m_versions[SystemInfo::BuildRelease]), SystemInfo::BuildRelease);
    }
    if(!m_versions[SystemInfo::BuildCurrent].isEmpty()) {
        ui.selectedVersion->addItem(tr("Development Version (Revison %1)").arg(
                    m_versions[SystemInfo::BuildCurrent]), SystemInfo::BuildCurrent);
    }
    if(!m_versions[SystemInfo::BuildCandidate].isEmpty()) {
        ui.selectedVersion->addItem(tr("Release Candidate (Revison %1)").arg(
                    m_versions[SystemInfo::BuildCandidate]), SystemInfo::BuildCandidate);
    }
    if(!m_versions[SystemInfo::BuildDaily].isEmpty()) {
        ui.selectedVersion->addItem(tr("Daily Build (%1)").arg(
                    m_versions[SystemInfo::BuildDaily]), SystemInfo::BuildDaily);
    }

    // select previously selected version
    int index = ui.selectedVersion->findData(
                static_cast<SystemInfo::BuildType>(RbSettings::value(RbSettings::Build).toInt()));
    if(index < 0) {
        if(!m_versions[SystemInfo::BuildRelease].isEmpty()) {
            index = ui.selectedVersion->findData(SystemInfo::BuildRelease);
        }
        else {
            index = ui.selectedVersion->findData(SystemInfo::BuildCurrent);
        }
    }
    ui.selectedVersion->setCurrentIndex(index);
    // check if Rockbox is installed. If it is untick the bootloader option, as
    // well as if the selected player doesn't need a bootloader.
    if(m_blmethod == "none") {
        ui.bootloaderCheckbox->setEnabled(false);
        ui.bootloaderCheckbox->setChecked(false);
        ui.bootloaderLabel->setEnabled(false);
        ui.bootloaderLabel->setText(tr("The selected player doesn't need a bootloader."));
    }
    else {
        ui.bootloaderCheckbox->setEnabled(true);
        ui.bootloaderLabel->setEnabled(true);
        ui.bootloaderLabel->setText(tr("The bootloader is required for starting "
                    "Rockbox. Installation of the bootloader is only necessary "
                    "on first time installation."));
        // check if Rockbox is installed by looking after rockbox-info.txt.
        // If installed uncheck bootloader installation.
        RockboxInfo info(m_mountpoint);
        ui.bootloaderCheckbox->setChecked(!info.success());
    }
    // populate languages for voice file.
    // FIXME: currently only english. Need to get the available languages from
    // build-info later.
    QVariant current = ui.voiceCombobox->currentData();
    QMap<QString, QStringList> languages = SystemInfo::languages(true);
    QStringList voicelangs;
    voicelangs << "english";
    ui.voiceCombobox->clear();
    for(int i = 0; i < voicelangs.size(); i++) {
        QString key = voicelangs.at(i);
        if(!languages.contains(key)) {
            LOG_WARNING() << "trying to add unknown language" << key;
            continue;
        }
        ui.voiceCombobox->addItem(languages.value(key).at(0), key);
    }
    // try to select the previously selected one again (if still present)
    // TODO: Fall back to system language if not found, or english.
    int sel = ui.voiceCombobox->findData(current);
    if(sel >= 0)
        ui.voiceCombobox->setCurrentIndex(sel);

}


void SelectiveInstallWidget::saveSettings(void)
{
    LOG_INFO() << "saving current settings";

    RbSettings::setValue(RbSettings::InstallRockbox, ui.rockboxCheckbox->isChecked());
    RbSettings::setValue(RbSettings::InstallFonts, ui.fontsCheckbox->isChecked());
    RbSettings::setValue(RbSettings::InstallThemes, ui.themesCheckbox->isChecked());
    RbSettings::setValue(RbSettings::InstallGamefiles, ui.gamefileCheckbox->isChecked());
    RbSettings::setValue(RbSettings::InstallVoice, ui.voiceCheckbox->isChecked());
    RbSettings::setValue(RbSettings::InstallManual, ui.manualCheckbox->isChecked());
    RbSettings::setValue(RbSettings::VoiceLanguage, ui.voiceCombobox->currentData().toString());
}


void SelectiveInstallWidget::startInstall(void)
{
    LOG_INFO() << "starting installation";
    saveSettings();

    m_installStage = 0;
    if(m_logger != nullptr) delete m_logger;
    m_logger = new ProgressLoggerGui(this);
    QString warning = Utils::checkEnvironment(false);
    if(!warning.isEmpty())
    {
        warning += "<br/>" + tr("Continue with installation?");
        if(QMessageBox::warning(this, tr("Really continue?"), warning,
                    QMessageBox::Ok | QMessageBox::Abort, QMessageBox::Abort)
                == QMessageBox::Abort)
        {
            emit installSkipped(true);
            return;
        }
    }

    m_logger->show();
    if(!QFileInfo(m_mountpoint).isDir()) {
        m_logger->addItem(tr("Mountpoint is wrong"), LOGERROR);
        m_logger->setFinished();
        return;
    }
    // start installation. No errors until now.
    continueInstall(false);

}


void SelectiveInstallWidget::continueInstall(bool error)
{
    LOG_INFO() << "continuing install with stage" << m_installStage;
    if(error) {
        LOG_ERROR() << "Last part returned error.";
        m_logger->setFinished();
        m_installStage = 9;
    }
    m_installStage++;
    switch(m_installStage) {
        case 0: LOG_ERROR() << "Something wrong!"; break;
        case 1: installBootloader(); break;
        case 2: installRockbox(); break;
        case 3: installFonts(); break;
        case 4: installThemes(); break;
        case 5: installGamefiles(); break;
        case 6: installVoicefile(); break;
        case 7: installManual(); break;
        case 8: installBootloaderPost(); break;
        default: break;
    }

    if(m_installStage > 8) {
        LOG_INFO() << "All install stages done.";
        m_logger->setFinished();
        if(m_blmethod != "none") {
            // check if Rockbox is installed by looking after rockbox-info.txt.
            // If installed uncheck bootloader installation.
            RockboxInfo info(m_mountpoint);
            ui.bootloaderCheckbox->setChecked(!info.success());
        }
    }
}


void SelectiveInstallWidget::installBootloader(void)
{
    if(ui.bootloaderCheckbox->isChecked()) {
        LOG_INFO() << "installing bootloader";

        QString platform = RbSettings::value(RbSettings::Platform).toString();
        QString backupDestination = "";

        // create installer
        BootloaderInstallBase *bl =
            BootloaderInstallHelper::createBootloaderInstaller(this,
                    SystemInfo::platformValue(SystemInfo::BootloaderMethod).toString());
        if(bl == nullptr) {
            m_logger->addItem(tr("No install method known."), LOGERROR);
            m_logger->setFinished();
            return;
        }

        // the bootloader install class does NOT use any GUI stuff.
        // All messages are passed via signals.
        connect(bl, SIGNAL(done(bool)), m_logger, SLOT(setFinished()));
        connect(bl, SIGNAL(done(bool)), this, SLOT(continueInstall(bool)));
        connect(bl, SIGNAL(logItem(QString, int)), m_logger, SLOT(addItem(QString, int)));
        connect(bl, SIGNAL(logProgress(int, int)), m_logger, SLOT(setProgress(int, int)));
        // pass Abort button click signal to current installer
        connect(m_logger, SIGNAL(aborted()), bl, SLOT(progressAborted()));

        // set bootloader filename. Do this now as installed() needs it.
        QStringList blfile = SystemInfo::platformValue(SystemInfo::BootloaderFile).toStringList();
        QStringList blfilepath;
        for(int a = 0; a < blfile.size(); a++) {
            blfilepath.append(RbSettings::value(RbSettings::Mountpoint).toString()
                    + blfile.at(a));
        }
        bl->setBlFile(blfilepath);
        QUrl url(SystemInfo::value(SystemInfo::BootloaderUrl).toString()
                + SystemInfo::platformValue(SystemInfo::BootloaderName).toString());
        bl->setBlUrl(url);
        bl->setLogfile(RbSettings::value(RbSettings::Mountpoint).toString()
                + "/.rockbox/rbutil.log");

        if(bl->installed() == BootloaderInstallBase::BootloaderRockbox) {
            if(QMessageBox::question(this, tr("Bootloader detected"),
                        tr("Bootloader already installed. Do you want to reinstall the bootloader?"),
                        QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
                // keep m_logger open for auto installs.
                // don't consider abort as error in auto-mode.
                m_logger->addItem(tr("Bootloader installation skipped"), LOGINFO);
                delete bl;
                emit installSkipped(true);
                return;
            }
        }
        else if(bl->installed() == BootloaderInstallBase::BootloaderOther
                && bl->capabilities() & BootloaderInstallBase::Backup)
        {
            QString targetFolder = SystemInfo::platformValue(SystemInfo::Name).toString()
                + " Firmware Backup";
            // remove invalid character(s)
            targetFolder.remove(QRegExp("[:/]"));
            if(QMessageBox::question(this, tr("Create Bootloader backup"),
                        tr("You can create a backup of the original bootloader "
                            "file. Press \"Yes\" to select an output folder on your "
                            "computer to save the file to. The file will get placed "
                            "in a new folder \"%1\" created below the selected folder.\n"
                            "Press \"No\" to skip this step.").arg(targetFolder),
                        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                backupDestination = QFileDialog::getExistingDirectory(this,
                        tr("Browse backup folder"), QDir::homePath());
                if(!backupDestination.isEmpty())
                    backupDestination += "/" + targetFolder;

                LOG_INFO() << "backing up to" << backupDestination;
                // backup needs to be done after the m_logger has been set up.
            }
        }

        if(bl->capabilities() & BootloaderInstallBase::NeedsOf)
        {
            int ret;
            ret = QMessageBox::information(this, tr("Prerequisites"),
                    bl->ofHint(),QMessageBox::Ok | QMessageBox::Abort);
            if(ret != QMessageBox::Ok) {
                // consider aborting an error to close window / abort automatic
                // installation.
                m_logger->addItem(tr("Bootloader installation aborted"), LOGINFO);
                m_logger->setFinished();
                emit installSkipped(true);
                return;
            }
            // open dialog to browse to of file
            QString offile;
            QString filter
                = SystemInfo::platformValue(SystemInfo::BootloaderFilter).toString();
            if(!filter.isEmpty()) {
                filter = tr("Bootloader files (%1)").arg(filter) + ";;";
            }
            filter += tr("All files (*)");
            offile = QFileDialog::getOpenFileName(this,
                    tr("Select firmware file"), QDir::homePath(), filter);
            if(!QFileInfo(offile).isReadable()) {
                m_logger->addItem(tr("Error opening firmware file"), LOGERROR);
                m_logger->setFinished();
                emit installSkipped(true);
                return;
            }
            if(!bl->setOfFile(offile, blfile)) {
                m_logger->addItem(tr("Error reading firmware file"), LOGERROR);
                m_logger->setFinished();
                emit installSkipped(true);
                return;
            }
        }

        // start install.
        if(!backupDestination.isEmpty()) {
            if(!bl->backup(backupDestination)) {
                if(QMessageBox::warning(this, tr("Backup error"),
                            tr("Could not create backup file. Continue?"),
                            QMessageBox::No | QMessageBox::Yes)
                        == QMessageBox::No) {
                    m_logger->setFinished();
                    return;
                }
            }
        }
        bl->install();

    }
    else {
        LOG_INFO() << "Bootloader install disabled.";
        emit installSkipped(false);
    }
}

void SelectiveInstallWidget::installBootloaderPost()
{
    // don't do anything if no bootloader install has been done.
    if(ui.bootloaderCheckbox->isChecked()) {
        QString msg = BootloaderInstallHelper::postinstallHints(
                RbSettings::value(RbSettings::Platform).toString());
        if(!msg.isEmpty()) {
            QMessageBox::information(this, tr("Manual steps required"), msg);
        }
    }
    emit installSkipped(false);
}


void SelectiveInstallWidget::installRockbox(void)
{
    if(ui.rockboxCheckbox->isChecked()) {
        LOG_INFO() << "installing Rockbox";
        QString url;

        RbSettings::setValue(RbSettings::Build, m_buildtype);
        RbSettings::sync();

        switch(m_buildtype) {
        case SystemInfo::BuildRelease:
            url = ServerInfo::instance()->platformValue(
                        ServerInfo::CurReleaseUrl, m_target).toString();
            break;
        case SystemInfo::BuildCurrent:
            url = ServerInfo::instance()->platformValue(
                        ServerInfo::CurDevelUrl, m_target).toString();
            break;
        case SystemInfo::BuildCandidate:
            url = ServerInfo::instance()->platformValue(
                        ServerInfo::RelCandidateUrl, m_target).toString();
            break;
        case SystemInfo::BuildDaily:
            url = ServerInfo::instance()->platformValue(
                        ServerInfo::DailyUrl, m_target).toString();
        }
        //! install build
        if(m_zipinstaller != nullptr) m_zipinstaller->deleteLater();
        m_zipinstaller = new ZipInstaller(this);
        m_zipinstaller->setUrl(url);
        m_zipinstaller->setLogSection("Rockbox (Base)");
        if(!RbSettings::value(RbSettings::CacheDisabled).toBool())
            m_zipinstaller->setCache(true);
        m_zipinstaller->setLogVersion(m_versions[m_buildtype]);
        m_zipinstaller->setMountPoint(m_mountpoint);

        connect(m_zipinstaller, SIGNAL(done(bool)), this, SLOT(continueInstall(bool)));

        connect(m_zipinstaller, SIGNAL(logItem(QString, int)), m_logger, SLOT(addItem(QString, int)));
        connect(m_zipinstaller, SIGNAL(logProgress(int, int)), m_logger, SLOT(setProgress(int, int)));
        connect(m_logger, SIGNAL(aborted()), m_zipinstaller, SLOT(abort()));
        m_zipinstaller->install();

    }
    else {
        LOG_INFO() << "Rockbox install disabled.";
        emit installSkipped(false);
    }
}


void SelectiveInstallWidget::installFonts(void)
{
    if(ui.fontsCheckbox->isChecked()) {
        LOG_INFO() << "installing Fonts";

        RockboxInfo installInfo(m_mountpoint);
        QString fontsurl;
        QString logversion;
        QString relversion = installInfo.release();
        if(!relversion.isEmpty()) {
            // release is empty for non-release versions (i.e. daily / current)
            logversion = installInfo.release();
        }
        fontsurl = SystemInfo::value(SystemInfo::FontUrl, m_buildtype).toString();
        fontsurl.replace("%RELVERSION%", relversion);

        // create new zip installer
        if(m_zipinstaller != nullptr) m_zipinstaller->deleteLater();
        m_zipinstaller = new ZipInstaller(this);
        m_zipinstaller->setUrl(fontsurl);
        m_zipinstaller->setLogSection("Fonts");
        m_zipinstaller->setLogVersion(logversion);
        m_zipinstaller->setMountPoint(m_mountpoint);
        if(!RbSettings::value(RbSettings::CacheDisabled).toBool())
            m_zipinstaller->setCache(true);

        connect(m_zipinstaller, SIGNAL(done(bool)), this, SLOT(continueInstall(bool)));
        connect(m_zipinstaller, SIGNAL(logItem(QString, int)), m_logger, SLOT(addItem(QString, int)));
        connect(m_zipinstaller, SIGNAL(logProgress(int, int)), m_logger, SLOT(setProgress(int, int)));
        connect(m_logger, SIGNAL(aborted()), m_zipinstaller, SLOT(abort()));
        m_zipinstaller->install();
    }
    else {
        LOG_INFO() << "Fonts install disabled.";
        emit installSkipped(false);
    }
}

void SelectiveInstallWidget::installVoicefile(void)
{
    if(ui.voiceCheckbox->isChecked() && ui.voiceCheckbox->isEnabled()) {
        LOG_INFO() << "installing Voice file";
        QString lang = ui.voiceCombobox->currentData().toString();

        RockboxInfo installInfo(m_mountpoint);
        QString voiceurl;
        QString logversion;
        QString relversion = installInfo.release();
        if(m_buildtype != SystemInfo::BuildRelease) {
            // release is empty for non-release versions (i.e. daily / current)
            logversion = installInfo.release();
        }
        voiceurl = SystemInfo::value(SystemInfo::VoiceUrl, m_buildtype).toString();
        voiceurl.replace("%RELVERSION%", m_versions[m_buildtype]);
        voiceurl.replace("%MODEL%", m_target);
        voiceurl.replace("%LANGUAGE%", lang);

        // create new zip installer
        if(m_zipinstaller != nullptr) m_zipinstaller->deleteLater();
        m_zipinstaller = new ZipInstaller(this);
        m_zipinstaller->setUrl(voiceurl);
        m_zipinstaller->setLogSection("Prerendered Voice (" + lang + ")");
        m_zipinstaller->setLogVersion(logversion);
        m_zipinstaller->setMountPoint(m_mountpoint);
        if(!RbSettings::value(RbSettings::CacheDisabled).toBool())
            m_zipinstaller->setCache(true);

        connect(m_zipinstaller, SIGNAL(done(bool)), this, SLOT(continueInstall(bool)));
        connect(m_zipinstaller, SIGNAL(logItem(QString, int)), m_logger, SLOT(addItem(QString, int)));
        connect(m_zipinstaller, SIGNAL(logProgress(int, int)), m_logger, SLOT(setProgress(int, int)));
        connect(m_logger, SIGNAL(aborted()), m_zipinstaller, SLOT(abort()));
        m_zipinstaller->install();
    }
    else {
        LOG_INFO() << "Voice install disabled.";
        emit installSkipped(false);
    }
}

void SelectiveInstallWidget::installManual(void)
{
    if(ui.manualCheckbox->isChecked() && ui.manualCheckbox->isEnabled()) {
        LOG_INFO() << "installing Manual";
        QString mantype = ui.manualCombobox->currentData().toString();

        RockboxInfo installInfo(m_mountpoint);
        QString manualurl;
        QString logversion;
        QString relversion = installInfo.release();
        if(m_buildtype != SystemInfo::BuildRelease) {
            // release is empty for non-release versions (i.e. daily / current)
            logversion = installInfo.release();
        }

        manualurl = SystemInfo::value(SystemInfo::ManualUrl, m_buildtype).toString();
        manualurl.replace("%RELVERSION%", m_versions[m_buildtype]);
        QString model = SystemInfo::platformValue(SystemInfo::Manual, m_target).toString();
        if(model.isEmpty())
            model = m_target;
        manualurl.replace("%MODEL%", model);

        if(mantype == "pdf")
            manualurl.replace("%FORMAT%", ".pdf");
        else
            manualurl.replace("%FORMAT%", "-html.zip");

        // create new zip installer
        if(m_zipinstaller != nullptr) m_zipinstaller->deleteLater();
        m_zipinstaller = new ZipInstaller(this);
        m_zipinstaller->setUrl(manualurl);
        m_zipinstaller->setLogSection("Manual (" + mantype + ")");
        m_zipinstaller->setLogVersion(logversion);
        m_zipinstaller->setMountPoint(m_mountpoint);
        if(!RbSettings::value(RbSettings::CacheDisabled).toBool())
            m_zipinstaller->setCache(true);
        // if type is html extract it.
        m_zipinstaller->setUnzip(mantype == "html");

        connect(m_zipinstaller, SIGNAL(done(bool)), this, SLOT(continueInstall(bool)));
        connect(m_zipinstaller, SIGNAL(logItem(QString, int)), m_logger, SLOT(addItem(QString, int)));
        connect(m_zipinstaller, SIGNAL(logProgress(int, int)), m_logger, SLOT(setProgress(int, int)));
        connect(m_logger, SIGNAL(aborted()), m_zipinstaller, SLOT(abort()));
        m_zipinstaller->install();
    }
    else {
        LOG_INFO() << "Manual install disabled.";
        emit installSkipped(false);
    }
}

void SelectiveInstallWidget::customizeThemes(void)
{
    if(m_themesinstaller == nullptr)
        m_themesinstaller = new ThemesInstallWindow(this);

    m_themesinstaller->setSelectOnly(true);
    m_themesinstaller->show();
}


void SelectiveInstallWidget::installThemes(void)
{
    if(ui.themesCheckbox->isChecked()) {
        LOG_INFO() << "installing themes";
        if(m_themesinstaller == nullptr)
            m_themesinstaller = new ThemesInstallWindow(this);

        connect(m_themesinstaller, SIGNAL(done(bool)), this, SLOT(continueInstall(bool)));
        m_themesinstaller->setLogger(m_logger);
        m_themesinstaller->setModal(true);
        m_themesinstaller->install();
    }
    else {
        LOG_INFO() << "Themes install disabled.";
        emit installSkipped(false);
    }
}

static const struct {
    const char *name;
    const char *pluginpath;
    SystemInfo::SystemInfos zipurl; // add new games to SystemInfo
} GamesList[] = {
    { "Doom",          "/.rockbox/rocks/games/doom.rock",         SystemInfo::DoomUrl      },
    { "Duke3D",        "/.rockbox/rocks/games/duke3d.rock",       SystemInfo::Duke3DUrl    },
    { "Quake",         "/.rockbox/rocks/games/quake.rock",        SystemInfo::QuakeUrl     },
    { "Puzzles fonts", "/.rockbox/rocks/games/sgt-blackbox.rock", SystemInfo::PuzzFontsUrl },
    { "Wolf3D",        "/.rockbox/rocks/games/wolf3d.rock",       SystemInfo::Wolf3DUrl    },
    { "XWorld",        "/.rockbox/rocks/games/xworld.rock",       SystemInfo::XWorldUrl    },
};

void SelectiveInstallWidget::installGamefiles(void)
{
    if(ui.gamefileCheckbox->isChecked()) {
        // build a list of zip urls that we need, then install
        QStringList gameUrls;
        QStringList gameNames;

        for(unsigned int i = 0; i < sizeof(GamesList) / sizeof(GamesList[0]); i++)
        {
            // check if installed Rockbox has this plugin.
            if(QFileInfo(m_mountpoint + GamesList[i].pluginpath).exists()) {
                gameNames.append(GamesList[i].name);
                gameUrls.append(SystemInfo::value(GamesList[i].zipurl).toString());
                LOG_INFO() << gameUrls.at(gameUrls.size() - 1);
            }
        }

        if(gameUrls.size() == 0)
        {
            m_logger->addItem(tr("Your installation doesn't require any game files, skipping."), LOGINFO);
            emit installSkipped(false);
            return;
        }

        LOG_INFO() << "installing gamefiles";

        // create new zip installer
        if(m_zipinstaller != nullptr) m_zipinstaller->deleteLater();
        m_zipinstaller = new ZipInstaller(this);

        m_zipinstaller->setUrl(gameUrls);
        m_zipinstaller->setLogSection(gameNames);
        m_zipinstaller->setLogVersion();
        m_zipinstaller->setMountPoint(m_mountpoint);
        if(!RbSettings::value(RbSettings::CacheDisabled).toBool())
            m_zipinstaller->setCache(true);
        connect(m_zipinstaller, SIGNAL(done(bool)), this, SLOT(continueInstall(bool)));
        connect(m_zipinstaller, SIGNAL(logItem(QString, int)), m_logger, SLOT(addItem(QString, int)));
        connect(m_zipinstaller, SIGNAL(logProgress(int, int)), m_logger, SLOT(setProgress(int, int)));
        connect(m_logger, SIGNAL(aborted()), m_zipinstaller, SLOT(abort()));
        m_zipinstaller->install();
    }
    else {
        LOG_INFO() << "Gamefile install disabled.";
        emit installSkipped(false);
    }
}

void SelectiveInstallWidget::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange) {
        ui.retranslateUi(this);
    } else {
        QWidget::changeEvent(e);
    }
}
