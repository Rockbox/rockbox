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

#include <QtGui>

#include "version.h"
#include "configure.h"
#include "autodetection.h"
#include "ui_configurefrm.h"
#include "encoderbase.h"
#include "ttsbase.h"
#include "system.h"
#include "encttscfggui.h"
#include "rbsettings.h"
#include "serverinfo.h"
#include "systeminfo.h"
#include "utils.h"
#include "comboboxviewdelegate.h"
#if defined(Q_OS_WIN32)
#if defined(UNICODE)
#define _UNICODE
#endif
#include <tchar.h>
#include <windows.h>
#endif
#include "rbutilqt.h"

#include "systrace.h"

#define DEFAULT_LANG "English (en)"
#define DEFAULT_LANG_CODE "en"

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

    // build language list and sort alphabetically
    QStringList langs = findLanguageFiles();
    for(int i = 0; i < langs.size(); ++i)
        lang.insert(languageName(langs.at(i))
            + QString(" (%1)").arg(langs.at(i)), langs.at(i));
    lang.insert(DEFAULT_LANG, DEFAULT_LANG_CODE);
    QMap<QString, QString>::const_iterator i = lang.constBegin();
    while (i != lang.constEnd()) {
        ui.listLanguages->addItem(i.key());
        i++;
    }

    ComboBoxViewDelegate *delegate = new ComboBoxViewDelegate(this);
    ui.mountPoint->setItemDelegate(delegate);
#if !defined(DBG)
    ui.mountPoint->setEditable(false);
#endif

    ui.listLanguages->setSelectionMode(QAbstractItemView::SingleSelection);
    ui.proxyPass->setEchoMode(QLineEdit::Password);
    ui.treeDevices->setAlternatingRowColors(true);
    ui.listLanguages->setAlternatingRowColors(true);

    /* Explicitly set some widgets to have left-to-right layout */
    ui.treeDevices->setLayoutDirection(Qt::LeftToRight);
    ui.mountPoint->setLayoutDirection(Qt::LeftToRight);
    ui.proxyHost->setLayoutDirection(Qt::LeftToRight);
    ui.proxyPort->setLayoutDirection(Qt::LeftToRight);
    ui.proxyUser->setLayoutDirection(Qt::LeftToRight);
    ui.proxyPass->setLayoutDirection(Qt::LeftToRight);
    ui.listLanguages->setLayoutDirection(Qt::LeftToRight);
    ui.cachePath->setLayoutDirection(Qt::LeftToRight);
    ui.comboTts->setLayoutDirection(Qt::LeftToRight);

    this->setModal(true);

    connect(ui.buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui.buttonCancel, SIGNAL(clicked()), this, SLOT(abort()));
    connect(ui.radioNoProxy, SIGNAL(toggled(bool)), this, SLOT(setNoProxy(bool)));
    connect(ui.radioSystemProxy, SIGNAL(toggled(bool)), this, SLOT(setSystemProxy(bool)));
    connect(ui.refreshMountPoint, SIGNAL(clicked()), this, SLOT(refreshMountpoint()));
    connect(ui.buttonAutodetect,SIGNAL(clicked()),this,SLOT(autodetect()));
    connect(ui.buttonCacheBrowse, SIGNAL(clicked()), this, SLOT(browseCache()));
    connect(ui.buttonCacheClear, SIGNAL(clicked()), this, SLOT(cacheClear()));
    connect(ui.configTts, SIGNAL(clicked()), this, SLOT(configTts()));
    connect(ui.configEncoder, SIGNAL(clicked()), this, SLOT(configEnc()));
    connect(ui.comboTts, SIGNAL(currentIndexChanged(int)), this, SLOT(updateTtsState(int)));
    connect(ui.treeDevices, SIGNAL(itemSelectionChanged()), this, SLOT(updateEncState()));
    connect(ui.testTTS,SIGNAL(clicked()),this,SLOT(testTts()));
    connect(ui.showDisabled, SIGNAL(toggled(bool)), this, SLOT(showDisabled(bool)));
    connect(ui.mountPoint, SIGNAL(editTextChanged(QString)), this, SLOT(updateMountpoint(QString)));
    connect(ui.mountPoint, SIGNAL(currentIndexChanged(int)), this, SLOT(updateMountpoint(int)));
    // delete this dialog after it finished automatically.
    connect(this, SIGNAL(finished(int)), this, SLOT(deleteLater()));

    setUserSettings();
    setDevices();
}


