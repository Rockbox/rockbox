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

#include "createvoicewindow.h"
#include "ui_createvoicefrm.h"

#include "configure.h"
#include "rbsettings.h"
#include "systeminfo.h"

CreateVoiceWindow::CreateVoiceWindow(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    voicecreator = new VoiceFileCreator(this);
    updateSettings();
    connect(ui.change,SIGNAL(clicked()),this,SLOT(change()));
}

void CreateVoiceWindow::change()
{
    // save window settings
    saveSettings();

    // call configuration dialog
    Config *cw = new Config(this,4);
    connect(cw, SIGNAL(settingsUpdated()), this, SLOT(updateSettings()));
    cw->show();    
}

void CreateVoiceWindow::accept()
{
    logger = new ProgressLoggerGui(this);
    connect(logger,SIGNAL(closed()),this,SLOT(close()));
    logger->show();    

    saveSettings();
    
    //configure voicecreator
    voicecreator->setMountPoint(RbSettings::value(RbSettings::Mountpoint).toString());
    voicecreator->setLang(ui.comboLanguage->itemData(ui.comboLanguage->currentIndex()).toString());
    voicecreator->setWavtrimThreshold(ui.wavtrimthreshold->value());
       
    //start creating
    connect(voicecreator, SIGNAL(done(bool)), logger, SLOT(setFinished()));
    connect(voicecreator, SIGNAL(logItem(QString, int)), logger, SLOT(addItem(QString, int)));
    connect(voicecreator, SIGNAL(logProgress(int, int)), logger, SLOT(setProgress(int, int)));
    connect(logger,SIGNAL(aborted()),voicecreator,SLOT(abort()));
    voicecreator->createVoiceFile();
}


/** @brief update displayed settings
 */
void CreateVoiceWindow::updateSettings(void)
{
    // fill in language combobox
    QMap<QString, QStringList> languages = SystemInfo::languages();

    for(int i = 0; i < languages.keys().size(); i++) {
        QString key = languages.keys().at(i);
        ui.comboLanguage->addItem(languages.value(key).at(1), languages.value(key).at(0));
    }
    // set saved lang
    int sel = ui.comboLanguage->findData(
            RbSettings::value(RbSettings::VoiceLanguage).toString());
    // if no saved language is found try to figure the language from the UI lang
    if(sel == -1) {
        QString uilang = RbSettings::value(RbSettings::Language).toString();
        // if no language is set default to english. Make sure not to check an empty string.
        QString f = "english";
        if(!uilang.isEmpty() && languages.contains(uilang)) {
            f = languages.value(uilang).at(0);
        }
        sel = ui.comboLanguage->findData(f);
        qDebug() << "[CreateVoiceWindow] Selected language index:" << sel;
    }
    ui.comboLanguage->setCurrentIndex(sel);

    QString ttsName = RbSettings::value(RbSettings::Tts).toString();
    TTSBase* tts = TTSBase::getTTS(this,ttsName);
    if(tts->configOk())
        ui.labelTtsProfile->setText(tr("Engine: <b>%1</b>")
            .arg(TTSBase::getTTSName(ttsName)));
    else
        ui.labelTtsProfile->setText(tr("Engine: <b>%1</b>")
            .arg("Invalid TTS configuration!"));

    ui.wavtrimthreshold->setValue(RbSettings::value(RbSettings::WavtrimThreshold).toInt());
    emit settingsUpdated();
}


/** @brief save options
  */
void CreateVoiceWindow::saveSettings(void)
{
    // save selected language
    RbSettings::setValue(RbSettings::VoiceLanguage,
                         ui.comboLanguage->itemData(ui.comboLanguage->currentIndex()).toString());
    // save wavtrim threshold value
    RbSettings::setValue(RbSettings::WavtrimThreshold,
                         ui.wavtrimthreshold->value());
    RbSettings::sync();
}

void CreateVoiceWindow::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange) {
        ui.retranslateUi(this);
        updateSettings();
    } else {
        QWidget::changeEvent(e);
    }
}

