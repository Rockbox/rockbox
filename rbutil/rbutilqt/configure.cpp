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
#include "encoders.h"
#include "tts.h"
#include "utils.h"

#include <stdio.h>
#if defined(Q_OS_WIN32)
#if defined(UNICODE)
#define _UNICODE
#endif
#include <tchar.h>
#include <windows.h>
#endif

#define DEFAULT_LANG "English (C)"

Config::Config(QWidget *parent,int index) : QDialog(parent)
{
    programPath = qApp->applicationDirPath() + "/";
    ui.setupUi(this);
    ui.tabConfiguration->setCurrentIndex(index);
    ui.radioManualProxy->setChecked(true);
    QRegExpValidator *proxyValidator = new QRegExpValidator(this);
    QRegExp validate("[0-9]*");
    proxyValidator->setRegExp(validate);
    ui.proxyPort->setValidator(proxyValidator);
#if !defined(Q_OS_LINUX) && !defined(Q_OS_WIN32)
    ui.radioSystemProxy->setEnabled(false); // not on macox for now
#endif
    // build language list and sort alphabetically
    QStringList langs = findLanguageFiles();
    for(int i = 0; i < langs.size(); ++i)
        lang.insert(languageName(langs.at(i)) + tr(" (%1)").arg(langs.at(i)), langs.at(i));
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
    connect(ui.configTts, SIGNAL(clicked()), this, SLOT(configTts()));
    connect(ui.configEncoder, SIGNAL(clicked()), this, SLOT(configEnc()));
    connect(ui.comboTts, SIGNAL(currentIndexChanged(int)), this, SLOT(updateTtsState(int)));
     
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
    
    settings->setProxy(proxy.toString());
    qDebug() << "new proxy:" << proxy;
    // proxy type
    QString proxyType;
    if(ui.radioNoProxy->isChecked()) proxyType = "none";
    else if(ui.radioSystemProxy->isChecked()) proxyType = "system";
    else proxyType = "manual";
    settings->setProxyType(proxyType);

    // language
    if(settings->curLang() != language)
        QMessageBox::information(this, tr("Language changed"),
            tr("You need to restart the application for the changed language to take effect."));
    settings->setLang(language);

    // mountpoint
    QString mp = ui.mountPoint->text();
    if(QFileInfo(mp).isDir())
        settings->setMountpoint(QDir::fromNativeSeparators(mp));

    // platform
    QString nplat;
    if(ui.treeDevices->selectedItems().size() != 0) {
        nplat = ui.treeDevices->selectedItems().at(0)->data(0, Qt::UserRole).toString();
        settings->setCurPlatform(nplat);
    }

    // cache settings
    if(QFileInfo(ui.cachePath->text()).isDir())
        settings->setCachePath(ui.cachePath->text());
    else // default to system temp path
        settings->setCachePath( QDir::tempPath());
    settings->setCacheDisable(ui.cacheDisable->isChecked());
    settings->setCacheOffline(ui.cacheOfflineMode->isChecked());

    // tts settings
    int i = ui.comboTts->currentIndex();
    settings->setCurTTS(ui.comboTts->itemData(i).toString());
   
    // sync settings
    settings->sync();
    this->close();
    emit settingsUpdated();
}


void Config::abort()
{
    qDebug() << "Config::abort()";
    this->close();
}

void Config::setSettings(RbSettings* sett)
{
    settings = sett;
      
    setUserSettings();
    setDevices();
}

