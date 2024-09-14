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
#include "playerbuildinfo.h"
#include "rbsettings.h"
#include "rockboxinfo.h"
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
    ui.pluginDataCheckbox->setChecked(RbSettings::value(RbSettings::InstallPluginData).toBool());
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
    m_themesinstaller = new ThemesInstallWindow(this);
    connect(m_themesinstaller, &ThemesInstallWindow::selected,
            [this](int count) {ui.themesCheckbox->setChecked(count > 0);});

    connect(ui.installButton, &QAbstractButton::clicked,
            this, &SelectiveInstallWidget::startInstall);
    connect(this, &SelectiveInstallWidget::installSkipped,
            this, &SelectiveInstallWidget::continueInstall);
    connect(ui.themesCustomize, &QAbstractButton::clicked,
            [this]() { m_themesinstaller->show(); } );
    connect(ui.selectedVersion, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &SelectiveInstallWidget::selectedVersionChanged);
    // update version information. This also handles setting the previously
    // selected build type and bootloader disabling.
    updateVersion();
}


void SelectiveInstallWidget::selectedVersionChanged(int index)
{
    m_buildtype = static_cast<PlayerBuildInfo::BuildType>(ui.selectedVersion->itemData(index).toInt());
    bool voice = true;
    switch(m_buildtype) {
    case PlayerBuildInfo::TypeRelease:
        ui.selectedDescription->setText(tr("This is the latest stable "
                    "release available."));
        voice = true;
        break;
    case PlayerBuildInfo::TypeDevel:
        ui.selectedDescription->setText(tr("The development version is "
                "updated on every code change."));
        voice = false;
        break;
    case PlayerBuildInfo::TypeCandidate:
        ui.selectedDescription->setText(tr("This will eventually become the "
                    "next Rockbox version. Install it to help testing."));
        voice = false;
        break;
    case PlayerBuildInfo::TypeDaily:
        ui.selectedDescription->setText(tr("Daily updated development version."));
        voice = true;
        break;
    }
    ui.voiceCheckbox->setEnabled(voice);
    ui.voiceCombobox->setEnabled(voice);
    ui.voiceLabel->setEnabled(voice);
    ui.voiceCheckbox->setToolTip(voice ? "" : tr("Not available for the selected version"));
    QString fontsurl = PlayerBuildInfo::instance()->value(
                        PlayerBuildInfo::BuildFontUrl, m_buildtype).toString();
    ui.fontsCheckbox->setEnabled(!fontsurl.isEmpty());
    QString manualurl = PlayerBuildInfo::instance()->value(
                        PlayerBuildInfo::BuildManualUrl, m_buildtype).toString();
    ui.manualCheckbox->setEnabled(!manualurl.isEmpty());

    updateVoiceLangs();
}