void Config::accept()
{
    qDebug() << "[Config] checking configuration";
    QString errormsg = tr("The following errors occurred:") + "<ul>";
    bool error = false;

    // proxy: save entered proxy values, not displayed.
    if(ui.radioManualProxy->isChecked()) {
        proxy.setScheme("http");
        proxy.setUserName(ui.proxyUser->text());
        proxy.setPassword(ui.proxyPass->text());
        proxy.setHost(ui.proxyHost->text());
        proxy.setPort(ui.proxyPort->text().toInt());
    }

    // QUrl::toEncoded() doesn't encode a colon in the password correctly,
    // which will result in errors during parsing the string.
    // QUrl::toPercentEncoding() does work as expected, so build the string to
    // store in the configuration file manually.
    QString proxystring = "http://"
        + QString(QUrl::toPercentEncoding(proxy.userName())) + ":"
        + QString(QUrl::toPercentEncoding(proxy.password())) + "@"
        + proxy.host() + ":"
        + QString::number(proxy.port());
    RbSettings::setValue(RbSettings::Proxy, proxystring);
    qDebug() << "[Config] setting proxy to:" << proxy;
    // proxy type
    QString proxyType;
    if(ui.radioNoProxy->isChecked()) proxyType = "none";
    else if(ui.radioSystemProxy->isChecked()) proxyType = "system";
    else proxyType = "manual";
    RbSettings::setValue(RbSettings::ProxyType, proxyType);

    RbSettings::setValue(RbSettings::Language, language);

    // make sure mountpoint is read from dropdown box
    if(mountpoint.isEmpty()) {
        updateMountpoint(ui.mountPoint->currentIndex());
    }

    // mountpoint
    if(mountpoint.isEmpty()) {
        errormsg += "<li>" + tr("No mountpoint given") + "</li>";
        error = true;
    }
    else if(!QFileInfo(mountpoint).exists()) {
        errormsg += "<li>" + tr("Mountpoint does not exist") + "</li>";
        error = true;
    }
    else if(!QFileInfo(mountpoint).isDir()) {
        errormsg += "<li>" + tr("Mountpoint is not a directory.") + "</li>";
        error = true;
    }
    else if(!QFileInfo(mountpoint).isWritable()) {
        errormsg += "<li>" + tr("Mountpoint is not writeable") + "</li>";
        error = true;
    }
    else {
        RbSettings::setValue(RbSettings::Mountpoint,
                QDir::fromNativeSeparators(mountpoint));
    }

    // platform
    QString nplat;
    if(ui.treeDevices->selectedItems().size() != 0) {
        nplat = ui.treeDevices->selectedItems().at(0)->data(0, Qt::UserRole).toString();
        RbSettings::setValue(RbSettings::Platform, nplat);
    }
    else {
        errormsg += "<li>" + tr("No player selected") + "</li>";
        error = true;
    }

    // cache settings
    if(QFileInfo(ui.cachePath->text()).isDir()) {
        if(!QFileInfo(ui.cachePath->text()).isWritable()) {
            errormsg += "<li>" + tr("Cache path not writeable. Leave path empty "
                        "to default to systems temporary path.") + "</li>";
            error = true;
        }
        else
            RbSettings::setValue(RbSettings::CachePath, ui.cachePath->text());
    }
    else // default to system temp path
        RbSettings::setValue(RbSettings::CachePath, QDir::tempPath());
    RbSettings::setValue(RbSettings::CacheDisabled, ui.cacheDisable->isChecked());
    RbSettings::setValue(RbSettings::CacheOffline, ui.cacheOfflineMode->isChecked());

    // tts settings
    RbSettings::setValue(RbSettings::UseTtsCorrections, ui.ttsCorrections->isChecked());
    int i = ui.comboTts->currentIndex();
    RbSettings::setValue(RbSettings::Tts, ui.comboTts->itemData(i).toString());

    RbSettings::setValue(RbSettings::RbutilVersion, PUREVERSION);

    errormsg += "</ul>";
    errormsg += tr("You need to fix the above errors before you can continue.");

    if(error) {
        QMessageBox::critical(this, tr("Configuration error"), errormsg);
    }
    else {
        // sync settings
        RbSettings::sync();
        this->close();
        emit settingsUpdated();
    }
}


