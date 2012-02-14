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
    connect(ui.buttonDownloadManual, SIGNAL(clicked()), this, SLOT(downloadManual()));
}


QString ManualWidget::manualUrl(ManualFormat format)
{
    if(RbSettings::value(RbSettings::Platform).toString().isEmpty()) {
        return QString();
    }

    QString buildservermodel = SystemInfo::value(SystemInfo::CurBuildserverModel).toString();
    QString modelman = SystemInfo::value(SystemInfo::CurManual).toString();
    QString manualbasename;

    if(modelman.isEmpty()) {
        manualbasename = "rockbox-" + buildservermodel;
    }
    else {
        manualbasename = "rockbox-" + modelman;
    }

    QString manual = SystemInfo::value(SystemInfo::ManualUrl).toString();
    switch(format) {
        case ManualPdf:
            manual.replace("%EXTENSION%", "pdf");
            break;
        case ManualHtml:
            manual.replace("%EXTENSION%", "html");
            manualbasename += "/rockbox-build";
            break;
        case ManualZip:
            manual.replace("%EXTENSION%", "-html.zip");
            manualbasename += "/rockbox-build";
            break;
        default:
            break;
    };

    manual.replace("%MANUALBASENAME%", manualbasename);
    return manual;
}


void ManualWidget::updateManual()
{
    if(!RbSettings::value(RbSettings::Platform).toString().isEmpty())
    {
        ui.labelPdfManual->setText(tr("<a href='%1'>PDF Manual</a>")
            .arg(manualUrl(ManualPdf)));
        ui.labelHtmlManual->setText(tr("<a href='%1'>HTML Manual (opens in browser)</a>")
            .arg(manualUrl(ManualHtml)));
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

    QDate date = QDate::fromString(ServerInfo::value(
                ServerInfo::DailyDate).toString(), Qt::ISODate);
    QString manualurl;

    ProgressLoggerGui* logger = new ProgressLoggerGui(this);
    logger->show();
    ZipInstaller *installer = new ZipInstaller(this);
    installer->setMountPoint(RbSettings::value(RbSettings::Mountpoint).toString());
    if(!RbSettings::value(RbSettings::CacheDisabled).toBool())
        installer->setCache(true);

    if(ui.radioPdf->isChecked()) {
        manualurl = manualUrl(ManualPdf);
        installer->setLogSection("Manual (PDF)");
        installer->setTarget("/" + manual + ".pdf");
    }
    else {
        manualurl = manualUrl(ManualZip);
        installer->setLogSection("Manual (HTML)");
        installer->setTarget("/" + manual + "-" + date.toString("yyyyMMdd") + "-html.zip");
    }
    qDebug() << "[ManualWidget] Manual URL:" << manualurl;

    installer->setLogVersion(ServerInfo::value(ServerInfo::DailyDate).toString());
    installer->setUrl(manualurl);
    installer->setUnzip(false);

    connect(installer, SIGNAL(logItem(QString, int)), logger, SLOT(addItem(QString, int)));
    connect(installer, SIGNAL(logProgress(int, int)), logger, SLOT(setProgress(int, int)));
    connect(installer, SIGNAL(done(bool)), logger, SLOT(setFinished()));
    connect(logger, SIGNAL(aborted()), installer, SLOT(abort()));
    installer->install();
}

