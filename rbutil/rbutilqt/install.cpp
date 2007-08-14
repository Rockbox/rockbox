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

Install::Install(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);

    connect(ui.radioCurrent, SIGNAL(toggled(bool)), this, SLOT(setCached(bool)));
    connect(ui.radioStable, SIGNAL(toggled(bool)), this, SLOT(setDetailsStable(bool)));
    connect(ui.radioCurrent, SIGNAL(toggled(bool)), this, SLOT(setDetailsCurrent(bool)));
    connect(ui.radioArchived, SIGNAL(toggled(bool)), this, SLOT(setDetailsArchived(bool)));
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


void Install::accept()
{
    logger = new ProgressLoggerGui(this);
    logger->show();
    QString mountPoint = userSettings->value("defaults/mountpoint").toString();
    qDebug() << "mountpoint:" << userSettings->value("defaults/mountpoint").toString();
    // show dialog with error if mount point is wrong
    if(!QFileInfo(mountPoint).isDir()) {
        logger->addItem(tr("Mount point is wrong!"),LOGERROR);
        logger->abort();
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

    installer = new ZipInstaller(this);
    installer->setUrl(file);
    installer->setProxy(proxy);
    installer->setLogSection("rockboxbase");
    installer->setMountPoint(mountPoint);
    installer->install(logger);
    
    connect(installer, SIGNAL(done(bool)), this, SLOT(done(bool)));
  
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

void Install::setUserSettings(QSettings *user)
{
    userSettings = user;
}
