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

#include "install.h"
#include "ui_installfrm.h"
#include "rbzip.h"
#include "detect.h"

Install::Install(RbSettings *sett,QWidget *parent) : QDialog(parent)
{
    settings = sett;
    ui.setupUi(this);

    connect(ui.radioCurrent, SIGNAL(toggled(bool)), this, SLOT(setCached(bool)));
    connect(ui.radioStable, SIGNAL(toggled(bool)), this, SLOT(setDetailsStable(bool)));
    connect(ui.radioCurrent, SIGNAL(toggled(bool)), this, SLOT(setDetailsCurrent(bool)));
    connect(ui.radioArchived, SIGNAL(toggled(bool)), this, SLOT(setDetailsArchived(bool)));
    connect(ui.changeBackup, SIGNAL(pressed()), this, SLOT(changeBackupPath()));
    connect(ui.backup, SIGNAL(stateChanged(int)), this, SLOT(backupCheckboxChanged(int)));

    //! check if rockbox is already installed
    QString version = Detect::installedVersion(settings->mountpoint());

    if(version != "")
    {
        ui.Backupgroup->show();
        m_backupName = settings->mountpoint();
        if(!m_backupName.endsWith("/")) m_backupName += "/";
        m_backupName += ".backup/rockbox-backup-"+version+".zip";
        // for some reason the label doesn't return its final size yet.
        // Delay filling ui.backupLocation until the checkbox is changed.
    }
    else
    {
        ui.Backupgroup->hide();
    }
    backupCheckboxChanged(Qt::Unchecked);
}


void Install::resizeEvent(QResizeEvent *e)
{
    (void)e;

    // recalculate width of elided text.
    updateBackupLocation();
}


void Install::updateBackupLocation(void)
{
    ui.backupLocation->setText(QDir::toNativeSeparators(
        fontMetrics().elidedText(tr("Backup to %1").arg(m_backupName),
        Qt::ElideMiddle, ui.backupLocation->size().width())));
}


void Install::backupCheckboxChanged(int state)
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


void Install::setCached(bool cache)
{
    ui.checkBoxCache->setEnabled(!cache);
}


void Install::accept()
{
    logger = new ProgressLoggerGui(this);
    logger->show();
    QString mountPoint = settings->mountpoint();
    qDebug() << "mountpoint:" << settings->mountpoint();
    // show dialog with error if mount point is wrong
    if(!QFileInfo(mountPoint).isDir()) {
        logger->addItem(tr("Mount point is wrong!"),LOGERROR);
        logger->abort();
        return;
    }

    QString myversion;
    QString buildname = settings->curPlatformName();
    if(ui.radioStable->isChecked()) {
        file = QString("%1/%2/rockbox-%3-%4.zip")
                .arg(settings->releaseUrl(), version.value("rel_rev"),
                    buildname, version.value("rel_rev"));
        fileName = QString("rockbox-%1-%2.zip")
                   .arg(version.value("rel_rev"), buildname);
        settings->setBuild("stable");
        myversion = version.value("rel_rev");
    }
    else if(ui.radioArchived->isChecked()) {
        file = QString("%1%2/rockbox-%3-%4.zip")
            .arg(settings->dailyUrl(),
            buildname, buildname, version.value("arch_date"));
        fileName = QString("rockbox-%1-%2.zip")
            .arg(buildname, version.value("arch_date"));
        settings->setBuild("archived");
        myversion = "r" + version.value("arch_rev") + "-" + version.value("arch_date");
    }
    else if(ui.radioCurrent->isChecked()) {
        file = QString("%1%2/rockbox.zip")
            .arg(settings->bleedingUrl(), buildname);
        fileName = QString("rockbox.zip");
        settings->setBuild("current");
        myversion = "r" + version.value("bleed_rev");
    }
    else {
        qDebug() << "no build selected -- this shouldn't happen";
        return;
    }
    settings->sync();

    QString warning = Detect::check(settings, false, settings->curTargetId());
    if(!warning.isEmpty())
    {
        if(QMessageBox::warning(this, tr("Really continue?"), warning,
            QMessageBox::Ok | QMessageBox::Abort, QMessageBox::Abort)
            == QMessageBox::Abort)
        {
            logger->addItem(tr("Aborted!"),LOGERROR);
            logger->abort();
            return;
        }
    }

    //! check if we should backup
    if(ui.backup->isChecked())
    {
        logger->addItem(tr("Beginning Backup..."),LOGINFO);

        //! create dir, if it doesnt exist
        QFileInfo backupFile(m_backupName);
        if(!QDir(backupFile.path()).exists())
        {
            QDir a;
            a.mkpath(backupFile.path());
        }

        //! create backup
        RbZip backup;
        connect(&backup,SIGNAL(zipProgress(int,int)),logger,SLOT(setProgress(int,int)));
        if(backup.createZip(m_backupName,settings->mountpoint() + "/.rockbox") == Zip::Ok)
        {
            logger->addItem(tr("Backup successful"),LOGOK);
        }
        else
        {
            logger->addItem(tr("Backup failed!"),LOGERROR);
            logger->abort();
            return;
        }
    }

    //! install build
    installer = new ZipInstaller(this);
    installer->setUrl(file);
    installer->setLogSection("Rockbox (Base)");
    if(!settings->cacheDisabled()
        && !ui.checkBoxCache->isChecked())
    {
        installer->setCache(true);
    }
    installer->setLogVersion(myversion);
    installer->setMountPoint(mountPoint);

    connect(installer, SIGNAL(done(bool)), this, SLOT(done(bool)));

    installer->install(logger);

}