void Config::abort()
{
    qDebug() << "[Config] aborted.";
    this->close();
}


void Config::setUserSettings()
{
    // set proxy
    proxy.setEncodedUrl(RbSettings::value(RbSettings::Proxy).toByteArray());

    if(proxy.port() > 0)
        ui.proxyPort->setText(QString("%1").arg(proxy.port()));
    else ui.proxyPort->setText("");
    ui.proxyHost->setText(proxy.host());
    ui.proxyUser->setText(proxy.userName());
    ui.proxyPass->setText(proxy.password());

    QString proxyType = RbSettings::value(RbSettings::ProxyType).toString();
    if(proxyType == "manual") ui.radioManualProxy->setChecked(true);
    else if(proxyType == "system") ui.radioSystemProxy->setChecked(true);
    else ui.radioNoProxy->setChecked(true);

    // set language selection
    QList<QListWidgetItem*> a;
    QString b;
    // find key for lang value
    QMap<QString, QString>::const_iterator i = lang.constBegin();
    QString l = RbSettings::value(RbSettings::Language).toString();
    if(l.isEmpty())
        l = QLocale::system().name();
    while (i != lang.constEnd()) {
        if(i.value() == l) {
            b = i.key();
            break;
        }
        else if(l.startsWith(i.value(), Qt::CaseInsensitive)) {
            // check if there is a base language (en -> en_US, etc.)
            b = i.key();
            break;
        }
        i++;
    }
    a = ui.listLanguages->findItems(b, Qt::MatchExactly);
    if(a.size() > 0)
        ui.listLanguages->setCurrentItem(a.at(0));
    // don't connect before language list has been set up to prevent
    // triggering the signal by selecting the saved language.
    connect(ui.listLanguages, SIGNAL(itemSelectionChanged()), this, SLOT(updateLanguage()));

    // devices tab
    refreshMountpoint();
    mountpoint = QDir::toNativeSeparators(RbSettings::value(RbSettings::Mountpoint).toString());
    setMountpoint(mountpoint);

    // cache tab
    if(!QFileInfo(RbSettings::value(RbSettings::CachePath).toString()).isDir())
        RbSettings::setValue(RbSettings::CachePath, QDir::tempPath());
    ui.cachePath->setText(QDir::toNativeSeparators(RbSettings::value(RbSettings::CachePath).toString()));
    ui.cacheDisable->setChecked(RbSettings::value(RbSettings::CacheDisabled).toBool());
    ui.cacheOfflineMode->setChecked(RbSettings::value(RbSettings::CacheOffline).toBool());
    updateCacheInfo(RbSettings::value(RbSettings::CachePath).toString());

    // TTS tab
    ui.ttsCorrections->setChecked(RbSettings::value(RbSettings::UseTtsCorrections).toBool());
}


void Config::updateCacheInfo(QString path)
{
    QList<QFileInfo> fs;
    fs = QDir(path + "/rbutil-cache/").entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    qint64 sz = 0;
    for(int i = 0; i < fs.size(); i++) {
        sz += fs.at(i).size();
    }
    ui.cacheSize->setText(tr("Current cache size is %L1 kiB.")
            .arg(sz/1024));
}


void Config::showDisabled(bool show)
{
    qDebug() << "[Config] disabled targets shown:" << show;
    if(show)
        QMessageBox::warning(this, tr("Showing disabled targets"),
                tr("You just enabled showing targets that are marked disabled. "
                   "Disabled targets are not recommended to end users. Please "
                   "use this option only if you know what you are doing."));
    setDevices();

}


