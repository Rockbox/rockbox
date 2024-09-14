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

#include <QDialog>
#include <QMessageBox>
#include <QTextCodec>

#include "ui_themesinstallfrm.h"
#include "themesinstallwindow.h"
#include "zipinstaller.h"
#include "progressloggergui.h"
#include "utils.h"
#include "rbsettings.h"
#include "playerbuildinfo.h"
#include "rockboxinfo.h"
#include "version.h"
#include "Logger.h"

ThemesInstallWindow::ThemesInstallWindow(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    ui.listThemes->setAlternatingRowColors(true);
    ui.listThemes->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui.listThemes->setSortingEnabled(true);
    ui.themePreview->clear();
    ui.themePreview->setText(tr("no theme selected"));
    ui.labelSize->setText(tr("no selection"));
    ui.listThemes->setLayoutDirection(Qt::LeftToRight);
    ui.themeDescription->setLayoutDirection(Qt::LeftToRight);

    connect(ui.buttonCancel, &QAbstractButton::clicked, this, &QWidget::close);
    connect(ui.buttonOk, &QAbstractButton::clicked, this, &ThemesInstallWindow::buttonOk);
    connect(ui.listThemes, &QListWidget::currentItemChanged,
            this, &ThemesInstallWindow::updateDetails);
    connect(ui.listThemes, &QListWidget::itemSelectionChanged, this, &ThemesInstallWindow::updateSize);
    connect(&igetter, &HttpGet::done, this, &ThemesInstallWindow::updateImage);

    if(!RbSettings::value(RbSettings::CacheDisabled).toBool())
        igetter.setCache(true);
    else
    {
        if(infocachedir.isEmpty())
        {
            infocachedir = QDir::tempPath() + "rbutil-themeinfo";
            QDir d = QDir::temp();
            d.mkdir("rbutil-themeinfo");
        }
        igetter.setCache(infocachedir);
    }

    logger = nullptr;
}

ThemesInstallWindow::~ThemesInstallWindow()
{
    if(!infocachedir.isEmpty())
        Utils::recursiveRmdir(infocachedir);
}


void ThemesInstallWindow::downloadInfo()
{
    // try to get the current build information
    getter = new HttpGet(this);
    RockboxInfo installInfo
        = RockboxInfo(RbSettings::value(RbSettings::Mountpoint).toString());

    themesInfo.open();
    LOG_INFO() << "downloading info to" << themesInfo.fileName();
    themesInfo.close();

    QString infoUrl = PlayerBuildInfo::instance()->value(PlayerBuildInfo::ThemesInfoUrl).toString();
    if (PlayerBuildInfo::instance()->value(PlayerBuildInfo::ThemeName).toString() != "") {
            infoUrl.replace("%TARGET%",
            PlayerBuildInfo::instance()->value(PlayerBuildInfo::ThemeName).toString());
    } else {
        infoUrl.replace("%TARGET%",
        RbSettings::value(RbSettings::CurrentPlatform).toString().split(".").at(0));
    }
    infoUrl.replace("%REVISION%", installInfo.revision());
    infoUrl.replace("%RELEASE%", installInfo.release());
    infoUrl.replace("%RBUTILVER%", VERSION);
    QUrl url = QUrl(infoUrl);
    LOG_INFO() << "Info URL:" << url;
    getter->setFile(&themesInfo);

    connect(getter, &HttpGet::done, this, &ThemesInstallWindow::downloadDone);
    connect(logger, &ProgressLoggerGui::aborted, getter, &HttpGet::abort);
    getter->getFile(url);
}