void Install::changeBackupPath()
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
void Install::done(bool error)
{
    qDebug() << "Install::done, error:" << error;

    if(error)
    {
       logger->abort();
        return;
    }

    // no error, close the window, when the logger is closed
    connect(logger,SIGNAL(closed()),this,SLOT(close()));
    // add platform info to log file for later detection
    QSettings installlog(settings->mountpoint()
            + "/.rockbox/rbutil.log", QSettings::IniFormat, 0);
    installlog.setValue("platform", settings->curPlatform());
    installlog.sync();
}


void Install::setDetailsCurrent(bool show)
{
    if(show) {
        ui.labelDetails->setText(tr("This is the absolute up to the minute "
                "Rockbox built. A current build will get updated every time "
                "a change is made. Latest version is r%1 (%2).")
                .arg(version.value("bleed_rev"), version.value("bleed_date")));
        if(version.value("rel_rev").isEmpty())
            ui.labelNote->setText(tr("<b>Note:</b> This option will always "
                "download a fresh copy. "
                "<b>This is the recommended version.</b>"));
        else
            ui.labelNote->setText(tr("<b>Note:</b> This option will always "
                "download a fresh copy."));
    }
}


void Install::setDetailsStable(bool show)
{
    if(show) {
        ui.labelDetails->setText(
            tr("This is the last released version of Rockbox."));

        if(!version.value("rel_rev").isEmpty())
            ui.labelNote->setText(tr("<b>Note:</b>"
            "The lastest released version is %1. "
            "<b>This is the recommended version.</b>")
                    .arg(version.value("rel_rev")));
        else ui.labelNote->setText("");
    }
}


void Install::setDetailsArchived(bool show)
{
    if(show) {
        ui.labelDetails->setText(tr("These are automatically built each day "
        "from the current development source code. This generally has more "
        "features than the last stable release but may be much less stable. "
        "Features may change regularly."));
        ui.labelNote->setText(tr("<b>Note:</b> archived version is r%1 (%2).")
            .arg(version.value("arch_rev"), version.value("arch_date")));
    }
}



void Install::setVersionStrings(QMap<QString, QString> ver)
{
    version = ver;
    // version strings map is as following:
    // rel_rev release version revision id
    // rel_date release version release date
    // same for arch_* and bleed_*

    if(version.value("arch_rev").isEmpty()) {
        ui.radioArchived->setEnabled(false);
        qDebug() << "no information about archived version available!";
    }
    if(version.value("rel_rev").isEmpty()) {
        ui.radioStable->setEnabled(false);
    }

    // try to use the old selection first. If no selection has been made
    // in the past, use a preselection based on released status.
    if(settings->build() == "stable" && !version.value("rel_rev").isEmpty())
        ui.radioStable->setChecked(true);
    else if(settings->build() == "archived")
        ui.radioArchived->setChecked(true);
    else if(settings->build() == "current")
        ui.radioCurrent->setChecked(true);
    else if(!version.value("rel_rev").isEmpty()) {
        ui.radioStable->setChecked(true);
        ui.radioStable->setEnabled(true);
        QFont font;
        font.setBold(true);
        ui.radioStable->setFont(font);
    }
    else {
        ui.radioCurrent->setChecked(true);
        ui.radioStable->setEnabled(false);
        ui.radioStable->setChecked(false);
        QFont font;
        font.setBold(true);
        ui.radioCurrent->setFont(font);
    }

    qDebug() << "Install::setVersionStrings" << version;
}