void Config::setUserSettings()
{
    // set proxy
    proxy = settings->proxy();

    if(proxy.port() > 0)
        ui.proxyPort->setText(QString("%1").arg(proxy.port()));
    else ui.proxyPort->setText("");
    ui.proxyHost->setText(proxy.host());
    ui.proxyUser->setText(proxy.userName());
    ui.proxyPass->setText(proxy.password());

    QString proxyType = settings->proxyType();
    if(proxyType == "manual") ui.radioManualProxy->setChecked(true);
    else if(proxyType == "system") ui.radioSystemProxy->setChecked(true);
    else ui.radioNoProxy->setChecked(true);

    // set language selection
    QList<QListWidgetItem*> a;
    QString b;
    // find key for lang value
    QMap<QString, QString>::const_iterator i = lang.constBegin();
    while (i != lang.constEnd()) {
        if(i.value() == settings->curLang()) {
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
    ui.mountPoint->setText(QDir::toNativeSeparators(settings->mountpoint()));

    // cache tab
    if(!QFileInfo(settings->cachePath()).isDir())
        settings->setCachePath(QDir::tempPath());
    ui.cachePath->setText(settings->cachePath());
    ui.cacheDisable->setChecked(settings->cacheDisabled());
    ui.cacheOfflineMode->setChecked(settings->cacheOffline());
    updateCacheInfo(settings->cachePath());
}


void Config::updateCacheInfo(QString path)
{
    QList<QFileInfo> fs;
    fs = QDir(path + "/rbutil-cache/").entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    qint64 sz = 0;
    for(int i = 0; i < fs.size(); i++) {
        sz += fs.at(i).size();
        qDebug() << fs.at(i).fileName() << fs.at(i).size();
    }
    ui.cacheSize->setText(tr("Current cache size is %L1 kiB.")
            .arg(sz/1024));
}


void Config::setDevices()
{
    
    // setup devices table
    qDebug() << "Config::setDevices()";
    
    QStringList platformList = settings->allPlatforms();

    QMap <QString, QString> manuf;
    QMap <QString, QString> devcs;
    for(int it = 0; it < platformList.size(); it++) 
    {
        QString curname = settings->name(platformList.at(it));
        QString curbrand = settings->brand(platformList.at(it));
        manuf.insertMulti(curbrand, platformList.at(it));
        devcs.insert(platformList.at(it), curname);
    }

    QString platform;
    platform = devcs.value(settings->curPlatform());

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
        items.append(w);

        // go through platforms again for sake of order
        for(int it = 0; it < platformList.size(); it++) {
           
            QString curname = settings->name(platformList.at(it));
            QString curbrand = settings->brand(platformList.at(it));
       
            if(curbrand != brands.at(c)) continue;
            qDebug() << "adding:" << brands.at(c) << curname;
            w2 = new QTreeWidgetItem(w, QStringList(curname));
            w2->setData(0, Qt::UserRole, platformList.at(it));

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

    // tts / encoder tab
    
    //encoders
    updateEncState();

    //tts
    QStringList ttslist = TTSBase::getTTSList();
    for(int a = 0; a < ttslist.size(); a++)
        ui.comboTts->addItem(TTSBase::getTTSName(ttslist.at(a)), ttslist.at(a));
    //update index of combobox
    int index = ui.comboTts->findData(settings->curTTS());
    if(index < 0) index = 0;
    ui.comboTts->setCurrentIndex(index);
    updateTtsState(index);
    
}


void Config::updateTtsState(int index)
{
    QString ttsName = ui.comboTts->itemData(index).toString();
    TTSBase* tts = TTSBase::getTTS(ttsName);
    tts->setCfg(settings);
    
    if(tts->configOk())
    {
        ui.configTTSstatus->setText("Configuration OK");
        ui.configTTSstatusimg->setPixmap(QPixmap(QString::fromUtf8(":/icons/go-next.png")));
    }
    else
    {
        ui.configTTSstatus->setText("Configuration INVALID");
        ui.configTTSstatusimg->setPixmap(QPixmap(QString::fromUtf8(":/icons/dialog-error.png")));
    }
}

void Config::updateEncState()
{
    ui.encoderName->setText(EncBase::getEncoderName(settings->curEncoder()));
    QString encoder = settings->curEncoder();
    EncBase* enc = EncBase::getEncoder(encoder);
    enc->setCfg(settings);
    
    if(enc->configOk())
    {
        ui.configEncstatus->setText("Configuration OK");
        ui.configEncstatusimg->setPixmap(QPixmap(QString::fromUtf8(":/icons/go-next.png")));
    }
    else
    {
        ui.configEncstatus->setText("Configuration INVALID");
        ui.configEncstatusimg->setPixmap(QPixmap(QString::fromUtf8(":/icons/dialog-error.png")));
    }        
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
        QUrl envproxy = systemProxy();

        ui.proxyHost->setText(envproxy.host());

        ui.proxyPort->setText(QString("%1").arg(envproxy.port()));
        ui.proxyUser->setText(envproxy.userName());
        ui.proxyPass->setText(envproxy.password());

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
    QStringList langs;
    fileNames = dir.entryList(QStringList("*.qm"), QDir::Files, QDir::Name);

    QDir resDir(":/lang");
    fileNames += resDir.entryList(QStringList("*.qm"), QDir::Files, QDir::Name);

    QRegExp exp("^rbutil_(.*)\\.qm");
    for(int i = 0; i < fileNames.size(); i++) {
        QString a = fileNames.at(i);
        a.replace(exp, "\\1");
        langs.append(a);
    }
    langs.sort();
    qDebug() << "Config::findLanguageFiles()" << langs;

    return langs;
}


QString Config::languageName(const QString &qmFile)
{
    QTranslator translator;

    QString file = "rbutil_" + qmFile;
    if(!translator.load(file, programPath))
        translator.load(file, ":/lang");

    return translator.translate("Configure", "English");
}


void Config::updateLanguage()
{
    qDebug() << "updateLanguage()";
    QList<QListWidgetItem*> a = ui.listLanguages->selectedItems();
    if(a.size() > 0)
        language = lang.value(a.at(0)->text());
    qDebug() << language;
}


void Config::browseFolder()
{
    browser = new BrowseDirtree(this,tr("Select your device"));
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
    browser->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
#elif defined(Q_OS_WIN32)
    browser->setFilter(QDir::Drives);
#endif
#if defined(Q_OS_MACX)
    browser->setRoot("/Volumes");
#elif defined(Q_OS_LINUX)
    browser->setDir("/media");
#endif
    if( ui.mountPoint->text() != "" )
    {
        browser->setDir(ui.mountPoint->text());
    }
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
    cbrowser->setDir(ui.cachePath->text());
    connect(cbrowser, SIGNAL(itemChanged(QString)), this, SLOT(setCache(QString)));
    cbrowser->show();
    
}

void Config::setMountpoint(QString m)
{
    ui.mountPoint->setText(m);
}


void Config::setCache(QString c)
{
    ui.cachePath->setText(c);
    updateCacheInfo(c);
}


void Config::autodetect()
{
    Autodetection detector(this);
    detector.setSettings(settings);
    // disable tree during detection as "working" feedback.
    // TODO: replace the tree view with a splash screen during this time.
    ui.treeDevices->setEnabled(false);
    QCoreApplication::processEvents();

    if(detector.detect())  //let it detect
    {
        QString devicename = detector.getDevice();
        // deexpand all items
        for(int a = 0; a < ui.treeDevices->topLevelItemCount(); a++)
            ui.treeDevices->topLevelItem(a)->setExpanded(false);
        //deselect the selected item(s)
        for(int a = 0; a < ui.treeDevices->selectedItems().size(); a++)
            ui.treeDevices->selectedItems().at(a)->setSelected(false);

        // find the new item
        // enumerate all platform items
        QList<QTreeWidgetItem*> itmList= ui.treeDevices->findItems("*",Qt::MatchWildcard);
        for(int i=0; i< itmList.size();i++)
        {
            //enumerate device items
            for(int j=0;j < itmList.at(i)->childCount();j++)
            {
                QString data = itmList.at(i)->child(j)->data(0, Qt::UserRole).toString();

                if(devicename == data) // item found
                {
                    itmList.at(i)->child(j)->setSelected(true); //select the item
                    itmList.at(i)->setExpanded(true); //expand the platform item
                    //ui.treeDevices->indexOfTopLevelItem(itmList.at(i)->child(j));
                    break;
                }
            }
        }

        if(!detector.errdev().isEmpty()) {
            QString text;
            if(detector.errdev() == "sansae200")
                text = tr("Sansa e200 in MTP mode found!\n"
                        "You need to change your player to MSC mode for installation. ");
            if(detector.errdev() == "h10")
                text = tr("H10 20GB in MTP mode found!\n"
                        "You need to change your player to UMS mode for installation. ");
            text += tr("Unless you changed this installation will fail!");

            QMessageBox::critical(this, tr("Fatal error"), text, QMessageBox::Ok);
            return;
        }
        if(!detector.incompatdev().isEmpty()) {
            QString text;
            // we need to set the platform here to get the brand from the
            // settings object
            settings->setCurPlatform(detector.incompatdev());
            text = tr("Detected an unsupported %1 player variant. Sorry, "
                      "Rockbox doesn't run on your player.").arg(settings->curBrand());

            QMessageBox::critical(this, tr("Fatal error: incompatible player found"),
                                  text, QMessageBox::Ok);
                return;
            }

        if(detector.getMountPoint() != "" )
        {
            ui.mountPoint->setText(QDir::toNativeSeparators(detector.getMountPoint()));
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
    ui.treeDevices->setEnabled(true);
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
    updateCacheInfo(settings->cachePath());
}


void Config::configTts()
{
    int index = ui.comboTts->currentIndex();
    TTSBase* tts = TTSBase::getTTS(ui.comboTts->itemData(index).toString());
    
    tts->setCfg(settings);
    tts->showCfg();
    updateTtsState(ui.comboTts->currentIndex());
}


void Config::configEnc()
{
    EncBase* enc = EncBase::getEncoder(settings->curEncoder());
    
    enc->setCfg(settings);
    enc->showCfg();
    updateEncState();
}
