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

#include "configure.h"
#include "autodetection.h"
#include "ui_configurefrm.h"
#include "browsedirtree.h"

#ifdef __linux
#include <stdio.h>
#endif

#define DEFAULT_LANG "English (builtin)"

Config::Config(QWidget *parent) : QDialog(parent)
{
    programPath = qApp->applicationDirPath() + "/";
    ui.setupUi(this);
    ui.radioManualProxy->setChecked(true);
    QRegExpValidator *proxyValidator = new QRegExpValidator(this);
    QRegExp validate("[0-9]*");
    proxyValidator->setRegExp(validate);
    ui.proxyPort->setValidator(proxyValidator);
#ifndef __linux
    ui.radioSystemProxy->setEnabled(false); // only on linux for now
#endif
    // build language list and sort alphabetically
    QStringList langs = findLanguageFiles();
    for(int i = 0; i < langs.size(); ++i)
        lang.insert(languageName(langs[i]), langs[i]);
    lang.insert(DEFAULT_LANG, "");
    QMap<QString, QString>::const_iterator i = lang.constBegin();
    while (i != lang.constEnd()) {
        ui.listLanguages->addItem(i.key());
        i++;
    }
    ui.listLanguages->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(ui.listLanguages, SIGNAL(itemSelectionChanged()), this, SLOT(updateLanguage()));
    ui.proxyPass->setEchoMode(QLineEdit::Password);
    ui.treeDevices->setAlternatingRowColors(true);
    ui.listLanguages->setAlternatingRowColors(true);

    this->setModal(true);
    
    connect(ui.buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui.buttonCancel, SIGNAL(clicked()), this, SLOT(abort()));
    connect(ui.radioNoProxy, SIGNAL(toggled(bool)), this, SLOT(setNoProxy(bool)));
    connect(ui.radioSystemProxy, SIGNAL(toggled(bool)), this, SLOT(setSystemProxy(bool)));
    connect(ui.browseMountPoint, SIGNAL(clicked()), this, SLOT(browseFolder()));
    connect(ui.buttonAutodetect,SIGNAL(clicked()),this,SLOT(autodetect()));
    connect(ui.buttonCacheBrowse, SIGNAL(clicked()), this, SLOT(browseCache()));
    connect(ui.buttonCacheClear, SIGNAL(clicked()), this, SLOT(cacheClear()));

}



void Config::accept()
{
    qDebug() << "Config::accept()";
    // proxy: save entered proxy values, not displayed.
    if(ui.radioManualProxy->isChecked()) {
        proxy.setScheme("http");
        proxy.setUserName(ui.proxyUser->text());
        proxy.setPassword(ui.proxyPass->text());
        proxy.setHost(ui.proxyHost->text());
        proxy.setPort(ui.proxyPort->text().toInt());
    }
    userSettings->setValue("defaults/proxy", proxy.toString());
    qDebug() << "new proxy:" << proxy;
    // proxy type
    QString proxyType;
    if(ui.radioNoProxy->isChecked()) proxyType = "none";
    else if(ui.radioSystemProxy->isChecked()) proxyType = "system";
    else proxyType = "manual";
    userSettings->setValue("defaults/proxytype", proxyType);

    // language
    if(userSettings->value("defaults/lang").toString() != language)
        QMessageBox::information(this, tr("Language changed"),
            tr("You need to restart the application for the changed language to take effect."));
    userSettings->setValue("defaults/lang", language);

    // mountpoint
    QString mp = ui.mountPoint->text();
    if(QFileInfo(mp).isDir())
        userSettings->setValue("defaults/mountpoint", mp);

    // platform
    QString nplat;
    if(ui.treeDevices->selectedItems().size() != 0) {
        nplat = ui.treeDevices->selectedItems().at(0)->data(0, Qt::UserRole).toString();
        userSettings->setValue("defaults/platform", nplat);
    }

    // cache settings
    if(QFileInfo(ui.cachePath->text()).isDir())
        userSettings->setValue("defaults/cachepath", ui.cachePath->text());
    else // default to system temp path
        userSettings->setValue("defaults/cachepath", QDir::tempPath());
    userSettings->setValue("defaults/cachedisable", ui.cacheDisable->isChecked());
    userSettings->setValue("defaults/offline", ui.cacheOfflineMode->isChecked());

    // sync settings
    userSettings->sync();
    this->close();
    emit settingsUpdated();
}