void Config::setDevices()
{

    // setup devices table
    qDebug() << "[Config] setting up devices list";

    QStringList platformList;
    if(ui.showDisabled->isChecked())
        platformList = SystemInfo::platforms(SystemInfo::PlatformAllDisabled);
    else
        platformList = SystemInfo::platforms(SystemInfo::PlatformAll);

    QMap <QString, QString> manuf;
    for(int it = 0; it < platformList.size(); it++)
    {
        QString curbrand = SystemInfo::platformValue(platformList.at(it),
                    SystemInfo::CurBrand).toString();
        manuf.insertMulti(curbrand, platformList.at(it));
    }

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

    QString selected = RbSettings::value(RbSettings::Platform).toString();
    for(int c = 0; c < brands.size(); c++) {
        w = new QTreeWidgetItem();
        w->setFlags(Qt::ItemIsEnabled);
        w->setText(0, brands.at(c));
        items.append(w);
        // go through platforms and add all players matching the current brand
        for(int it = 0; it < platformList.size(); it++) {
            // skip if not current brand
            if(!manuf.values(brands.at(c)).contains(platformList.at(it)))
                continue;
            // construct display name
            QString curname = SystemInfo::platformValue(platformList.at(it),
                                SystemInfo::CurName).toString() +
                " (" +ServerInfo::platformValue(platformList.at(it),
                            ServerInfo::CurStatus).toString() +")";
            qDebug() << "[Config] add supported device:" << brands.at(c) << curname;
            w2 = new QTreeWidgetItem(w, QStringList(curname));
            w2->setData(0, Qt::UserRole, platformList.at(it));

            if(platformList.at(it) == selected) {
                w2->setSelected(true);
                w->setExpanded(true);
                w3 = w2; // save pointer to hilight old selection
            }
            items.append(w2);
        }
    }
    // remove any old items in list
    QTreeWidgetItem* widgetitem;
    do {
        widgetitem = ui.treeDevices->takeTopLevelItem(0);
        delete widgetitem;
    }
    while(widgetitem);
    // add new items
    ui.treeDevices->insertTopLevelItems(0, items);
    if(w3 != 0) {
        ui.treeDevices->setCurrentItem(w3); // hilight old selection
        ui.treeDevices->scrollToItem(w3);
    }

    // tts / encoder tab

    //encoders
    updateEncState();

    //tts
    QStringList ttslist = TTSBase::getTTSList();
    for(int a = 0; a < ttslist.size(); a++)
        ui.comboTts->addItem(TTSBase::getTTSName(ttslist.at(a)), ttslist.at(a));
    //update index of combobox
    int index = ui.comboTts->findData(RbSettings::value(RbSettings::Tts).toString());
    if(index < 0) index = 0;
    ui.comboTts->setCurrentIndex(index);
    updateTtsState(index);

}


void Config::updateTtsState(int index)
{
    QString ttsName = ui.comboTts->itemData(index).toString();
    TTSBase* tts = TTSBase::getTTS(this,ttsName);

    if(tts->configOk())
    {
        ui.configTTSstatus->setText(tr("Configuration OK"));
        ui.configTTSstatusimg->setPixmap(QPixmap(QString::fromUtf8(":/icons/go-next.png")));
        ui.testTTS->setEnabled(true);
    }
    else
    {
        ui.configTTSstatus->setText(tr("Configuration INVALID"));
        ui.configTTSstatusimg->setPixmap(QPixmap(QString::fromUtf8(":/icons/dialog-error.png")));
        ui.testTTS->setEnabled(false);
    }

    delete tts; /* Config objects are never deleted (in fact, they are leaked..), so we can't rely on QObject,
                   since that would delete the TTSBase instance on application exit*/
}

