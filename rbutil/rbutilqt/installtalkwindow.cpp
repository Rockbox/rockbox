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

#include "installtalkwindow.h"
#include "ui_installtalkfrm.h"

#include "browsedirtree.h"

InstallTalkWindow::InstallTalkWindow(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    talkcreator = new TalkFileCreator(this);

    connect(ui.buttonBrowse, SIGNAL(clicked()), this, SLOT(browseFolder()));

    ui.OverwriteWav->setChecked(true);
    ui.RemoveWav->setChecked(true);
    ui.recursive->setChecked(true);
    ui.OverwriteTalk->setChecked(true);
    ui.StripExtensions->setChecked(true);
}

void InstallTalkWindow::browseFolder()
{
    BrowseDirtree browser(this);
    browser.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);

    if(QFileInfo(ui.lineTalkFolder->text()).isDir())
    {
        browser.setDir(ui.lineTalkFolder->text());
    }
    else
    {
        browser.setDir("/media"); // FIXME: This looks Linux specific
    }
    if(browser.exec() == QDialog::Accepted)
    {
        qDebug() << browser.getSelected();
        setTalkFolder(browser.getSelected());
    }
}

void InstallTalkWindow::setTalkFolder(QString folder)
{
    ui.lineTalkFolder->setText(folder);
}


void InstallTalkWindow::accept()
{
    logger = new ProgressLoggerGui(this);
    logger->show();
    connect(logger,SIGNAL(closed()),this,SLOT(close()));

    QString folderToTalk = ui.lineTalkFolder->text();
    
    // tts
    QString preset = userSettings->value("ttspreset").toString();
    userSettings->beginGroup(preset);
    QString pathTTS = userSettings->value("binary").toString();
    QString ttsOpts = userSettings->value("options").toString();
    QString ttsLanguage = userSettings->value("language").toString();
    QString ttsTemplate = userSettings->value("template").toString();
    QString ttsType =userSettings->value("type").toString();
    userSettings->endGroup();
    
    //encoder
    QString encoderPreset = userSettings->value("encpreset").toString();
    userSettings->beginGroup(encoderPreset);
    QString pathEncoder = userSettings->value("binary").toString();
    QString encOpts = userSettings->value("options").toString();
    QString encTemplate = userSettings->value("template").toString();
    QString encType =userSettings->value("type").toString();
    userSettings->endGroup();
  
    if(!QFileInfo(folderToTalk).isDir())
    {
        logger->addItem(tr("The Folder to Talk is wrong!"),LOGERROR);
        logger->abort();
        return;
    }

    if(!QFileInfo(pathEncoder).isExecutable())
    {
        logger->addItem(tr("Path to Encoder is wrong!"),LOGERROR);
        logger->abort();
        return;
    }

    if(!QFileInfo(pathTTS).exists())
    {
         logger->addItem(tr("Path to TTS is wrong!"),LOGERROR);
         logger->abort();
         return;
    }

    userSettings->setValue("last_talked_folder", folderToTalk);

    userSettings->sync();

    talkcreator->setDir(QDir(folderToTalk));
    talkcreator->setMountPoint(userSettings->value("mountpoint").toString());
    talkcreator->setTTSexe(pathTTS);
    talkcreator->setTTsOpts(ttsOpts);
    talkcreator->setTTsLanguage(ttsLanguage);
    talkcreator->setTTsType(ttsType);
    talkcreator->setTTsTemplate(ttsTemplate);
   
    talkcreator->setEncexe(pathEncoder);
    talkcreator->setEncOpts(encOpts);
    talkcreator->setEncTemplate(encTemplate);
    talkcreator->setEncType(encType);
    
    talkcreator->setOverwriteTalk(ui.OverwriteTalk->isChecked());
    talkcreator->setOverwriteWav(ui.OverwriteWav->isChecked());
    talkcreator->setRemoveWav(ui.RemoveWav->isChecked());
    talkcreator->setRecursive(ui.recursive->isChecked());
    talkcreator->setStripExtensions(ui.StripExtensions->isChecked());
    talkcreator->setTalkFolders(ui.talkFolders->isChecked());
    talkcreator->setTalkFiles(ui.talkFiles->isChecked());

    talkcreator->createTalkFiles(logger);
}


void InstallTalkWindow::setDeviceSettings(QSettings *dev)
{
    devices = dev;
    qDebug() << "Install::setDeviceSettings:" << devices;

    QString profile;

    profile = userSettings->value("ttspreset", "none").toString();
    devices->beginGroup("tts");
    ui.labelTtsProfile->setText(tr("TTS Profile: <b>%1</b>")
        .arg(devices->value(profile, tr("Invalid TTS profile!")).toString()));
    qDebug() << profile;
    devices->endGroup();
    profile = userSettings->value("encpreset", "none").toString();
    devices->beginGroup("encoders");
    ui.labelEncProfile->setText(tr("Encoder Profile: <b>%1</b>")
        .arg(devices->value(profile, tr("Invalid encoder profile!")).toString()));
    qDebug() << profile;
    devices->endGroup();
}




void InstallTalkWindow::setUserSettings(QSettings *user)
{
    userSettings = user;

    setTalkFolder(userSettings->value("last_talked_folder").toString());

}