void Config::abort()
{
    qDebug() << "Config::abort()";
    this->close();
}


void Config::setUserSettings(QSettings *user)
{
    userSettings = user;
    // set proxy
    proxy = userSettings->value("defaults/proxy").toString();

    if(proxy.port() > 0)
        ui.proxyPort->setText(QString("%1").arg(proxy.port()));
    else ui.proxyPort->setText("");
    ui.proxyHost->setText(proxy.host());
    ui.proxyUser->setText(proxy.userName());
    ui.proxyPass->setText(proxy.password());

    QString proxyType = userSettings->value("defaults/proxytype").toString();
    if(proxyType == "manual") ui.radioManualProxy->setChecked(true);
    else if(proxyType == "system") ui.radioSystemProxy->setChecked(true);
    else ui.radioNoProxy->setChecked(true);

    // set language selection
    QList<QListWidgetItem*> a;
    QString b;
    // find key for lang value
    QMap<QString, QString>::const_iterator i = lang.constBegin();
    while (i != lang.constEnd()) {
        if(i.value() == userSettings->value("defaults/lang").toString() + ".qm") {
            b = i.key();
            break;
        }
        i++;
    }
    a = ui.listLanguages->findItems(b, Qt::MatchExactly);
    if(a.size() <= 0)
        a = ui.listLanguages->findItems(DEFAULT_LANG, Qt::MatchExactly);
    if(a.size() > 0)
        ui.listLanguages->setCurrentItem(a.at(0));

    // devices tab
    ui.mountPoint->setText(userSettings->value("defaults/mountpoint").toString());

    // cache tab
    if(!QFileInfo(userSettings->value("defaults/cachepath").toString()).isDir())
        userSettings->setValue("defaults/cachepath", QDir::tempPath());
    ui.cachePath->setText(userSettings->value("defaults/cachepath").toString());
    ui.cacheDisable->setChecked(userSettings->value("defaults/cachedisable", true).toBool());
    ui.cacheOfflineMode->setChecked(userSettings->value("defaults/offline").toBool());
    QList<QFileInfo> fs = QDir(userSettings->value("defaults/cachepath").toString() + "/rbutil-cache/").entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    qint64 sz = 0;
    for(int i = 0; i < fs.size(); i++) {
        sz += fs.at(i).size();
        qDebug() << fs.at(i).fileName() << fs.at(i).size();
    }
    ui.cacheSize->setText(tr("Current cache size is %1 kiB.")
            .arg(sz/1024));
}


