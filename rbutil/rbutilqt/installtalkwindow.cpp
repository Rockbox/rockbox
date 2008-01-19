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
#include "configure.h"

InstallTalkWindow::InstallTalkWindow(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    talkcreator = new TalkFileCreator(this);

    connect(ui.buttonBrowse, SIGNAL(clicked()), this, SLOT(browseFolder()));
    connect(ui.change,SIGNAL(clicked()),this,SLOT(change()));

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

void InstallTalkWindow::change()
{
    Config *cw = new Config(this,4);
    cw->setSettings(userSettings,devices);
    cw->show();
    connect(cw, SIGNAL(settingsUpdated()), this, SIGNAL(settingsUpdated()));
}

void InstallTalkWindow::accept()
{
    logger = new ProgressLoggerGui(this);
    logger->show();
    connect(logger,SIGNAL(closed()),this,SLOT(close()));

    QString folderToTalk = ui.lineTalkFolder->text();
     
    if(!QFileInfo(folderToTalk).isDir())
    {
        logger->addItem(tr("The Folder to Talk is wrong!"),LOGERROR);
        logger->abort();
        return;
    }

    userSettings->setValue("last_talked_folder", folderToTalk);

    userSettings->sync();

    talkcreator->setSettings(userSettings,devices);
    talkcreator->setDir(QDir(folderToTalk));
    talkcreator->setMountPoint(userSettings->value("mountpoint").toString());
    
    talkcreator->setOverwriteTalk(ui.OverwriteTalk->isChecked());
    talkcreator->setOverwriteWav(ui.OverwriteWav->isChecked());
    talkcreator->setRemoveWav(ui.RemoveWav->isChecked());
    talkcreator->setRecursive(ui.recursive->isChecked());
    talkcreator->setStripExtensions(ui.StripExtensions->isChecked());
    talkcreator->setTalkFolders(ui.talkFolders->isChecked());
    talkcreator->setTalkFiles(ui.talkFiles->isChecked());

    talkcreator->createTalkFiles(logger);
}


void InstallTalkWindow::setSettings(QSettings *user,QSettings *dev)
{
    devices = dev;
    userSettings = user;
    qDebug() << "Install::setDeviceSettings:" << devices;

    QString ttsName = userSettings->value("tts", "none").toString();
    TTSBase* tts = getTTS(ttsName);
    tts->setCfg(userSettings,devices);
    if(tts->configOk())
        ui.labelTtsProfile->setText(tr("Selected TTS engine : <b>%1</b>").arg(ttsName));
    else
        ui.labelTtsProfile->setText(tr("Selected TTS Engine: <b>%1</b>").arg("Invalid TTS configuration!"));
    
    QString encoder = userSettings->value("encoder", "none").toString();
    EncBase* enc = getEncoder(encoder);
    enc->setUserCfg(userSettings);
    if(enc->configOk())
        ui.labelEncProfile->setText(tr("Selected Encoder: <b>%1</b>").arg(encoder));
    else
        ui.labelEncProfile->setText(tr("Selected Encoder: <b>%1</b>").arg("Invalid encoder configuration!"));
        
    setTalkFolder(userSettings->value("last_talked_folder").toString());

}

