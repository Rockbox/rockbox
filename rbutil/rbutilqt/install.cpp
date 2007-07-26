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
#include "ui_installprogressfrm.h"
#include "httpget.h"
#include "zip/zip.h"
#include "zip/unzip.h"

#include <QtGui>
#include <QtNetwork>

Install::Install(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);

    connect(ui.radioCurrent, SIGNAL(toggled(bool)), this, SLOT(setCached(bool)));
    connect(ui.radioStable, SIGNAL(toggled(bool)), this, SLOT(setDetailsStable(bool)));
    connect(ui.radioCurrent, SIGNAL(toggled(bool)), this, SLOT(setDetailsCurrent(bool)));
    connect(ui.radioArchived, SIGNAL(toggled(bool)), this, SLOT(setDetailsArchived(bool)));
    connect(ui.buttonBrowse, SIGNAL(clicked()), this, SLOT(browseFolder()));
}


void Install::setCached(bool cache)
{
    ui.checkBoxCache->setEnabled(!cache);
}


void Install::setReleased(QString rel)
{
    releasever = rel;
    if(!rel.isEmpty()) {
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
    qDebug() << "Install::setReleased" << releasever;

}


void Install::setProxy(QUrl proxy_url)
{
    proxy = proxy_url;
    qDebug() << "Install::setProxy" << proxy;
}


void Install::setMountPoint(QString mount)
{
    QFileInfo m(mount);
    if(m.isDir()) {
        ui.lineMountPoint->clear();
        ui.lineMountPoint->insert(mount);
    }
}


void Install::browseFolder()
{
    QFileDialog browser(this);
    if(QFileInfo(ui.lineMountPoint->text()).isDir())
        browser.setDirectory(ui.lineMountPoint->text());
    else
        browser.setDirectory("/media");
    browser.setReadOnly(true);
    browser.setFileMode(QFileDialog::DirectoryOnly);
    browser.setAcceptMode(QFileDialog::AcceptOpen);
    if(browser.exec()) {
        qDebug() << browser.directory();
        QStringList files = browser.selectedFiles();
        setMountPoint(files.at(0));
    }
}


void Install::accept()
{
    QDialog *downloadProgress = new QDialog(this);
    dp.setupUi(downloadProgress);
    // connect close button now as it's needed if we break upon an error
    connect(dp.buttonAbort, SIGNAL(clicked()), downloadProgress, SLOT(close()));
    // show dialog with error if mount point is wrong
    if(QFileInfo(ui.lineMountPoint->text()).isDir()) {
        mountPoint = ui.lineMountPoint->text();
        userSettings->setValue("defaults/mountpoint", mountPoint);
    }
    else {
        dp.listProgress->addItem(tr("Mount point is wrong!"));
        dp.buttonAbort->setText(tr("&Ok"));
        downloadProgress->show();
        return;
    }

    if(ui.radioStable->isChecked()) {
        file = "stable"; // FIXME: this is wrong!
        fileName = QString("rockbox.zip");
        userSettings->setValue("defaults/build", "stable");
    }
    else if(ui.radioArchived->isChecked()) {
        file = QString("%1%2/rockbox-%3-%4.zip")
            .arg(devices->value("daily_url").toString(),
            userSettings->value("defaults/platform").toString(),
            userSettings->value("defaults/platform").toString(),
            archived);
        fileName = QString("rockbox-%1-%2.zip")
            .arg(userSettings->value("defaults/platform").toString(),
            archived);
        userSettings->setValue("defaults/build", "archived");
    }
    else if(ui.radioCurrent->isChecked()) {
        file = QString("%1%2/rockbox.zip")
            .arg(devices->value("bleeding_url").toString(),
        userSettings->value("defaults/platform").toString());
        fileName = QString("rockbox.zip");
        userSettings->setValue("defaults/build", "current");
    }
    else {
        qDebug() << "no build selected -- this shouldn't happen";
        return;
    }
    userSettings->sync();

    dp.listProgress->addItem(tr("Downloading file %1.%2")
            .arg(QFileInfo(file).baseName(), QFileInfo(file).completeSuffix()));

    // temporary file needs to be opened to get the filename
    downloadFile.open();
    fileName = downloadFile.fileName();
    downloadFile.close();
    // get the real file.
    getter = new HttpGet(this);
    getter->setProxy(proxy);
    getter->setFile(&downloadFile);

    getter->getFile(QUrl(file));
    connect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
    connect(dp.buttonAbort, SIGNAL(clicked()), getter, SLOT(abort()));
    connect(getter, SIGNAL(dataReadProgress(int, int)), this, SLOT(updateDataReadProgress(int, int)));
    connect(getter, SIGNAL(downloadDone(int, bool)), this, SLOT(downloadRequestFinished(int, bool)));

    downloadProgress->show();
}

void Install::downloadRequestFinished(int id, bool error)
{
    qDebug() << "Install::downloadRequestFinished" << id << error;
    qDebug() << "error:" << getter->errorString();

    downloadDone(error);

}

void Install::downloadDone(bool error)
{
    qDebug() << "Install::downloadDone, error:" << error;

    // update progress bar
    int max = dp.progressBar->maximum();
    if(max == 0) {
        max = 100;
        dp.progressBar->setMaximum(max);
    }
    dp.progressBar->setValue(max);
    if(getter->httpResponse() != 200) {
        dp.listProgress->addItem(tr("Download error: received HTTP error %1.").arg(getter->httpResponse()));
        dp.buttonAbort->setText(tr("&Ok"));
        connect(dp.buttonAbort, SIGNAL(clicked()), this, SLOT(close()));
        return;
    }
    if(error) {
        dp.listProgress->addItem(tr("Download error: %1").arg(getter->errorString()));
        dp.buttonAbort->setText(tr("&Ok"));
        connect(dp.buttonAbort, SIGNAL(clicked()), this, SLOT(close()));
        return;
    }
    else dp.listProgress->addItem(tr("Download finished."));

    // unzip downloaded file
    qDebug() << "about to unzip the downloaded file" << fileName << "to" << mountPoint;

    dp.listProgress->addItem(tr("Extracting file."));

    qDebug() << "file to unzip: " << fileName;
    UnZip::ErrorCode ec;
    UnZip uz;
    ec = uz.openArchive(fileName);
    if(ec != UnZip::Ok) {
        dp.listProgress->addItem(tr("Opening archive failed: %1.")
            .arg(uz.formatError(ec)));
        dp.buttonAbort->setText(tr("&Ok"));
        connect(dp.buttonAbort, SIGNAL(clicked()), this, SLOT(close()));
        return;
    }
    ec = uz.extractAll(mountPoint);
    if(ec != UnZip::Ok) {
        dp.listProgress->addItem(tr("Extracting failed: %1.")
            .arg(uz.formatError(ec)));
        dp.buttonAbort->setText(tr("&Ok"));
        connect(dp.buttonAbort, SIGNAL(clicked()), this, SLOT(close()));
        return;
    }

    dp.listProgress->addItem(tr("creating installation log"));
    
       
    QStringList zipContents = uz.fileList();
       
    QSettings installlog(mountPoint + "/.rockbox/rbutil.log", QSettings::IniFormat, 0); 

    installlog.beginGroup("rockboxbase");
    for(int i = 0; i < zipContents.size(); i++)
    {
        installlog.setValue(zipContents.at(i),installlog.value(zipContents.at(i),0).toInt()+1);
    }
    installlog.endGroup();
   

    // remove temporary file
    downloadFile.remove();

    dp.listProgress->addItem(tr("Extraction finished successfully."));
    dp.buttonAbort->setText(tr("&Ok"));
    connect(dp.buttonAbort, SIGNAL(clicked()), this, SLOT(close()));

}


void Install::setDetailsCurrent(bool show)
{
    if(show) {
        ui.labelDetails->setText(tr("This is the absolute up to the minute "
                "Rockbox built. A current build will get updated every time "
                "a change is made."));
        if(releasever == "")
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

        if(releasever != "") ui.labelNote->setText(tr("<b>Note:</b>"
            "The lastest released version is %1. "
            "<b>This is the recommended version.</b>").arg(releasever));
        else ui.labelNote->setText("");
    }
}


void Install::setDetailsArchived(bool show)
{
    if(show) {
        ui.labelDetails->setText(tr("These are automatically built each day "
        "from the current development source code. This generally has more "
        "features than the last release but may be much less stable. "
        "Features may change regularly."));
        ui.labelNote->setText(tr("<b>Note:</b> archived version is %1.")
            .arg(archived));
    }
}


void Install::setDeviceSettings(QSettings *dev)
{
    devices = dev;
    qDebug() << "Install::setDeviceSettings:" << devices;
}


void Install::setArchivedString(QString string)
{
    archived = string;
    if(archived.isEmpty()) {
        ui.radioArchived->setEnabled(false);
        qDebug() << "no information about archived version available!";
    }
    qDebug() << "Install::setArchivedString" << archived;
}


void Install::updateDataReadProgress(int read, int total)
{
    dp.progressBar->setMaximum(total);
    dp.progressBar->setValue(read);
    qDebug() << "progress:" << read << "/" << total;
}

void Install::setUserSettings(QSettings *user)
{
    userSettings = user;
}