void Config::setDevices(QSettings *dev)
{
    devices = dev;
    // setup devices table
    qDebug() << "Config::setDevices()";
    devices->beginGroup("platforms");
    QStringList a = devices->childKeys();
    devices->endGroup();
    
    QMap <QString, QString> manuf;
    QMap <QString, QString> devcs;
    for(int it = 0; it < a.size(); it++) {
        QString curdev;
        devices->beginGroup("platforms");
        curdev = devices->value(a.at(it), "null").toString();
        devices->endGroup();
        QString curname;
        devices->beginGroup(curdev);
        curname = devices->value("name", "null").toString();
        QString curbrand = devices->value("brand", "").toString();
        devices->endGroup();
        manuf.insertMulti(curbrand, curdev);
        devcs.insert(curdev, curname);
    }

    QString platform;
    platform = devcs.value(userSettings->value("defaults/platform").toString());

    // set up devices table
    ui.treeDevices->header()->hide();
    ui.treeDevices->expandAll();
    ui.treeDevices->setColumnCount(1);
    QList<QTreeWidgetItem *> items;

    // get manufacturers
    QStringList brands = manuf.uniqueKeys();
    QTreeWidgetItem *w;
    QTreeWidgetItem *w2;
    QTreeWidgetItem *w3 = 0;
    for(int c = 0; c < brands.size(); c++) {
        qDebug() << brands.at(c);
        w = new QTreeWidgetItem();
        w->setFlags(Qt::ItemIsEnabled);
        w->setText(0, brands.at(c));
//        w->setData(0, Qt::DecorationRole, <icon>);
        items.append(w);
        
        // go through platforms again for sake of order
        for(int it = 0; it < a.size(); it++) {
            QString curdev;
            devices->beginGroup("platforms");
            curdev = devices->value(a.at(it), "null").toString();
            devices->endGroup();
            QString curname;
            devices->beginGroup(curdev);
            curname = devices->value("name", "null").toString();
            QString curbrand = devices->value("brand", "").toString();
            devices->endGroup();
            if(curbrand != brands.at(c)) continue;
            qDebug() << "adding:" << brands.at(c) << curname << curdev;
            w2 = new QTreeWidgetItem(w, QStringList(curname));
            w2->setData(0, Qt::UserRole, curdev);
            if(platform.contains(curname)) {
                w2->setSelected(true);
                w->setExpanded(true);
                w3 = w2; // save pointer to hilight old selection
            }
            items.append(w2);
        }
    }
    ui.treeDevices->insertTopLevelItems(0, items);
    if(w3 != 0)
        ui.treeDevices->setCurrentItem(w3); // hilight old selection
}


void Config::setNoProxy(bool checked)
{
    bool i = !checked;
    ui.proxyPort->setEnabled(i);
    ui.proxyHost->setEnabled(i);
    ui.proxyUser->setEnabled(i);
    ui.proxyPass->setEnabled(i);
}


void Config::setSystemProxy(bool checked)
{
    bool i = !checked;
    ui.proxyPort->setEnabled(i);
    ui.proxyHost->setEnabled(i);
    ui.proxyUser->setEnabled(i);
    ui.proxyPass->setEnabled(i);
    if(checked) {
        // save values in input box
        proxy.setScheme("http");
        proxy.setUserName(ui.proxyUser->text());
        proxy.setPassword(ui.proxyPass->text());
        proxy.setHost(ui.proxyHost->text());
        proxy.setPort(ui.proxyPort->text().toInt());
        // show system values in input box
#ifdef __linux
        QUrl envproxy = QUrl(getenv("http_proxy"));
        ui.proxyHost->setText(envproxy.host());
        ui.proxyPort->setText(QString("%1").arg(envproxy.port()));
        ui.proxyUser->setText(envproxy.userName());
        ui.proxyPass->setText(envproxy.password());
#endif
    }
    else {
        ui.proxyHost->setText(proxy.host());
        if(proxy.port() > 0)
            ui.proxyPort->setText(QString("%1").arg(proxy.port()));
        else ui.proxyPort->setText("");
        ui.proxyUser->setText(proxy.userName());
        ui.proxyPass->setText(proxy.password());
    }

}


QStringList Config::findLanguageFiles()
{
    QDir dir(programPath);
    QStringList fileNames;
    fileNames = dir.entryList(QStringList("*.qm"), QDir::Files, QDir::Name);

    QDir resDir(":/lang");
    fileNames += resDir.entryList(QStringList("*.qm"), QDir::Files, QDir::Name);

    fileNames.sort();
    qDebug() << "Config::findLanguageFiles()" << fileNames;

    return fileNames;
}