void ThemesInstallWindow::downloadDone(QNetworkReply::NetworkError error)
{
    LOG_INFO() << "Download done, error:" << error;

    disconnect(logger, &ProgressLoggerGui::aborted, getter, &HttpGet::abort);
    disconnect(logger, &ProgressLoggerGui::aborted, this, &QWidget::close);
    themesInfo.open();

    QSettings iniDetails(themesInfo.fileName(), QSettings::IniFormat, this);
#if QT_VERSION < 0x060000
    iniDetails.setIniCodec(QTextCodec::codecForName("UTF-8"));
#endif
    QStringList tl = iniDetails.childGroups();
    LOG_INFO() << "Theme site result:"
               << iniDetails.value("error/code").toString()
               << iniDetails.value("error/description").toString()
               << iniDetails.value("error/query").toString();

    if(error != QNetworkReply::NoError) {
        logger->addItem(tr("Network error: %1.\n"
                "Please check your network and proxy settings.")
                .arg(getter->errorString()), LOGERROR);
        getter->abort();
        logger->setFinished();
        disconnect(getter, &HttpGet::done, this, &ThemesInstallWindow::downloadDone);
        connect(logger, &ProgressLoggerGui::closed, this, &QWidget::close);
        return;
    }
    // handle possible error codes
    if(iniDetails.value("error/code").toInt() != 0 || !iniDetails.contains("error/code")) {
        LOG_ERROR() << "Theme site returned an error:"
                    << iniDetails.value("error/code");
        logger->addItem(tr("the following error occured:\n%1")
            .arg(iniDetails.value("error/description", "unknown error").toString()), LOGERROR);
        logger->setFinished();
        connect(logger, &ProgressLoggerGui::closed, this, &QWidget::close);
        return;
    }
    logger->addItem(tr("done."), LOGOK);
    logger->setFinished();
    logger->close();

    // setup list
    for(int i = 0; i < tl.size(); i++) {
        iniDetails.beginGroup(tl.at(i));
        // skip all themes without name field set (i.e. error section)
        if(iniDetails.value("name").toString().isEmpty()) {
            iniDetails.endGroup();
            continue;
        }
        LOG_INFO() << "adding to list:" << tl.at(i);
        // convert to unicode and replace HTML-specific entities
        QByteArray raw = iniDetails.value("name").toByteArray();
        QTextCodec* codec = QTextCodec::codecForHtml(raw);
        QString name = codec->toUnicode(raw);
        name.replace("&quot;", "\"").replace("&amp;", "&");
        name.replace("&lt;", "<").replace("&gt;", ">");
        QListWidgetItem *w = new QListWidgetItem;
        w->setData(Qt::DisplayRole, name.trimmed());
        w->setData(Qt::UserRole, tl.at(i));
        ui.listThemes->addItem(w);

        iniDetails.endGroup();
    }
    // check if there's a themes "MOTD" available
    if(iniDetails.contains("status/msg")) {
        // check if there's a localized msg available
        QString lang = RbSettings::value(RbSettings::Language).toString().split("_").at(0);
        QString msg;
        if(iniDetails.contains("status/msg." + lang))
            msg = iniDetails.value("status/msg." + lang).toString();
        else
            msg = iniDetails.value("status/msg").toString();
        LOG_INFO() << "MOTD" << msg;
        if(!msg.isEmpty())
            QMessageBox::information(this, tr("Information"), msg);
    }
}


void ThemesInstallWindow::updateSize(void)
{
    long size = 0;
    // sum up size for all selected themes
    QSettings iniDetails(themesInfo.fileName(), QSettings::IniFormat, this);
#if QT_VERSION < 0x060000
    iniDetails.setIniCodec(QTextCodec::codecForName("UTF-8"));
#endif
    int items = ui.listThemes->selectedItems().size();
    for(int i = 0; i < items; i++) {
        iniDetails.beginGroup(ui.listThemes->selectedItems()
                              .at(i)->data(Qt::UserRole).toString());
        size += iniDetails.value("size").toInt();
        iniDetails.endGroup();
    }
    ui.labelSize->setText(tr("Download size %L1 kiB (%n item(s))", "", items)
                             .arg((size + 512) / 1024));
}


void ThemesInstallWindow::updateDetails(QListWidgetItem* cur, QListWidgetItem* prev)
{
    if(cur == prev)
        return;

    QSettings iniDetails(themesInfo.fileName(), QSettings::IniFormat, this);
#if QT_VERSION < 0x060000
    iniDetails.setIniCodec(QTextCodec::codecForName("UTF-8"));
#endif

    QCoreApplication::processEvents();
    ui.themeDescription->setText(tr("fetching details for %1")
        .arg(cur->data(Qt::DisplayRole).toString()));
    ui.themePreview->clear();
    ui.themePreview->setText(tr("fetching preview ..."));
    imgData.clear();

    iniDetails.beginGroup(cur->data(Qt::UserRole).toString());

    QUrl img, txt;
    txt = QUrl(QString(PlayerBuildInfo::instance()->value(PlayerBuildInfo::ThemesUrl).toString() + "/"
        + iniDetails.value("descriptionfile").toString()));
    img = QUrl(QString(PlayerBuildInfo::instance()->value(PlayerBuildInfo::ThemesUrl).toString() + "/"
        + iniDetails.value("image").toString()));

    QString text;
    QTextCodec* codec = QTextCodec::codecForName("UTF-8");
    text = tr("<b>Author:</b> %1<hr/>").arg(codec->toUnicode(iniDetails
                    .value("author", tr("unknown")).toByteArray()));
    text += tr("<b>Version:</b> %1<hr/>").arg(codec->toUnicode(iniDetails
                    .value("version", tr("unknown")).toByteArray()));
    text += tr("<b>Description:</b> %1<hr/>").arg(codec->toUnicode(iniDetails
                    .value("about", tr("no description")).toByteArray()));

    text.replace("\n", "<br/>");
    ui.themeDescription->setHtml(text);
    iniDetails.endGroup();
    igetter.abort();
    igetter.getFile(img);
}


