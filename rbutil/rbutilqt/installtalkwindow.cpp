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
    connect(ui.buttonBrowseTTS, SIGNAL(clicked()), this, SLOT(browseTTS()));
    connect(ui.buttonBrowseEncoder, SIGNAL(clicked()), this, SLOT(browseEncoder()));
  
    connect(ui.Encodercbx,SIGNAL(currentIndexChanged(int)),this,SLOT(setEncoderOptions(int)));
    connect(ui.TTScbx,SIGNAL(currentIndexChanged(int)),this,SLOT(setTTSOptions(int)));
    
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
        QDir d(ui.lineTalkFolder->text());
        browser.setDir(d);
    }
    else
    {
        QDir d("/media");
        browser.setDir(d);
    }
    if(browser.exec() == QDialog::Accepted)
    {
        qDebug() << browser.getSelected();
        setTalkFolder(browser.getSelected());
    }
}

void InstallTalkWindow::setTalkFolder(QString folder)
{
	ui.lineTalkFolder->clear();
	ui.lineTalkFolder->insert(folder);
}

void InstallTalkWindow::browseTTS()
{
    BrowseDirtree browser(this);
    browser.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    
    if(QFileInfo(ui.TTSpath->text()).isDir())
    {
        QDir d(ui.TTSpath->text());
        browser.setDir(d);
    }
    else
    {
        QDir d("/media");
        browser.setDir(d);
    }
    if(browser.exec() == QDialog::Accepted)
    {
        qDebug() << browser.getSelected();
        setTTSExec(browser.getSelected());
    }
    
}

void InstallTalkWindow::setTTSExec(QString path)
{
	ui.TTSpath->clear();
	ui.TTSpath->insert(path);
}

void InstallTalkWindow::browseEncoder()
{
    BrowseDirtree browser(this);
    browser.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
        
    if(QFileInfo(ui.Encoderpath->text()).isDir())
    {
       QDir d(ui.Encoderpath->text());
       browser.setDir(d);
    }
    else
    {
       QDir d("/media");
       browser.setDir(d);
    }
    if(browser.exec() == QDialog::Accepted)
    {
        qDebug() << browser.getSelected();
        setEncoderExec(browser.getSelected());
    }
}

void InstallTalkWindow::setEncoderExec(QString path)
{
	ui.Encoderpath->clear();
	ui.Encoderpath->insert(path);
}

void InstallTalkWindow::setEncoderOptions(int index)
{
	QString options = talkcreator->getEncOpts(ui.Encodercbx->itemText(index));
	setEncoderOptions(options);
}
void InstallTalkWindow::setEncoderOptions(QString options)
{	
	ui.EncoderOptions->clear();
	ui.EncoderOptions->insert(options);
}
void InstallTalkWindow::setTTSOptions(QString options)
{
	ui.TTSOptions->clear();
	ui.TTSOptions->insert(options);
}
void InstallTalkWindow::setTTSOptions(int index)
{
	QString options = talkcreator->getTTsOpts(ui.TTScbx->itemText(index));
	setEncoderOptions(options);
}

void InstallTalkWindow::accept()
{
    logger = new ProgressLoggerGui(this);
    logger->show();
    
    QString folderToTalk = ui.lineTalkFolder->text();
    QString pathEncoder = ui.Encoderpath->text();
    QString pathTTS = ui.TTSpath->text();
    
    if(!QFileInfo(folderToTalk).isDir())
    {
    	 logger->addItem(tr("The Folder to Talk is wrong!"),LOGERROR);
    	 logger->abort();
    	 return;
    }
    
    if(!QFileInfo(pathEncoder).exists())
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
    
    userSettings->setValue("defaults/folderToTalk",folderToTalk);
    userSettings->setValue("defaults/pathEncoder",pathEncoder);
    userSettings->setValue("defaults/pathTTS",pathTTS);
    
    userSettings->sync();
    
    talkcreator->setDir(folderToTalk);
    talkcreator->setTTSexe(pathTTS);
    talkcreator->setEncexe(pathEncoder);
    talkcreator->setEncOpts(ui.EncoderOptions->text());
    talkcreator->setTTsOpts(ui.TTSOptions->text());
    talkcreator->setTTsType(ui.TTScbx->currentText());
    talkcreator->setEncType(ui.Encodercbx->currentText());
    
    talkcreator->setOverwriteTalk(ui.OverwriteTalk->isChecked());
    talkcreator->setOverwriteWav(ui.OverwriteWav->isChecked());
    talkcreator->setRemoveWav(ui.RemoveWav->isChecked());
    talkcreator->setRecursive(ui.recursive->isChecked());
    talkcreator->setStripExtensions(ui.StripExtensions->isChecked());

    talkcreator->createTalkFiles(logger);
    connect(logger,SIGNAL(closed()),this,SLOT(close()));    
}


void InstallTalkWindow::setDeviceSettings(QSettings *dev)
{
    devices = dev;
    qDebug() << "Install::setDeviceSettings:" << devices;

    QStringList encoders;
    QStringList encodersOpts;
    QStringList encodersTemplates;

    QStringList tts;
    QStringList ttsOpts;
    QStringList ttsTemplates;
    
    devices->beginGroup("encoders");
    QStringList keys = devices->allKeys();
    qDebug() << keys;
    for(int i=0; i < keys.size();i++)
    {
       	encoders << devices->value(keys.at(i),"null").toString();
    }
    qDebug() << encoders;
    devices->endGroup();
    for(int i=0; i < encoders.size();i++)
    {
    	devices->beginGroup(encoders.at(i));
       	encodersOpts << devices->value("options","null").toString();
       	encodersTemplates << devices->value("template","null").toString();
       	devices->endGroup();
    }
    qDebug() << encodersOpts;
    qDebug() << encodersTemplates;
    
    devices->beginGroup("tts");
    keys = devices->allKeys();
    qDebug() << keys;
    for(int i=0; i < keys.size();i++)
    {
        tts << devices->value(keys.at(i),"null").toString();
    } 
    qDebug() << tts;
    devices->endGroup();
    for(int i= 0; i < tts.size();i++)
    {
    	devices->beginGroup(tts.at(i));
       	ttsOpts << devices->value("options","null").toString();
        ttsTemplates << devices->value("template","null").toString();
        devices->endGroup();
    }
    qDebug() << ttsOpts;
    qDebug() << ttsTemplates;
    
    talkcreator->setSupportedEnc(encoders);
    talkcreator->setSupportedEncOptions(encodersOpts);
    talkcreator->setSupportedEncTemplates(encodersTemplates);
       
    talkcreator->setSupportedTTS(tts);
    talkcreator->setSupportedTTSOptions(ttsOpts);
    talkcreator->setSupportedTTSTemplates(ttsTemplates);
       
    ui.Encodercbx->insertItems(0,talkcreator->getSupportedEnc());
    ui.TTScbx->insertItems(0,talkcreator->getSupportedTTS());    
    
}




void InstallTalkWindow::setUserSettings(QSettings *user)
{
    userSettings = user;
    
    talkcreator->setMountPoint(userSettings->value("defaults/mountpoint").toString());
   
    setTalkFolder(userSettings->value("defaults/folderToTalk").toString());
    setEncoderExec(userSettings->value("defaults/pathEncoder").toString());
    setTTSExec(userSettings->value("defaults/pathTTS").toString());
}