QString Config::languageName(const QString &qmFile)
{
    QTranslator translator;

    if(!translator.load(qmFile, programPath))
        translator.load(qmFile, ":/lang");

    return translator.translate("Configure", "English");
}


void Config::updateLanguage()
{
    qDebug() << "updateLanguage()";
    QList<QListWidgetItem*> a = ui.listLanguages->selectedItems();
    if(a.size() > 0)
        language = QFileInfo(lang.value(a.at(0)->text())).baseName();
}


void Config::browseFolder()
{
    browser = new BrowseDirtree(this);
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
    browser->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
#elif defined(Q_OS_WIN32)
    browser->setFilter(QDir::Drives);
#endif
    QDir d(ui.mountPoint->text());
    browser->setDir(d);
    browser->show();
    connect(browser, SIGNAL(itemChanged(QString)), this, SLOT(setMountpoint(QString)));
}


void Config::browseCache()
{
    cbrowser = new BrowseDirtree(this);
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
    cbrowser->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
#elif defined(Q_OS_WIN32)
    cbrowser->setFilter(QDir::Drives);
#endif
    QDir d(ui.cachePath->text());
    cbrowser->setDir(d);
    cbrowser->show();
    connect(cbrowser, SIGNAL(itemChanged(QString)), this, SLOT(setCache(QString)));
}

void Config::setMountpoint(QString m)
{
    ui.mountPoint->setText(m);
}


void Config::setCache(QString c)
{
    ui.cachePath->setText(c);
}


void Config::autodetect()
{
    Autodetection detector(this);

    if(detector.detect())  //let it detect
    {
        QString devicename = detector.getDevice();
        //deexpand the platform
        ui.treeDevices->selectedItems().at(0)->parent()->setExpanded(false);
        //deselect the selected item
        ui.treeDevices->selectedItems().at(0)->setSelected(false);

        // find the new item
        //enumerate al plattform items
        QList<QTreeWidgetItem*> itmList= ui.treeDevices->findItems("*",Qt::MatchWildcard);
        for(int i=0; i< itmList.size();i++)
        {
            //enumerate device items
            for(int j=0;j < itmList.at(i)->childCount();j++)
            {
                QString data = itmList.at(i)->child(j)->data(0, Qt::UserRole).toString();
                
                if( devicename.contains(data)) //item found
                {
                    itmList.at(i)->child(j)->setSelected(true); //select the item
                    itmList.at(i)->setExpanded(true); //expand the platform item
                    break;
                }
            }
        }

        if(detector.getMountPoint() != "" )
        {
            ui.mountPoint->setText(detector.getMountPoint());
        }
        else
        {
            QMessageBox::warning(this, tr("Autodetection"),
                    tr("Could not detect a Mountpoint.\n"
                    "Select your Mountpoint manually."),
                    QMessageBox::Ok ,QMessageBox::Ok);
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Autodetection"),
                tr("Could not detect a device.\n"
                   "Select your device and Mountpoint manually."),
                   QMessageBox::Ok ,QMessageBox::Ok);
        
    }
}

void Config::cacheClear()
{
    if(QMessageBox::critical(this, tr("Really delete cache?"),
       tr("Do you really want to delete the cache? "
         "Make absolutely sure this setting is correct as it will "
         "remove <b>all</b> files in this folder!").arg(ui.cachePath->text()),
       QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    
    QString cache = ui.cachePath->text() + "/rbutil-cache/";
    if(!QFileInfo(cache).isDir()) {
        QMessageBox::critical(this, tr("Path wrong!"),
            tr("The cache path is invalid. Aborting."), QMessageBox::Ok);
        return;
    }
    QDir dir(cache);
    QStringList fn;
    fn = dir.entryList(QStringList("*"), QDir::Files, QDir::Name);
    qDebug() << fn;

    for(int i = 0; i < fn.size(); i++) {
        QString f = cache + fn.at(i);
        QFile::remove(f);
        qDebug() << "removed:" << f;
    }
}
