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

#include "installwindow.h"
#include "ui_installwindowfrm.h"
#include "system.h"
#include "rbsettings.h"
#include "serverinfo.h"
#include "systeminfo.h"
#include "utils.h"
#include "rockboxinfo.h"
#include "ziputil.h"

InstallWindow::InstallWindow(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);

    connect(ui.radioStable, SIGNAL(toggled(bool)), this, SLOT(setDetailsStable(bool)));
    connect(ui.radioCandidate, SIGNAL(toggled(bool)), this, SLOT(setDetailsCandidate(bool)));
    connect(ui.radioCurrent, SIGNAL(toggled(bool)), this, SLOT(setDetailsCurrent(bool)));
    connect(ui.changeBackup, SIGNAL(pressed()), this, SLOT(changeBackupPath()));
    connect(ui.backup, SIGNAL(stateChanged(int)), this, SLOT(backupCheckboxChanged(int)));

    //! check if rockbox is already installed
    RockboxInfo rbinfo(RbSettings::value(RbSettings::Mountpoint).toString());
    QString version = rbinfo.version();

    if(version != "") {
        ui.Backupgroup->show();
        m_backupName = RbSettings::value(RbSettings::Mountpoint).toString();
        if(!m_backupName.endsWith("/")) m_backupName += "/";
        m_backupName += ".backup/rockbox-backup-"+version+".zip";
        // for some reason the label doesn't return its final size yet.
        // Delay filling ui.backupLocation until the checkbox is changed.
    }
    else {
        ui.Backupgroup->hide();
    }
    backupCheckboxChanged(Qt::Unchecked);

    if(ServerInfo::value(ServerInfo::CurReleaseVersion).toString().isEmpty()) {
        ui.radioStable->setEnabled(false);
    }

    // try to use the old selection first. If no selection has been made
    // in the past, use a preselection based on released status.
    QString lastinstalled = RbSettings::value(RbSettings::Build).toString();
    if(lastinstalled == "stable"
        && !ServerInfo::value(ServerInfo::CurReleaseVersion).toString().isEmpty()) {
        ui.radioStable->setChecked(true);
    }
    else if(lastinstalled == "rc"
            && !ServerInfo::value(ServerInfo::RelCandidateVersion).toString().isEmpty()) {
        ui.radioCandidate->setChecked(true);
    }
    else if(lastinstalled == "current") {
        ui.radioCurrent->setChecked(true);
    }
    else if(!ServerInfo::value(ServerInfo::CurReleaseVersion).toString().isEmpty()) {
        ui.radioStable->setChecked(true);
        ui.radioStable->setEnabled(true);
    }
    else {
        ui.radioCurrent->setChecked(true);
        ui.radioStable->setEnabled(false);
        ui.radioStable->setChecked(false);
    }
    if(ServerInfo::value(ServerInfo::RelCandidateVersion).toString().isEmpty()) {
        ui.radioCandidate->setEnabled(false);
    }

}


void InstallWindow::resizeEvent(QResizeEvent *e)
{
    (void)e;

    // recalculate width of elided text.
    updateBackupLocation();
}


void InstallWindow::updateBackupLocation(void)
{
    ui.backupLocation->setText(QDir::toNativeSeparators(
        fontMetrics().elidedText(tr("Backup to %1").arg(m_backupName),
        Qt::ElideMiddle, ui.backupLocation->size().width())));
}


void InstallWindow::backupCheckboxChanged(int state)
{
    if(state == Qt::Checked)
    {
        ui.backupLocation->show();
        ui.changeBackup->show();
        // update backup location display.
        updateBackupLocation();
    }
    else
    {
        ui.backupLocation->hide();
        ui.changeBackup->hide();
    }
}


