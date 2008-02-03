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
    cw->setSettings(settings);
    cw->show();
    connect(cw, SIGNAL(settingsUpdated()), this, SIGNAL(settingsUpdated()));
}

void CreateVoiceWindow::accept()
{
    logger = new ProgressLoggerGui(this);
    logger->show();
    connect(logger,SIGNAL(closed()),this,SLOT(close()));
    
    QString lang = ui.comboLanguage->currentText();
    int wvThreshold = ui.wavtrimthreshold->value();
    
    //safe selected language
    settings->setVoiceLanguage(lang);
    settings->setWavtrimTh(wvThreshold);
    settings->sync();
    
    //configure voicecreator
    voicecreator->setSettings(settings);
    voicecreator->setMountPoint(settings->mountpoint());
    voicecreator->setTargetId(settings->curTargetId());
    voicecreator->setLang(lang);
    voicecreator->setProxy(m_proxy);
    voicecreator->setWavtrimThreshold(wvThreshold);
       
    //start creating
    voicecreator->createVoiceFile(logger);
}



void CreateVoiceWindow::setSettings(RbSettings* sett)
{
    settings = sett;

    // fill in language combobox
    QStringList languages = settings->allLanguages();
    languages.sort();
    ui.comboLanguage->addItems(languages);
    // set saved lang
    int sel = ui.comboLanguage->findText(settings->voiceLanguage());
    // if no saved language is found try to figure the language from the UI lang
    if(sel == -1) {
        QString f = settings->curLang();
        // if no language is set default to english. Make sure not to check an empty string.
        if(f.isEmpty()) f = "english";
        sel = ui.comboLanguage->findText(f, Qt::MatchStartsWith);
        qDebug() << "sel =" << sel;
        // still nothing found?
        if(sel == -1)
            sel = ui.comboLanguage->findText("english", Qt::MatchStartsWith);
    }
    ui.comboLanguage->setCurrentIndex(sel);
    
    QString ttsName = settings->curTTS();
    TTSBase* tts = getTTS(ttsName);
    tts->setCfg(settings);
    if(tts->configOk())
        ui.labelTtsProfile->setText(tr("Selected TTS engine : <b>%1</b>").arg(ttsName));
    else
        ui.labelTtsProfile->setText(tr("Selected TTS Engine: <b>%1</b>").arg("Invalid TTS configuration!"));
    
    QString encoder = settings->curEncoder();
    // only proceed if encoder setting is set
    EncBase* enc = getEncoder(encoder);
    if(enc != NULL) {
        enc->setCfg(settings);
        if(enc->configOk())
            ui.labelEncProfile->setText(tr("Selected Encoder: <b>%1</b>").arg(encoder));
        else
            ui.labelEncProfile->setText(tr("Selected Encoder: <b>%1</b>").arg("Invalid encoder configuration!"));
    }
    else
        ui.labelEncProfile->setText(tr("Selected Encoder: <b>%1</b>").arg("Invalid encoder configuration!"));
    ui.wavtrimthreshold->setValue(settings->wavtrimTh());

}





