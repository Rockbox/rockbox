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

#include <stdio.h>
#if defined(Q_OS_WIN32)
#if defined(UNICODE)
#define _UNICODE
#endif
#include <tchar.h>
#include <windows.h>
#endif

#define DEFAULT_LANG "English (C)"

Config::Config(QWidget *parent) : QDialog(parent)
{
    programPath = qApp->applicationDirPath() + "/";
    ui.setupUi(this);
    ui.tabConfiguration->setCurrentIndex(0);
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
    connect(ui.browseTts, SIGNAL(clicked()), this, SLOT(browseTts()));
    connect(ui.configEncoder, SIGNAL(clicked()), this, SLOT(configEnc()));
    connect(ui.comboTts, SIGNAL(currentIndexChanged(int)), this, SLOT(updateTtsOpts(int)));
    connect(ui.comboEncoder, SIGNAL(currentIndexChanged(int)), this, SLOT(updateEncState(int)));
    
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
    userSettings->setValue("proxy", proxy.toString());
    qDebug() << "new proxy:" << proxy;
    // proxy type
    QString proxyType;
    if(ui.radioNoProxy->isChecked()) proxyType = "none";
    else if(ui.radioSystemProxy->isChecked()) proxyType = "system";
    else proxyType = "manual";
    userSettings->setValue("proxytype", proxyType);

    // language
    if(userSettings->value("lang").toString() != language)
        QMessageBox::information(this, tr("Language changed"),
            tr("You need to restart the application for the changed language to take effect."));
    userSettings->setValue("lang", language);

    // mountpoint
    QString mp = ui.mountPoint->text();
    if(QFileInfo(mp).isDir())
        userSettings->setValue("mountpoint", mp);

    // platform
    QString nplat;
    if(ui.treeDevices->selectedItems().size() != 0) {
        nplat = ui.treeDevices->selectedItems().at(0)->data(0, Qt::UserRole).toString();
        userSettings->setValue("platform", nplat);
    }

    // cache settings
    if(QFileInfo(ui.cachePath->text()).isDir())
        userSettings->setValue("cachepath", ui.cachePath->text());
    else // default to system temp path
        userSettings->setValue("cachepath", QDir::tempPath());
    userSettings->setValue("cachedisable", ui.cacheDisable->isChecked());
    userSettings->setValue("offline", ui.cacheOfflineMode->isChecked());

    // tts settings
    QString preset;
    preset = ui.comboTts->itemData(ui.comboTts->currentIndex(), Qt::UserRole).toString();
    userSettings->setValue("ttspreset", preset);
    userSettings->beginGroup(preset);
    
    if(QFileInfo(ui.ttsExecutable->text()).exists())
        userSettings->setValue("binary", ui.ttsExecutable->text());
    userSettings->setValue("options", ui.ttsOptions->text());
    userSettings->setValue("language", ui.ttsLanguage->text());
    devices->beginGroup(preset);
    userSettings->setValue("template", devices->value("template").toString());
    userSettings->setValue("type", devices->value("tts").toString());
    devices->endGroup();
    userSettings->endGroup();
    
    //encoder settings 
    userSettings->setValue("encoder",ui.comboEncoder->currentText());

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
    proxy = userSettings->value("proxy").toString();

    if(proxy.port() > 0)
        ui.proxyPort->setText(QString("%1").arg(proxy.port()));
    else ui.proxyPort->setText("");
    ui.proxyHost->setText(proxy.host());
    ui.proxyUser->setText(proxy.userName());
    ui.proxyPass->setText(proxy.password());

    QString proxyType = userSettings->value("proxytype", "system").toString();
    if(proxyType == "manual") ui.radioManualProxy->setChecked(true);
    else if(proxyType == "system") ui.radioSystemProxy->setChecked(true);
    else ui.radioNoProxy->setChecked(true);

    // set language selection
    QList<QListWidgetItem*> a;
    QString b;
    // find key for lang value
    QMap<QString, QString>::const_iterator i = lang.constBegin();
    while (i != lang.constEnd()) {
        if(i.value() == userSettings->value("lang").toString()) {
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
    ui.mountPoint->setText(userSettings->value("mountpoint").toString());

    // cache tab
    if(!QFileInfo(userSettings->value("cachepath").toString()).isDir())
        userSettings->setValue("cachepath", QDir::tempPath());
    ui.cachePath->setText(userSettings->value("cachepath").toString());
    ui.cacheDisable->setChecked(userSettings->value("cachedisable", true).toBool());
    ui.cacheOfflineMode->setChecked(userSettings->value("offline").toBool());
    updateCacheInfo(userSettings->value("cachepath").toString());
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
    platform = devcs.value(userSettings->value("platform").toString());

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
        for(int it = 0; it < a.size(); it++) {
            QString curdev;
            devices->beginGroup("platforms");
            curdev = devices->value(a.at(it), "null").toString();
            devices->endGroup();
            QString curname;
            devices->beginGroup(curdev);
            curname = devices->value("name", "null").toString();
            QString curbrand = devices->value("brand", "").toString();
            QString curicon = devices->value("icon", "").toString();
            devices->endGroup();
            if(curbrand != brands.at(c)) continue;
            qDebug() << "adding:" << brands.at(c) << curname << curdev;
            w2 = new QTreeWidgetItem(w, QStringList(curname));
            w2->setData(0, Qt::UserRole, curdev);
//            QIcon icon;
//            icon.addFile(":/icons/devices/" + curicon + "-tiny.png");
//            w2->setIcon(0, icon);
//            ui.treeDevices->setIconSize(QSize(32, 32));
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
    ui.comboEncoder->addItems(getEncoderList());
    
    //update index of combobox
    int index = ui.comboEncoder->findText(userSettings->value("encoder").toString(),Qt::MatchExactly);
    if(index < 0) index = 0;
    ui.comboEncoder->setCurrentIndex(index);
    updateEncState(index);
    
    //tts
    devices->beginGroup("tts");
    QStringList keys = devices->allKeys();
    for(int i=0; i < keys.size();i++)
    {
        devices->endGroup();
        devices->beginGroup(keys.at(i));
        QString os = devices->value("os").toString();
        devices->endGroup();
        devices->beginGroup("tts");
        
        if(os == "all")
            ui.comboTts->addItem(devices->value(keys.at(i), "null").toString(), keys.at(i));
            
#if defined(Q_OS_WIN32)
        if(os == "win32")
            ui.comboTts->addItem(devices->value(keys.at(i), "null").toString(), keys.at(i));
#endif
    }
    devices->endGroup();


    index = ui.comboTts->findData(userSettings->value("ttspreset").toString(),
                            Qt::UserRole, Qt::MatchExactly);
    if(index < 0) index = 0;
    ui.comboTts->setCurrentIndex(index);
    updateTtsOpts(index);
    

}


void Config::updateTtsOpts(int index)
{
    bool edit;
    QString e;
    bool needsLanguageCfg;
    QString c = ui.comboTts->itemData(index, Qt::UserRole).toString();
    devices->beginGroup(c);
    edit = devices->value("edit").toBool();
    needsLanguageCfg = devices->value("needslanguagecfg").toBool();
    ui.ttsLanguage->setVisible(needsLanguageCfg);
    ui.ttsLanguageLabel->setVisible(needsLanguageCfg);
    ui.ttsOptions->setText(devices->value("options").toString());
    ui.ttsOptions->setEnabled(devices->value("edit").toBool());
    e = devices->value("tts").toString();
    devices->endGroup();
       
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
    QStringList path = QString(getenv("PATH")).split(":", QString::SkipEmptyParts);
#elif defined(Q_OS_WIN)
    QStringList path = QString(getenv("PATH")).split(";", QString::SkipEmptyParts);
#endif
    qDebug() << path;
    ui.ttsExecutable->setEnabled(true);
    for(int i = 0; i < path.size(); i++) {
        QString executable = QDir::fromNativeSeparators(path.at(i)) + "/" + e;
#if defined(Q_OS_WIN)
        executable += ".exe";
        QStringList ex = executable.split("\"", QString::SkipEmptyParts);
        executable = ex.join("");
#endif
        qDebug() << executable;
        if(QFileInfo(executable).isExecutable()) {
            ui.ttsExecutable->setText(QDir::toNativeSeparators(executable));
            // disallow changing the detected path if non-customizable profile
            if(!edit)
                ui.ttsExecutable->setEnabled(false);
            break;
        }
    }
    
    //user settings
    userSettings->beginGroup(c);
    QString temp = userSettings->value("binary","null").toString();
    if(temp != "null")  ui.ttsExecutable->setText(temp);
    temp = userSettings->value("options","null").toString();
    if(temp != "null") ui.ttsOptions->setText(temp);
    temp = userSettings->value("language","null").toString();
    if(temp != "null") ui.ttsLanguage->setText(temp);
    userSettings->endGroup();
}

void Config::updateEncState(int index)
{
    QString encoder = ui.comboEncoder->itemText(index);
    EncBase* enc = getEncoder(encoder);
    enc->setUserCfg(userSettings);
    
    if(enc->configOk())
    {
        ui.configstatus->setText("Configuration OK");
        ui.configstatusimg->setPixmap(QPixmap(QString::fromUtf8(":/icons/icons/go-next.png")));
    }
    else
    {
        ui.configstatus->setText("Configuration INVALID");
        ui.configstatusimg->setPixmap(QPixmap(QString::fromUtf8(":/icons/icons/dialog-error.png")));
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
        QUrl envproxy;
#if defined(Q_OS_LINUX)
        envproxy = QUrl(getenv("http_proxy"));
#endif
#if defined(Q_OS_WIN32)
        HKEY hk;
        wchar_t proxyval[80];
        DWORD buflen = 80;
        long ret;

        ret = RegOpenKeyEx(HKEY_CURRENT_USER, _TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"),
            0, KEY_QUERY_VALUE, &hk);
        if(ret != ERROR_SUCCESS) return;

        ret = RegQueryValueEx(hk, _TEXT("ProxyServer"), NULL, NULL, (LPBYTE)proxyval, &buflen);
        if(ret != ERROR_SUCCESS) return;

        RegCloseKey(hk);
        envproxy = QUrl("http://" + QString::fromWCharArray(proxyval));
        qDebug() << envproxy;
        ui.proxyHost->setText(envproxy.host());
        ui.proxyPort->setText(QString("%1").arg(envproxy.port()));
        ui.proxyUser->setText(envproxy.userName());
        ui.proxyPass->setText(envproxy.password());
#endif
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
    updateCacheInfo(c);
}


void Config::autodetect()
{
    Autodetection detector(this);

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
    updateCacheInfo(userSettings->value("cachepath").toString());
}


void Config::browseTts()
{
    BrowseDirtree browser(this);
    browser.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

    if(QFileInfo(ui.ttsExecutable->text()).isDir())
    {
        browser.setDir(ui.ttsExecutable->text());
    }
    if(browser.exec() == QDialog::Accepted)
    {
        qDebug() << browser.getSelected();
        QString exe = browser.getSelected();
        if(!QFileInfo(exe).exists())
            return;
        ui.ttsExecutable->setText(exe);
    }

}


void Config::configEnc()
{
    EncBase* enc =getEncoder(ui.comboEncoder->currentText());
    
    enc->setUserCfg(userSettings);
    enc->showCfg();
    updateEncState(ui.comboEncoder->currentIndex());
}