void Config::updateEncState()
{
    if(ui.treeDevices->selectedItems().size() == 0)
        return;

    QString devname = ui.treeDevices->selectedItems().at(0)->data(0, Qt::UserRole).toString();
    QString encoder = SystemInfo::platformValue(devname,
                        SystemInfo::CurEncoder).toString();
    ui.encoderName->setText(EncoderBase::getEncoderName(SystemInfo::platformValue(devname,
                        SystemInfo::CurEncoder).toString()));

    EncoderBase* enc = EncoderBase::getEncoder(this,encoder);

    if(enc->configOk())
    {
        ui.configEncstatus->setText(tr("Configuration OK"));
        ui.configEncstatusimg->setPixmap(QPixmap(QString::fromUtf8(":/icons/go-next.png")));
    }
    else
    {
        ui.configEncstatus->setText(tr("Configuration INVALID"));
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
    ui.proxyPort->setEnabled(!checked);
    ui.proxyHost->setEnabled(!checked);
    ui.proxyUser->setEnabled(!checked);
    ui.proxyPass->setEnabled(!checked);
    if(checked) {
        // save values in input box
        proxy.setScheme("http");
        proxy.setUserName(ui.proxyUser->text());
        proxy.setPassword(ui.proxyPass->text());
        proxy.setHost(ui.proxyHost->text());
        proxy.setPort(ui.proxyPort->text().toInt());
        // show system values in input box
        QUrl envproxy = System::systemProxy();
        qDebug() << "[Config] setting system proxy" << envproxy;

        ui.proxyHost->setText(envproxy.host());
        ui.proxyPort->setText(QString("%1").arg(envproxy.port()));
        ui.proxyUser->setText(envproxy.userName());
        ui.proxyPass->setText(envproxy.password());

        if(envproxy.host().isEmpty() || envproxy.port() == -1) {
            qDebug() << "[Config] sytem proxy is invalid.";
            QMessageBox::warning(this, tr("Proxy Detection"),
                    tr("The System Proxy settings are invalid!\n"
                        "Rockbox Utility can't work with this proxy settings. "
                        "Make sure the system proxy is set correctly. Note that "
                        "\"proxy auto-config (PAC)\" scripts are not supported by "
                        "Rockbox Utility. If your system uses this you need "
                        "to use manual proxy settings."),
                    QMessageBox::Ok ,QMessageBox::Ok);
            // the current proxy settings are invalid. Check the saved proxy
            // type again.
            if(RbSettings::value(RbSettings::ProxyType).toString() == "manual")
                ui.radioManualProxy->setChecked(true);
            else
                ui.radioNoProxy->setChecked(true);
        }

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
    qDebug() << "[Config] available lang files:" << langs;

    return langs;
}


QString Config::languageName(const QString &qmFile)
{
    QTranslator translator;

    QString file = "rbutil_" + qmFile;
    if(!translator.load(file, programPath))
        translator.load(file, ":/lang");

    return translator.translate("Configure", "English",
        "This is the localized language name, i.e. your language.");
}


void Config::updateLanguage()
{
    qDebug() << "[Config] update selected language";

    // remove all old translators
    for(int i = 0; i < RbUtilQt::translators.size(); ++i) {
        qApp->removeTranslator(RbUtilQt::translators.at(i));
        // do not delete old translators, this confuses Qt.
    }
    RbUtilQt::translators.clear();
    QList<QListWidgetItem*> a = ui.listLanguages->selectedItems();
    if(a.size() > 0)
        language = lang.value(a.at(0)->text());
    qDebug() << "[Config] new language:" << language;

    QString applang = QLocale::system().name();
    QTranslator *translator = new QTranslator(qApp);
    QTranslator *qttrans = new QTranslator(qApp);
    QString absolutePath = QCoreApplication::instance()->applicationDirPath();

    if(!translator->load("rbutil_" + language, absolutePath))
        translator->load("rbutil_" + language, ":/lang");
    if(!qttrans->load("qt_" + language,
                QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        qttrans->load("qt_" + language, ":/lang");

    qApp->installTranslator(translator);
    qApp->installTranslator(qttrans);
    //: This string is used to indicate the writing direction. Translate it
    //: to "RTL" (without quotes) for RTL languages. Anything else will get
    //: treated as LTR language.
    if(QObject::tr("LTR") == "RTL")
        qApp->setLayoutDirection(Qt::RightToLeft);
    else
        qApp->setLayoutDirection(Qt::LeftToRight);

    RbUtilQt::translators.append(translator);
    RbUtilQt::translators.append(qttrans);

    QLocale::setDefault(language);

}


void Config::browseCache()
{
    QString old = ui.cachePath->text();
    if(!QFileInfo(old).isDir())
        old = QDir::tempPath();
    QString c = QFileDialog::getExistingDirectory(this, tr("Set Cache Path"), old);
    if(c.isEmpty())
        c = old;
    else if(!QFileInfo(c).isDir())
        c = QDir::tempPath();
    ui.cachePath->setText(QDir::toNativeSeparators(c));
    updateCacheInfo(c);
}


void Config::refreshMountpoint()
{
    // avoid QComboBox to send signals during rebuild to avoid changing to an
    // unwanted item.
    ui.mountPoint->blockSignals(true);
    ui.mountPoint->clear();
    QStringList mps = Utils::mountpoints();
    for(int i = 0; i < mps.size(); ++i) {
        // add mountpoint as user data so we can change the displayed string
        // later (to include volume label or similar)
        // Skip unwritable mountpoints, they are not useable for us.
        if(QFileInfo(mps.at(i)).isWritable()) {
            QString description = QString("%1 (%2 GiB of %3 GiB free)")
                .arg(Utils::filesystemName(mps.at(i)))
                .arg((double)Utils::filesystemFree(mps.at(i))/(1<<30), 0, 'f', 2)
                .arg((double)Utils::filesystemTotal(mps.at(i))/(1<<30), 0, 'f', 2);
            ui.mountPoint->addItem(QDir::toNativeSeparators(mps.at(i)), description);
        }
    }
    if(!mountpoint.isEmpty()) {
        setMountpoint(mountpoint);
    }
    ui.mountPoint->blockSignals(false);
}


void Config::updateMountpoint(QString m)
{
    if(!m.isEmpty()) {
        mountpoint = QDir::fromNativeSeparators(m);
        qDebug() << "[Config] Mountpoint set to" << mountpoint;
    }
}


void Config::updateMountpoint(int idx)
{
    if(idx == -1) {
        return;
    }
    QString mp = ui.mountPoint->itemText(idx);
    if(!mp.isEmpty()) {
        mountpoint = QDir::fromNativeSeparators(mp);
        qDebug() << "[Config] Mountpoint set to" << mountpoint;
    }
}


void Config::setMountpoint(QString m)
{
    if(m.isEmpty()) {
        return;
    }
    int index = ui.mountPoint->findText(QDir::toNativeSeparators(m));
    if(index != -1) {
        ui.mountPoint->setCurrentIndex(index);
    }
    else {
        // keep a mountpoint that is not in the list for convenience (to allow
        // easier development)
        ui.mountPoint->addItem(QDir::toNativeSeparators(m));
        ui.mountPoint->setCurrentIndex(ui.mountPoint->findText(m));
    }
    qDebug() << "[Config] Mountpoint set to" << mountpoint;
}


void Config::autodetect()
{
    Autodetection detector(this);
    // disable tree during detection as "working" feedback.
    // TODO: replace the tree view with a splash screen during this time.
    ui.treeDevices->setEnabled(false);
    this->setCursor(Qt::WaitCursor);
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
        QList<QTreeWidgetItem*> itmList
            = ui.treeDevices->findItems("*",Qt::MatchWildcard);
        for(int i=0; i< itmList.size();i++)
        {
            //enumerate device items
            for(int j=0;j < itmList.at(i)->childCount();j++)
            {
                QString data = itmList.at(i)->child(j)->data(0, Qt::UserRole).toString();
                // unset bold flag
                QFont f = itmList.at(i)->child(j)->font(0);
                f.setBold(false);
                itmList.at(i)->child(j)->setFont(0, f);

                if(devicename == data) // item found
                {
                    f.setBold(true);
                    itmList.at(i)->child(j)->setFont(0, f);
                    itmList.at(i)->child(j)->setSelected(true); //select the item
                    itmList.at(i)->setExpanded(true); //expand the platform item
                    //ui.treeDevices->indexOfTopLevelItem(itmList.at(i)->child(j));
                    ui.treeDevices->scrollToItem(itmList.at(i)->child(j));
                    break;
                }
            }
        }
        this->unsetCursor();

        if(!detector.errdev().isEmpty()) {
            QString text;
            if(SystemInfo::platformValue(detector.errdev(),
                                         SystemInfo::CurBootloaderMethod) == "ipod") {
                text = tr("%1 \"MacPod\" found!\n"
                        "Rockbox needs a FAT formatted Ipod (so-called \"WinPod\") "
                        "to run. ").arg(SystemInfo::platformValue(
                                detector.errdev(), SystemInfo::CurName).toString());
            }
            // treat all other errors as MTP device for now.
            else {
                text = tr("%1 in MTP mode found!\n"
                        "You need to change your player to MSC mode for installation. ")
                        .arg(SystemInfo::platformValue(detector.errdev(),
                                    SystemInfo::CurName).toString());
            }
            text += tr("Until you change this installation will fail!");

            QMessageBox::critical(this, tr("Fatal error"), text, QMessageBox::Ok);
            return;
        }
        if(!detector.incompatdev().isEmpty()) {
            QString text;
            text = tr("Detected an unsupported player:\n%1\n"
                      "Sorry, Rockbox doesn't run on your player.")
                      .arg(SystemInfo::platformValue(detector.incompatdev(),
                                  SystemInfo::CurName).toString());

            QMessageBox::critical(this, tr("Fatal: player incompatible"),
                                  text, QMessageBox::Ok);
                return;
            }

        if(detector.getMountPoint() != "" )
        {
            setMountpoint(detector.getMountPoint());
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
        this->unsetCursor();
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

    for(int i = 0; i < fn.size(); i++) {
        QString f = cache + fn.at(i);
        QFile::remove(f);
    }
    updateCacheInfo(RbSettings::value(RbSettings::CachePath).toString());
}


void Config::configTts()
{
    int index = ui.comboTts->currentIndex();
    TTSBase* tts = TTSBase::getTTS(this,ui.comboTts->itemData(index).toString());

    EncTtsCfgGui gui(this,tts,TTSBase::getTTSName(ui.comboTts->itemData(index).toString()));
    gui.exec();
    updateTtsState(ui.comboTts->currentIndex());
    delete tts; /* Config objects are never deleted (in fact, they are
                   leaked..), so we can't rely on QObject, since that would
                   delete the TTSBase instance on application exit */
}

void Config::testTts()
{
    QString errstr;
    int index = ui.comboTts->currentIndex();
    TTSBase* tts;

    ui.testTTS->setEnabled(false);
    tts = TTSBase::getTTS(this,ui.comboTts->itemData(index).toString());
    if(!tts->configOk())
    {
        QMessageBox::warning(this,tr("TTS configuration invalid"),
                tr("TTS configuration invalid. \n Please configure TTS engine."));
        return;
    }
    if(!tts->start(&errstr))
    {
        QMessageBox::warning(this,tr("Could not start TTS engine."),
                tr("Could not start TTS engine.\n") + errstr
                + tr("\nPlease configure TTS engine."));
        ui.testTTS->setEnabled(true);
        return;
    }

    QString filename;
    QTemporaryFile file(this);
    // keep filename empty if the TTS can do speaking for itself.
    if(!(tts->capabilities() & TTSBase::CanSpeak)) {
        file.open();
        filename = file.fileName();
        file.close();
    }

    if(tts->voice(tr("Rockbox Utility Voice Test"),filename,&errstr) == FatalError)
    {
        tts->stop();
        QMessageBox::warning(this,tr("Could not voice test string."),
                tr("Could not voice test string.\n") + errstr
                + tr("\nPlease configure TTS engine."));
        ui.testTTS->setEnabled(false);
        return;
    }
    tts->stop();
    if(!filename.isEmpty()) {
#if defined(Q_OS_LINUX)
        QString exe = Utils::findExecutable("aplay");
        if(exe == "") exe = Utils::findExecutable("play");
        if(exe != "")
        {
            QProcess::execute(exe+" "+filename);
        }
#else
        QSound::play(filename);
#endif
    }
    ui.testTTS->setEnabled(true);
    delete tts; /* Config objects are never deleted (in fact, they are
                   leaked..), so we can't rely on QObject, since that would
                   delete the TTSBase instance on application exit */
}

void Config::configEnc()
{
    if(ui.treeDevices->selectedItems().size() == 0)
        return;

    QString devname = ui.treeDevices->selectedItems().at(0)->data(0, Qt::UserRole).toString();
    QString encoder = SystemInfo::platformValue(devname,
                    SystemInfo::CurEncoder).toString();
    ui.encoderName->setText(EncoderBase::getEncoderName(SystemInfo::platformValue(devname,
                    SystemInfo::CurEncoder).toString()));


    EncoderBase* enc = EncoderBase::getEncoder(this,encoder);

    EncTtsCfgGui gui(this,enc,EncoderBase::getEncoderName(encoder));
    gui.exec();

    updateEncState();
}


void Config::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange) {
        ui.retranslateUi(this);
        updateCacheInfo(ui.cachePath->text());
    } else {
        QWidget::changeEvent(e);
    }
}

