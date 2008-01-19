/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
 *   $Id: createvoicewindow.cpp 15932 2007-12-15 13:13:57Z domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "createvoicewindow.h"
#include "ui_createvoicefrm.h"

#include "browsedirtree.h"
#include "configure.h"

CreateVoiceWindow::CreateVoiceWindow(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    voicecreator = new VoiceFileCreator(this);
    
    connect(ui.change,SIGNAL(clicked()),this,SLOT(change()));
}

void CreateVoiceWindow::change()
{
    Config *cw = new Config(this,4);
    cw->setSettings(userSettings,devices);
    cw->show();
    connect(cw, SIGNAL(settingsUpdated()), this, SIGNAL(settingsUpdated()));
}

void CreateVoiceWindow::accept()
{
    logger = new ProgressLoggerGui(this);
    logger->show();
    connect(logger,SIGNAL(closed()),this,SLOT(close()));
    
    QString platform = userSettings->value("platform").toString();
    QString lang = ui.comboLanguage->currentText();   
    int wvThreshold = ui.wavtrimthreshold->value();
    
    //safe selected language
    userSettings->setValue("voicelanguage",lang);
    userSettings->setValue("wavtrimthreshold",wvThreshold);
    userSettings->sync();
    
    //configure voicecreator
    voicecreator->setSettings(userSettings,devices);
    voicecreator->setMountPoint(userSettings->value("mountpoint").toString());
    voicecreator->setTargetId(devices->value(platform + "/targetid").toInt());
    voicecreator->setLang(lang);
    voicecreator->setProxy(m_proxy);
    voicecreator->setWavtrimThreshold(wvThreshold);
       
    //start creating
    voicecreator->createVoiceFile(logger);
}



void CreateVoiceWindow::setSettings(QSettings *user,QSettings *dev)
{
    devices = dev;
     userSettings = user;
    qDebug() << "Install::setDeviceSettings:" << devices;

    // fill in language combobox
    devices->beginGroup("languages");
    QStringList keys = devices->allKeys();
    QStringList languages;
    for(int i =0 ; i < keys.size();i++)
    {
        languages << devices->value(keys.at(i)).toString();
    }
    devices->endGroup();
    
    languages.sort();
    ui.comboLanguage->addItems(languages);
    // set saved lang
    ui.comboLanguage->setCurrentIndex(ui.comboLanguage->findText(userSettings->value("voicelanguage").toString()));
    
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
        
    ui.wavtrimthreshold->setValue(userSettings->value("wavtrimthreshold", 500).toInt());    
        
}





