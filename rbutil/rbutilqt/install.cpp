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


void Install::setProxy(QUrl proxy_url)
{
    proxy = proxy_url;
    qDebug() << "Install::setProxy" << proxy;
}


void Install::accept()
{
    logger = new ProgressLoggerGui(this);
    logger->show();
    QString mountPoint = userSettings->value("mountpoint").toString();
    qDebug() << "mountpoint:" << userSettings->value("mountpoint").toString();
    // show dialog with error if mount point is wrong
    if(!QFileInfo(mountPoint).isDir()) {
        logger->addItem(tr("Mount point is wrong!"),LOGERROR);
        logger->abort();
        return;
    }

    QString myversion;
    QString buildname;
    devices->beginGroup(userSettings->value("platform").toString());
    buildname = devices->value("platform").toString();
    devices->endGroup();
    if(ui.radioStable->isChecked()) {
        file = QString("%1/rockbox-%2-%3.zip")
                .arg(devices->value("download_url").toString(),
                    devices->value("last_release").toString(), buildname);
        fileName = QString("rockbox-%1-%2.zip")
                   .arg(devices->value("last_release").toString(), buildname);
        userSettings->setValue("build", "stable");
        myversion = version.value("rel_rev");
    }
    else if(ui.radioArchived->isChecked()) {
        file = QString("%1%2/rockbox-%3-%4.zip")
            .arg(devices->value("daily_url").toString(),
            buildname, buildname, version.value("arch_date"));
        fileName = QString("rockbox-%1-%2.zip")
            .arg(buildname, version.value("arch_date"));
        userSettings->setValue("build", "archived");
        myversion = "r" + version.value("arch_rev") + "-" + version.value("arch_date");
    }
    else if(ui.radioCurrent->isChecked()) {
        file = QString("%1%2/rockbox.zip")
            .arg(devices->value("bleeding_url").toString(), buildname);
        fileName = QString("rockbox.zip");
        userSettings->setValue("build", "current");
        myversion = "r" + version.value("bleed_rev");
    }
    else {
        qDebug() << "no build selected -- this shouldn't happen";
        return;
    }
    userSettings->sync();

    installer = new ZipInstaller(this);
    installer->setUrl(file);
    installer->setProxy(proxy);
    installer->setLogSection("Rockbox (Base)");
    if(!userSettings->value("cachedisable").toBool()
        && !ui.radioCurrent->isChecked()
        && !ui.checkBoxCache->isChecked())
        installer->setCache(userSettings->value("cachepath",
                            QDir::tempPath()).toString());

    installer->setLogVersion(myversion);
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
    // add platform info to log file for later detection
    QSettings installlog(userSettings->value("mountpoint").toString()
            + "/.rockbox/rbutil.log", QSettings::IniFormat, 0);
    installlog.setValue("platform", userSettings->value("platform").toString());
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


void Install::setDeviceSettings(QSettings *dev)
{
    devices = dev;
    qDebug() << "Install::setDeviceSettings:" << devices;
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

    if(!version.value("rel_rev").isEmpty()) {
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

void Install::setUserSettings(QSettings *user)
{
    userSettings = user;
}