void InstallWindow::accept()
{
    QString url;
    logger = new ProgressLoggerGui(this);
    logger->show();
    QString mountPoint = RbSettings::value(RbSettings::Mountpoint).toString();
    qDebug() << "[Install] mountpoint:" << mountPoint;
    // show dialog with error if mount point is wrong
    if(!QFileInfo(mountPoint).isDir()) {
        logger->addItem(tr("Mount point is wrong!"),LOGERROR);
        logger->setFinished();
        return;
    }

    QString myversion;
    if(ui.radioStable->isChecked()) {
        url = ServerInfo::value(ServerInfo::CurReleaseUrl).toString();
        RbSettings::setValue(RbSettings::Build, "stable");
        myversion = ServerInfo::value(ServerInfo::CurReleaseVersion).toString();
    }
    else if(ui.radioCurrent->isChecked()) {
        url = ServerInfo::value(ServerInfo::CurDevelUrl).toString();
        RbSettings::setValue(RbSettings::Build, "current");
        myversion = "r" + ServerInfo::value(ServerInfo::BleedingRevision).toString();
    }
    else if(ui.radioCandidate->isChecked()) {
        url = ServerInfo::value(ServerInfo::RelCandidateUrl).toString();
        RbSettings::setValue(RbSettings::Build, "rc");
        myversion = ServerInfo::value(ServerInfo::RelCandidateVersion).toString();
    }
    else {
        qDebug() << "[Install] no build selected -- this shouldn't happen";
        return;
    }
    RbSettings::sync();

    QString warning = Utils::checkEnvironment(false);
    if(!warning.isEmpty())
    {
        if(QMessageBox::warning(this, tr("Really continue?"), warning,
            QMessageBox::Ok | QMessageBox::Abort, QMessageBox::Abort)
            == QMessageBox::Abort)
        {
            logger->addItem(tr("Aborted!"),LOGERROR);
            logger->setFinished();
            return;
        }
    }

    //! check if we should backup
    if(ui.backup->isChecked())
    {
        logger->addItem(tr("Beginning Backup..."),LOGINFO);
        QCoreApplication::processEvents();

        //! create dir, if it doesnt exist
        QFileInfo backupFile(m_backupName);
        if(!QDir(backupFile.path()).exists())
        {
            QDir a;
            a.mkpath(backupFile.path());
        }

        //! create backup
        bool result = true;
        ZipUtil zip(this);
        connect(&zip, SIGNAL(logProgress(int, int)), logger, SLOT(setProgress(int, int)));
        connect(&zip, SIGNAL(logItem(QString, int)), logger, SLOT(addItem(QString, int)));
        zip.open(m_backupName, QuaZip::mdCreate);
        QString mp = RbSettings::value(RbSettings::Mountpoint).toString();
        QString folder = mp + "/.rockbox";
        result = zip.appendDirToArchive(folder, mp);
        zip.close();
        if(result) {
            logger->addItem(tr("Backup finished."), LOGINFO);
        }
        else {
            logger->addItem(tr("Backup failed!"), LOGERROR);
            logger->setFinished();
            return;
        }
    }

    //! install build
    installer = new ZipInstaller(this);
    installer->setUrl(url);
    installer->setLogSection("Rockbox (Base)");
    if(!RbSettings::value(RbSettings::CacheDisabled).toBool()
        && !ui.checkBoxCache->isChecked())
    {
        installer->setCache(true);
    }
    installer->setLogVersion(myversion);
    installer->setMountPoint(mountPoint);

    connect(installer, SIGNAL(done(bool)), this, SLOT(done(bool)));

    connect(installer, SIGNAL(logItem(QString, int)), logger, SLOT(addItem(QString, int)));
    connect(installer, SIGNAL(logProgress(int, int)), logger, SLOT(setProgress(int, int)));
    connect(installer, SIGNAL(done(bool)), logger, SLOT(setFinished()));
    connect(logger, SIGNAL(aborted()), installer, SLOT(abort()));
    installer->install();

}

void InstallWindow::changeBackupPath()
{
    QString backupString = QFileDialog::getSaveFileName(this,
        tr("Select Backup Filename"), m_backupName, "*.zip");
    // only update if a filename was entered, ignore if cancelled
    if(!backupString.isEmpty()) {
        m_backupName = backupString;
        updateBackupLocation();
    }
}


// Zip installer has finished
void InstallWindow::done(bool error)
{
    qDebug() << "[Install] done, error:" << error;

    if(error)
    {
        logger->setFinished();
        return;
    }

    // no error, close the window, when the logger is closed
    connect(logger,SIGNAL(closed()),this,SLOT(close()));
    // add platform info to log file for later detection
    QSettings installlog(RbSettings::value(RbSettings::Mountpoint).toString()
            + "/.rockbox/rbutil.log", QSettings::IniFormat, 0);
    installlog.setValue("platform", RbSettings::value(RbSettings::Platform).toString());
    installlog.sync();
}


void InstallWindow::setDetailsCurrent(bool show)
{
    if(show) {
        ui.labelDetails->setText(tr("This is the absolute up to the minute "
                "Rockbox built. The development version will get updated every time "
                "a change is made. Latest development version is %1 (%2).")
                .arg(ServerInfo::value(ServerInfo::BleedingRevision).toString(),
                    ServerInfo::value(ServerInfo::BleedingDate).toString()));
    }
}


void InstallWindow::setDetailsStable(bool show)
{
    if(show) {
        ui.labelDetails->setText(
            tr("This is the last released version of Rockbox."));

        if(!ServerInfo::value(ServerInfo::CurReleaseVersion).toString().isEmpty())
            ui.labelNote->setText(tr("<b>Note:</b> "
            "The lastest stable version is %1.")
                    .arg(ServerInfo::value(ServerInfo::CurReleaseVersion).toString()));
        else ui.labelNote->setText("");
    }
}


void InstallWindow::setDetailsCandidate(bool show)
{
    if(show) {
        ui.labelDetails->setText(
            tr("This is the release candidate for the next Rockbox version."
                "<br/>A release candidate is intended for testing. It will "
                "receive bugfixes and eventually become the next stable "
                "release of Rockbox. If you want to help testing Rockbox and "
                "improve the next release install the release candidate."));

        if(!ServerInfo::value(ServerInfo::CurReleaseVersion).toString().isEmpty())
            ui.labelNote->setText(tr("<b>Note:</b> "
            "The lastest release candidate is %1.")
                    .arg(ServerInfo::value(ServerInfo::RelCandidateVersion).toString()));
        else ui.labelNote->setText("");
    }
}


void InstallWindow::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange) {
        ui.retranslateUi(this);
    } else {
        QWidget::changeEvent(e);
    }
}