void SelectiveInstallWidget::updateVersion(void)
{
    // get some configuration values globally
    m_mountpoint = RbSettings::value(RbSettings::Mountpoint).toString();
    m_target = RbSettings::value(RbSettings::CurrentPlatform).toString();
    m_blmethod = PlayerBuildInfo::instance()->value(
            PlayerBuildInfo::BootloaderMethod, m_target).toString();

    if(m_logger != nullptr) {
        delete m_logger;
        m_logger = nullptr;
    }

    // re-populate all version items
    QMap<PlayerBuildInfo::BuildType, QString> types;
    types[PlayerBuildInfo::TypeRelease] = tr("Stable Release (Version %1)");
    if (PlayerBuildInfo::instance()->value(PlayerBuildInfo::BuildStatus).toInt() != STATUS_RETIRED) {
        types[PlayerBuildInfo::TypeCandidate] = tr("Release Candidate (Revison %1)");
        types[PlayerBuildInfo::TypeDaily] = tr("Daily Build (%1)");
        types[PlayerBuildInfo::TypeDevel] = tr("Development Version (Revison %1)");
    }

    ui.selectedVersion->clear();
    for(auto i = types.begin(); i != types.end(); i++) {
        QString version = PlayerBuildInfo::instance()->value(
                    PlayerBuildInfo::BuildVersion, i.key()).toString();
        if(!version.isEmpty())
            ui.selectedVersion->addItem(i.value().arg(version), i.key());
    }

    // select previously selected version
    int index = ui.selectedVersion->findData(
            static_cast<PlayerBuildInfo::BuildType>(RbSettings::value(RbSettings::Build).toInt()));
    if(index < 0) {
        index = ui.selectedVersion->findData(PlayerBuildInfo::TypeRelease);
        if(index < 0) {
            index = ui.selectedVersion->findData(PlayerBuildInfo::TypeDevel);
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

    updateVoiceLangs();
}

void SelectiveInstallWidget::updateVoiceLangs()
{
    // populate languages for voice file.
    QVariant current = ui.voiceCombobox->currentData();
    QMap<QString, QVariant> langs = PlayerBuildInfo::instance()->value(
                PlayerBuildInfo::LanguageList).toMap();
    QStringList voicelangs = PlayerBuildInfo::instance()->value(
                PlayerBuildInfo::BuildVoiceLangs, m_buildtype).toStringList();
    ui.voiceCombobox->clear();
    for(auto it = langs.begin(); it != langs.end(); it++) {
        if(voicelangs.contains(it.key())) {
            ui.voiceCombobox->addItem(it.value().toString(), it.key());
            LOG_INFO() << "available voices: adding" << it.key();
        }

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
    RbSettings::setValue(RbSettings::InstallPluginData, ui.pluginDataCheckbox->isChecked());
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
        case 5: installPluginData(); break;
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

        QString backupDestination = "";

        // create installer
        BootloaderInstallBase *bl =
            BootloaderInstallHelper::createBootloaderInstaller(this,
                    PlayerBuildInfo::instance()->value(
                        PlayerBuildInfo::BootloaderMethod).toString());
        if(bl == nullptr) {
            m_logger->addItem(tr("No install method known."), LOGERROR);
            m_logger->setFinished();
            return;
        }

        // the bootloader install class does NOT use any GUI stuff.
        // All messages are passed via signals.
        connect(bl, &BootloaderInstallBase::done, this, &SelectiveInstallWidget::continueInstall);
        connect(bl, &BootloaderInstallBase::logItem, m_logger, &ProgressLoggerGui::addItem);
        connect(bl, &BootloaderInstallBase::logProgress, m_logger, &ProgressLoggerGui::setProgress);
        // pass Abort button click signal to current installer
        connect(m_logger, SIGNAL(aborted()), bl, SLOT(progressAborted()));

        // set bootloader filename. Do this now as installed() needs it.
        QStringList blfile = PlayerBuildInfo::instance()->value(
                    PlayerBuildInfo::BootloaderFile).toStringList();
        bl->setBlFile(RbSettings::value(RbSettings::Mountpoint).toString(), blfile);
        QUrl url(PlayerBuildInfo::instance()->value(PlayerBuildInfo::BootloaderUrl).toString()
                + PlayerBuildInfo::instance()->value(PlayerBuildInfo::BootloaderName).toString());
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
            QString targetFolder = PlayerBuildInfo::instance()->value(
                        PlayerBuildInfo::DisplayName).toString()
                + " Firmware Backup";
            // remove invalid character(s)
            targetFolder.remove(QRegularExpression("[:/]"));
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
                = PlayerBuildInfo::instance()->value(PlayerBuildInfo::BootloaderFilter).toString();
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

void SelectiveInstallWidget::installBootloaderHints()
{
    if(ui.bootloaderCheckbox->isChecked()) {
        QString msg = BootloaderInstallHelper::preinstallHints(
                RbSettings::value(RbSettings::Platform).toString());
        if(!msg.isEmpty()) {
            QMessageBox::information(this, tr("Manual steps required"), msg);
        }
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

        url = PlayerBuildInfo::instance()->value(
                    PlayerBuildInfo::BuildUrl, m_buildtype).toString();
        //! install build
        if(m_zipinstaller != nullptr) m_zipinstaller->deleteLater();
        m_zipinstaller = new ZipInstaller(this);
        m_zipinstaller->setUrl(url);
        m_zipinstaller->setLogSection("Rockbox (Base)");
        if(!RbSettings::value(RbSettings::CacheDisabled).toBool())
            m_zipinstaller->setCache(true);
        m_zipinstaller->setLogVersion(PlayerBuildInfo::instance()->value(
                PlayerBuildInfo::BuildVersion, m_buildtype).toString());
        m_zipinstaller->setMountPoint(m_mountpoint);

        connect(m_zipinstaller, &ZipInstaller::done, this, &SelectiveInstallWidget::continueInstall);

        connect(m_zipinstaller, &ZipInstaller::logItem, m_logger, &ProgressLoggerGui::addItem);
        connect(m_zipinstaller, &ZipInstaller::logProgress, m_logger, &ProgressLoggerGui::setProgress);
        connect(m_logger, &ProgressLoggerGui::aborted, m_zipinstaller, &ZipInstaller::abort);
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
        fontsurl = PlayerBuildInfo::instance()->value(
                        PlayerBuildInfo::BuildFontUrl, m_buildtype).toString();
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

        connect(m_zipinstaller, &ZipInstaller::done, this, &SelectiveInstallWidget::continueInstall);
        connect(m_zipinstaller, &ZipInstaller::logItem, m_logger, &ProgressLoggerGui::addItem);
        connect(m_zipinstaller, &ZipInstaller::logProgress, m_logger, &ProgressLoggerGui::setProgress);
        connect(m_logger, &ProgressLoggerGui::aborted, m_zipinstaller, &ZipInstaller::abort);
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
        if(m_buildtype != PlayerBuildInfo::TypeRelease) {
            // release is empty for non-release versions (i.e. daily / current)
            logversion = installInfo.release();
        }
        voiceurl = PlayerBuildInfo::instance()->value(
                        PlayerBuildInfo::BuildVoiceUrl, m_buildtype).toString();
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

        connect(m_zipinstaller, &ZipInstaller::done, this, &SelectiveInstallWidget::continueInstall);
        connect(m_zipinstaller, &ZipInstaller::logItem, m_logger, &ProgressLoggerGui::addItem);
        connect(m_zipinstaller, &ZipInstaller::logProgress, m_logger, &ProgressLoggerGui::setProgress);
        connect(m_logger, &ProgressLoggerGui::aborted, m_zipinstaller, &ZipInstaller::abort);
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
        if(m_buildtype != PlayerBuildInfo::TypeRelease) {
            // release is empty for non-release versions (i.e. daily / current)
            logversion = installInfo.release();
        }

        manualurl = PlayerBuildInfo::instance()->value(
                        PlayerBuildInfo::BuildManualUrl, m_buildtype).toString();
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

        connect(m_zipinstaller, &ZipInstaller::done, this, &SelectiveInstallWidget::continueInstall);
        connect(m_zipinstaller, &ZipInstaller::logItem, m_logger, &ProgressLoggerGui::addItem);
        connect(m_zipinstaller, &ZipInstaller::logProgress, m_logger, &ProgressLoggerGui::setProgress);
        connect(m_logger, &ProgressLoggerGui::aborted, m_zipinstaller, &ZipInstaller::abort);
        m_zipinstaller->install();
    }
    else {
        LOG_INFO() << "Manual install disabled.";
        emit installSkipped(false);
    }
}

void SelectiveInstallWidget::installThemes(void)
{
    if(ui.themesCheckbox->isChecked()) {
        LOG_INFO() << "installing themes";

        m_themesinstaller->setLogger(m_logger);
        m_themesinstaller->setModal(true);
        m_themesinstaller->install();
        connect(m_themesinstaller, &ThemesInstallWindow::finished,
                this, &SelectiveInstallWidget::continueInstall);
    }
    else {
        LOG_INFO() << "Themes install disabled.";
        emit installSkipped(false);
    }
}

static const struct {
    const char *name;                   // display name
    const char *rockfile;               // rock file to look for
    PlayerBuildInfo::BuildInfo zipurl;  // download url
} PluginDataFiles[] = {
    { "Doom",          "games/doom.rock",         PlayerBuildInfo::DoomUrl      },
    { "Duke3D",        "games/duke3d.rock",       PlayerBuildInfo::Duke3DUrl    },
    { "Quake",         "games/quake.rock",        PlayerBuildInfo::QuakeUrl     },
    { "Puzzles fonts", "games/sgt-blackbox.rock", PlayerBuildInfo::PuzzFontsUrl },
    { "Wolf3D",        "games/wolf3d.rock",       PlayerBuildInfo::Wolf3DUrl    },
    { "XRick",         "games/xrick.rock",        PlayerBuildInfo::XRickUrl     },
    { "XWorld",        "games/xworld.rock",       PlayerBuildInfo::XWorldUrl    },
    { "MIDI Patchset", "viewers/midi.rock",       PlayerBuildInfo::MidiPatchsetUrl },
};

void SelectiveInstallWidget::installPluginData(void)
{
    if(ui.pluginDataCheckbox->isChecked()) {
        // build a list of zip urls that we need, then install
        QStringList dataUrls;
        QStringList dataName;

        for(size_t i = 0; i < sizeof(PluginDataFiles) / sizeof(PluginDataFiles[0]); i++)
        {
            // check if installed Rockbox has this plugin.
            if(QFileInfo::exists(m_mountpoint + "/.rockbox/rocks/" + PluginDataFiles[i].rockfile)) {
                dataName.append(PluginDataFiles[i].name);
                // game URLs do not depend on the actual build type, but we need
                // to pass it (simplifies the API, and will allow to make them
                // type specific later if needed)
                dataUrls.append(PlayerBuildInfo::instance()->value(
                                    PluginDataFiles[i].zipurl, m_buildtype).toString());
            }
        }

        if(dataUrls.size() == 0)
        {
            m_logger->addItem(
                    tr("Your installation doesn't require any plugin data files, skipping."),
                    LOGINFO);
            emit installSkipped(false);
            return;
        }

        LOG_INFO() << "installing plugin data files";

        // create new zip installer
        if(m_zipinstaller != nullptr) m_zipinstaller->deleteLater();
        m_zipinstaller = new ZipInstaller(this);

        m_zipinstaller->setUrl(dataUrls);
        m_zipinstaller->setLogSection(dataName);
        m_zipinstaller->setLogVersion();
        m_zipinstaller->setMountPoint(m_mountpoint);
        if(!RbSettings::value(RbSettings::CacheDisabled).toBool())
            m_zipinstaller->setCache(true);
        connect(m_zipinstaller, &ZipInstaller::done, this, &SelectiveInstallWidget::continueInstall);
        connect(m_zipinstaller, &ZipInstaller::logItem, m_logger, &ProgressLoggerGui::addItem);
        connect(m_zipinstaller, &ZipInstaller::logProgress, m_logger, &ProgressLoggerGui::setProgress);
        connect(m_logger, &ProgressLoggerGui::aborted, m_zipinstaller, &ZipInstaller::abort);
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
