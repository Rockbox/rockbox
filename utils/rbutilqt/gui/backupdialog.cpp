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

#include <QThread>
#include <QDialog>
#include <QMessageBox>
#include <QFileDialog>
#include "backupdialog.h"
#include "ui_backupdialogfrm.h"
#include "rbsettings.h"
#include "progressloggergui.h"
#include "ziputil.h"
#include "rockboxinfo.h"
#include "Logger.h"

class BackupSizeThread : public QThread
{
    public:
        void run(void);
        void setPath(QString p) { m_path = p; }
        qint64 currentSize(void) { return m_currentSize; }

    private:
        QString m_path;
        qint64 m_currentSize;
};


void BackupSizeThread::run(void)
{
    LOG_INFO() << "Thread started, calculating" << m_path;
    m_currentSize = 0;

    QDirIterator it(m_path, QDirIterator::Subdirectories);
    while(it.hasNext()) {
        m_currentSize += QFileInfo(it.next()).size();
    }
    LOG_INFO() << "Thread done, sum:" << m_currentSize;
}


BackupDialog::BackupDialog(QWidget* parent) : QDialog(parent)
{
    ui.setupUi(this);

    m_thread = new BackupSizeThread();
    connect(m_thread, &QThread::finished, this, &BackupDialog::updateSizeInfo);

    connect(ui.buttonCancel, &QAbstractButton::clicked, this, &QWidget::close);
    connect(ui.buttonCancel, &QAbstractButton::clicked, m_thread, &QThread::quit);
    connect(ui.buttonChange, &QAbstractButton::clicked, this, &BackupDialog::changeBackupPath);
    connect(ui.buttonBackup, &QAbstractButton::clicked, this, &BackupDialog::backup);

    ui.backupSize->setText(tr("Installation size: calculating ..."));
    m_mountpoint = RbSettings::value(RbSettings::Mountpoint).toString();

    m_backupName = RbSettings::value(RbSettings::BackupPath).toString();
    if(m_backupName.isEmpty()) {
        m_backupName = m_mountpoint;
    }
    RockboxInfo info(m_mountpoint);
    m_backupName += "/.backup/rockbox-backup-" + info.version() + ".zip";
    ui.backupLocation->setText(QDir::toNativeSeparators(m_backupName));

    m_thread->setPath(m_mountpoint + "/.rockbox");
    m_thread->start();
}


void BackupDialog::changeBackupPath(void)
{
    QString backupString = QFileDialog::getSaveFileName(this,
        tr("Select Backup Filename"), m_backupName, "*.zip");
    // only update if a filename was entered, ignore if cancelled
    if(!backupString.isEmpty()) {
        m_backupName = backupString;
        ui.backupLocation->setText(QDir::toNativeSeparators(m_backupName));
        RbSettings::setValue(RbSettings::BackupPath, QFileInfo(m_backupName).absolutePath());
    }
}


void BackupDialog::updateSizeInfo(void)
{
    double size = m_thread->currentSize() / (1024 * 1024);
    QString unit = "MiB";

    if(size > 1024) {
        size /= 1024;
        unit = "GiB";
    }

    ui.backupSize->setText(tr("Installation size: %L1 %2").arg(size, 0, 'g', 4).arg(unit));
}


void BackupDialog::backup(void)
{
    if(QFileInfo(m_backupName).isFile()) {
        if(QMessageBox::warning(this, tr("File exists"),
                tr("The selected backup file already exists. Overwrite?"),
                QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
            return;
        }
    }
    m_logger = new ProgressLoggerGui(this);
    connect(m_logger, &ProgressLoggerGui::closed, this, &QWidget::close);
    m_logger->show();
    m_logger->addItem(tr("Starting backup ..."),LOGINFO);
    QCoreApplication::processEvents();

    // create dir, if it doesnt exist
    QFileInfo backupFile(m_backupName);
    if(!QDir(backupFile.path()).exists())
    {
        QDir a;
        a.mkpath(backupFile.path());
    }

    // create backup
    ZipUtil zip(this);
    connect(&zip, &ZipUtil::logProgress, m_logger, &ProgressLoggerGui::setProgress);
    connect(&zip, &ZipUtil::logItem, m_logger, &ProgressLoggerGui::addItem);
    zip.open(m_backupName, QuaZip::mdCreate);

    QString mp = m_mountpoint + "/.rockbox";
    if(zip.appendDirToArchive(mp, m_mountpoint)) {
        m_logger->addItem(tr("Backup successful."), LOGINFO);
    }
    else {
        m_logger->addItem(tr("Backup failed!"), LOGERROR);
    }
    zip.close();
    m_logger->setFinished();
}

