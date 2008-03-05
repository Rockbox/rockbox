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

#include <QtGui>

#include "ui_installthemesfrm.h"
#include "installthemes.h"
#include "installzip.h"
#include "progressloggergui.h"
#include "utils.h"

ThemesInstallWindow::ThemesInstallWindow(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    ui.listThemes->setAlternatingRowColors(true);
    ui.listThemes->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui.themePreview->clear();
    ui.themePreview->setText(tr("no theme selected"));

    connect(ui.buttonCancel, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui.buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui.buttonOkAll, SIGNAL(clicked()), this, SLOT(acceptAll()));
}

ThemesInstallWindow::~ThemesInstallWindow()
{
    if(infocachedir!="")
        recRmdir(infocachedir);
}

QString ThemesInstallWindow::resolution()
{
    return settings->curResolution();
}


void ThemesInstallWindow::downloadInfo()
{
    // try to get the current build information
    getter = new HttpGet(this);
    connect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));

    qDebug() << "downloading themes info";
    themesInfo.open();
    qDebug() << "file:" << themesInfo.fileName();
    themesInfo.close();

    QUrl url;
    url = QUrl(settings->themeUrl() + "/rbutilqt.php?res=" + resolution());
    qDebug() << "downloadInfo()" << url;
    qDebug() << url.queryItems();
    if(settings->cacheOffline())
        getter->setCache(true);
    getter->setFile(&themesInfo);
    getter->getFile(url);
}


void ThemesInstallWindow::downloadDone(int id, bool error)
{
    downloadDone(error);
    qDebug() << "downloadDone(bool) =" << id << error;
}


void ThemesInstallWindow::downloadDone(bool error)
{
    qDebug() << "downloadDone(bool) =" << error;

    disconnect(logger, SIGNAL(aborted()), getter, SLOT(abort()));
    disconnect(logger, SIGNAL(aborted()), this, SLOT(close()));
    themesInfo.open();

    QSettings iniDetails(themesInfo.fileName(), QSettings::IniFormat, this);
    QStringList tl = iniDetails.childGroups();
    qDebug() << tl.size();
    qDebug() << iniDetails.value("error/code").toString()
            << iniDetails.value("error/description").toString()
            << iniDetails.value("error/query").toString();

    if(error) {
        logger->addItem(tr("Network error: %1.\nPlease check your network and proxy settings.")
                .arg(getter->errorString()), LOGERROR);
        getter->abort();
        logger->abort();
        disconnect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
        connect(logger, SIGNAL(closed()), this, SLOT(close()));
        return;
    }
    // handle possible error codes
    if(iniDetails.value("error/code").toInt() != 0 || !iniDetails.contains("error/code")) {
        qDebug() << "error!";
        logger->addItem(tr("the following error occured:\n%1")
            .arg(iniDetails.value("error/description", "unknown error").toString()), LOGERROR);
        logger->abort();
        connect(logger, SIGNAL(closed()), this, SLOT(close()));
        return;
    }
    logger->addItem(tr("done."), LOGOK);
    logger->abort();
    logger->close();

    // setup list
    for(int i = 0; i < tl.size(); i++) {
        iniDetails.beginGroup(tl.at(i));
        // skip all themes without name field set (i.e. error section)
        if(iniDetails.value("name").toString().isEmpty()) continue;
        QListWidgetItem *w = new QListWidgetItem;
        w->setData(Qt::DisplayRole, iniDetails.value("name").toString());
        w->setData(Qt::UserRole, tl.at(i));
        ui.listThemes->addItem(w);

        iniDetails.endGroup();
    }
    connect(ui.listThemes, SIGNAL(currentRowChanged(int)), this, SLOT(updateDetails(int)));
}