void ThemesInstallWindow::updateImage(QNetworkReply::NetworkError error)
{
    LOG_INFO() << "Updating image:"<< !error;

    if(error != QNetworkReply::NoError) {
        ui.themePreview->clear();
        ui.themePreview->setText(tr("Retrieving theme preview failed.\n"
            "HTTP response code: %1").arg(igetter.httpResponse()));
        return;
    }
    else {
        QPixmap p;
        imgData = igetter.readAll();
        if(imgData.isNull()) return;
        p.loadFromData(imgData);
        if(p.isNull()) {
            ui.themePreview->clear();
            ui.themePreview->setText(tr("no theme preview"));
        }
        else
            ui.themePreview->setPixmap(p);
    }
}


void ThemesInstallWindow::resizeEvent(QResizeEvent* e)
{
    (void)e;
    QPixmap p, q;
    QSize img;
    img.setHeight(ui.themePreview->height());
    img.setWidth(ui.themePreview->width());

    p.loadFromData(imgData);
    if(p.isNull()) return;
    q = p.scaled(img, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui.themePreview->setScaledContents(false);
    ui.themePreview->setPixmap(p);
}



void ThemesInstallWindow::show()
{
    QDialog::show();
    ui.buttonOk->setText(tr("Select"));

    if(!logger)
        logger = new ProgressLoggerGui(this);

    if(ui.listThemes->count() == 0) {
        logger->show();
        logger->addItem(tr("getting themes information ..."), LOGINFO);

        connect(logger, &ProgressLoggerGui::aborted, this, &QWidget::close);

        downloadInfo();
    }

}


void ThemesInstallWindow::abort()
{
    igetter.abort();
    logger->setFinished();
    close();
}


void ThemesInstallWindow::buttonOk(void)
{
    emit selected(ui.listThemes->selectedItems().size());
    close();
}


void ThemesInstallWindow::install()
{
    if(ui.listThemes->selectedItems().size() == 0) {
        logger->addItem(tr("No themes selected, skipping"), LOGINFO);
        return;
    }
    QStringList themes;
    QStringList names;
    QStringList version;
    QString zip;
    QSettings iniDetails(themesInfo.fileName(), QSettings::IniFormat, this);
    for(int i = 0; i < ui.listThemes->selectedItems().size(); i++) {
        iniDetails.beginGroup(ui.listThemes->selectedItems().at(i)->data(Qt::UserRole).toString());
        zip = PlayerBuildInfo::instance()->value(PlayerBuildInfo::ThemesUrl).toString()
                + "/" + iniDetails.value("archive").toString();
        themes.append(zip);
        names.append("Theme: " +
                ui.listThemes->selectedItems().at(i)->data(Qt::DisplayRole).toString());
        // if no version info is available use installation (current) date
        version.append(iniDetails.value("version",
                QDate().currentDate().toString("yyyyMMdd")).toString());
        iniDetails.endGroup();
    }
    LOG_INFO() << "installing:" << themes;

    if(logger == nullptr)
        logger = new ProgressLoggerGui(this);
    logger->show();
    QString mountPoint = RbSettings::value(RbSettings::Mountpoint).toString();
    LOG_INFO() << "mountpoint:" << mountPoint;
    // show dialog with error if mount point is wrong
    if(!QFileInfo(mountPoint).isDir()) {
        logger->addItem(tr("Mount point is wrong!"),LOGERROR);
        logger->setFinished();
        return;
    }

    installer = new ZipInstaller(this);
    installer->setUrl(themes);
    installer->setLogSection(names);
    installer->setLogVersion(version);
    installer->setMountPoint(mountPoint);
    if(!RbSettings::value(RbSettings::CacheDisabled).toBool())
        installer->setCache(true);

    connect(installer, &ZipInstaller::logItem, logger, &ProgressLoggerGui::addItem);
    connect(installer, &ZipInstaller::logProgress, logger, &ProgressLoggerGui::setProgress);
    connect(installer, &ZipInstaller::done, this, &ThemesInstallWindow::finished);
    connect(logger, &ProgressLoggerGui::aborted, installer, &ZipInstaller::abort);
    installer->install();
}


void ThemesInstallWindow::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange) {
        ui.retranslateUi(this);
    } else {
        QWidget::changeEvent(e);
    }
}

