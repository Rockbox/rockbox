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

#include <QtGui>
#include <QDebug>
#include "manualwidget.h"
#include "rbutilqt.h"
#include "rbsettings.h"
#include "serverinfo.h"
#include "systeminfo.h"

ManualWidget::ManualWidget(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    ui.radioPdf->setChecked(true);
    platform = RbSettings::value(RbSettings::Platform).toString();
    connect(ui.buttonDownloadManual, SIGNAL(clicked()), this, SLOT(downloadManual()));
}


void ManualWidget::updateManual()
{
    if(!RbSettings::value(RbSettings::Platform).toString().isEmpty())
    {
        ui.labelPdfManual->setText(tr("<a href='%1'>PDF Manual</a>")
            .arg(ServerInfo::platformValue(platform, ServerInfo::ManualPdfUrl).toString()));
        ui.labelHtmlManual->setText(tr("<a href='%1'>HTML Manual (opens in browser)</a>")
            .arg(ServerInfo::platformValue(platform, ServerInfo::ManualHtmlUrl).toString()));
    }
    else {
        ui.labelPdfManual->setText(tr("Select a device for a link to the correct manual"));
        ui.labelHtmlManual->setText(tr("<a href='%1'>Manual Overview</a>")
            .arg("http://www.rockbox.org/manual.shtml"));
    }
}


void ManualWidget::downloadManual(void)
{
    if(RbUtilQt::chkConfig(this)) {
        return;
    }
    if(QMessageBox::question(this, tr("Confirm download"),
       tr("Do you really want to download the manual? The manual will be saved "
            "to the root folder of your player."),
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    QString manual = SystemInfo::value(SystemInfo::CurManual).toString();
    if(manual.isEmpty()) {
        manual = "rockbox-" + SystemInfo::value(SystemInfo::CurBuildserverModel).toString();
    }

    ProgressLoggerGui* logger = new ProgressLoggerGui(this);
    logger->show();
    ZipInstaller *installer = new ZipInstaller(this);
    installer->setMountPoint(RbSettings::value(RbSettings::Mountpoint).toString());
    if(!RbSettings::value(RbSettings::CacheDisabled).toBool())
        installer->setCache(true);

    if(ui.radioPdf->isChecked()) {
        installer->setUrl(ServerInfo::platformValue(platform,
                    ServerInfo::ManualPdfUrl).toString());
        installer->setLogSection("Manual (PDF)");
        installer->setTarget("/" + manual + ".pdf");
    }
    else {
        installer->setUrl(ServerInfo::platformValue(platform,
                    ServerInfo::ManualZipUrl).toString());
        installer->setLogSection("Manual (HTML)");
        installer->setTarget("/" + manual + "-" + "-html.zip");
    }
    installer->setLogVersion();
    installer->setUnzip(false);

    connect(installer, SIGNAL(logItem(QString, int)), logger, SLOT(addItem(QString, int)));
    connect(installer, SIGNAL(logProgress(int, int)), logger, SLOT(setProgress(int, int)));
    connect(installer, SIGNAL(done(bool)), logger, SLOT(setFinished()));
    connect(logger, SIGNAL(aborted()), installer, SLOT(abort()));
    installer->install();
}