void ThemesInstallWindow::updateDetails(int row)
{
    if(row == currentItem) return;

    currentItem = row;
    qDebug() << "updateDetails(int) =" << row;
    QSettings iniDetails(themesInfo.fileName(), QSettings::IniFormat, this);
    ui.themeDescription->setText(tr("fetching details for %1")
        .arg(ui.listThemes->item(row)->data(Qt::DisplayRole).toString()));
    ui.themePreview->clear();
    ui.themePreview->setText(tr("fetching preview ..."));

    int size = 0;

    iniDetails.beginGroup(ui.listThemes->item(row)->data(Qt::UserRole).toString());
    size += iniDetails.value("size").toInt();
    qDebug() << ui.listThemes->item(row)->data(Qt::UserRole).toString() << size;
    iniDetails.endGroup();
    ui.labelSize->setText(tr("Download size %L1 kiB").arg(size));

    iniDetails.beginGroup(ui.listThemes->item(row)->data(Qt::UserRole).toString());

    QUrl img, txt;
    txt = QUrl(QString(settings->themeUrl() + "/"
        + iniDetails.value("descriptionfile").toString()));
    img = QUrl(QString(settings->themeUrl() + "/"
        + iniDetails.value("image").toString()));
    qDebug() << "txt:" << txt;
    qDebug() << "img:" << img;

    QString text;
    text = tr("<b>Author:</b> %1<hr/>").arg(iniDetails.value("author", tr("unknown")).toString());
    text += tr("<b>Version:</b> %1<hr/>").arg(iniDetails.value("version", tr("unknown")).toString());
    text += tr("<b>Description:</b> %1<hr/>").arg(iniDetails.value("about", tr("no description")).toString());

    ui.themeDescription->setHtml(text);
    iniDetails.endGroup();

    igetter.abort();
    if(!settings->cacheDisabled())
        igetter.setCache(true);
    else
    {
        if(infocachedir=="")
        {
            infocachedir = QDir::tempPath()+"rbutil-themeinfo";
            QDir d = QDir::temp();
            d.mkdir("rbutil-themeinfo");
        }
        igetter.setCache(infocachedir);
    }
    igetter.getFile(img);
    connect(&igetter, SIGNAL(done(bool)), this, SLOT(updateImage(bool)));
}


void ThemesInstallWindow::updateImage(bool error)
{
    qDebug() << "updateImage(bool) =" << error;
    if(error) return;

    QPixmap p;
    if(!error) {
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
    qDebug() << "resizeEvent(QResizeEvent*) =" << e;

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
    logger = new ProgressLoggerGui(this);
    logger->show();
    logger->addItem(tr("getting themes information ..."), LOGINFO);
    downloadInfo();
    connect(logger, SIGNAL(aborted()), getter, SLOT(abort()));
    connect(logger, SIGNAL(aborted()), this, SLOT(close()));
}


void ThemesInstallWindow::abort()
{
    igetter.abort();
    logger->abort();
    this->close();
}


void ThemesInstallWindow::acceptAll()
{
    ui.listThemes->selectAll();
    accept();
}

void ThemesInstallWindow::accept()
{
    if(ui.listThemes->selectedItems().size() == 0) {
        this->close();
        return;
    }
    QStringList themes;
    QStringList names;
    QStringList version;
    QString zip;
    QSettings iniDetails(themesInfo.fileName(), QSettings::IniFormat, this);
    for(int i = 0; i < ui.listThemes->selectedItems().size(); i++) {
        iniDetails.beginGroup(ui.listThemes->selectedItems().at(i)->data(Qt::UserRole).toString());
        zip = settings->themeUrl()
                + "/" + iniDetails.value("archive").toString();
        themes.append(zip);
        names.append("Theme: " +
                ui.listThemes->selectedItems().at(i)->data(Qt::DisplayRole).toString());
        // if no version info is available use installation (current) date
        version.append(iniDetails.value("version",
                QDate().currentDate().toString("yyyyMMdd")).toString());
        iniDetails.endGroup();
    }
    qDebug() << "installing themes:" << themes;

    logger = new ProgressLoggerGui(this);
    logger->show();
    QString mountPoint = settings->mountpoint();
    qDebug() << "mountpoint:" << mountPoint;
    // show dialog with error if mount point is wrong
    if(!QFileInfo(mountPoint).isDir()) {
        logger->addItem(tr("Mount point is wrong!"),LOGERROR);
        logger->abort();
        return;
    }

    installer = new ZipInstaller(this);
    installer->setUrl(themes);
    installer->setLogSection(names);
    installer->setLogVersion(version);
    installer->setMountPoint(mountPoint);
    if(!settings->cacheDisabled())
        installer->setCache(true);
    installer->install(logger);
    connect(logger, SIGNAL(closed()), this, SLOT(close()));
}

