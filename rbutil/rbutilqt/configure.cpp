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
#include "ui_configurefrm.h"

#ifdef __linux
#include <stdio.h>
#endif

#define DEFAULT_LANG "English (builtin)"

Config::Config(QWidget *parent) : QDialog(parent)
{
    programPath = QFileInfo(qApp->arguments().at(0)).absolutePath() + "/";
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

    this->setModal(true);
    
    connect(ui.buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui.buttonCancel, SIGNAL(clicked()), this, SLOT(abort()));
    connect(ui.radioNoProxy, SIGNAL(toggled(bool)), this, SLOT(setNoProxy(bool)));
    connect(ui.radioSystemProxy, SIGNAL(toggled(bool)), this, SLOT(setSystemProxy(bool)));

    // disable unimplemented stuff
    ui.buttonCacheBrowse->setEnabled(false);
    ui.cacheDisable->setEnabled(false);
    ui.cacheOfflineMode->setEnabled(false);
    ui.buttonCacheClear->setEnabled(false);
    ui.scrobblerUser->setEnabled(false);
    ui.scrobblerPass->setEnabled(false);
    ui.scrobblerTimezone->setEnabled(false);
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
    QUrl proxy = userSettings->value("defaults/proxy").toString();

    ui.proxyPort->setText(QString("%1").arg(proxy.port()));
    ui.proxyHost->setText(proxy.host());
    ui.proxyUser->setText(proxy.userName());
    ui.proxyPass->setText(proxy.password());

    QString proxyType = userSettings->value("defaults/proxytype").toString();
    if(proxyType == "manual") ui.radioManualProxy->setChecked(true);
    else if(proxyType == "system") ui.radioSystemProxy->setChecked(true);
    else if(proxyType == "none") ui.radioNoProxy->setChecked(true);

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
        ui.proxyPort->setText(QString("%1").arg(proxy.port()));
        ui.proxyUser->setText(proxy.userName());
        ui.proxyPass->setText(proxy.password());
    }

}


QStringList Config::findLanguageFiles()
{
    QDir dir(programPath + "/");
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


