/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: tts.cpp 15212 2007-10-19 21:49:07Z domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
#include "ttsgui.h"

#include "rbsettings.h"
#include "tts.h"
#include "browsedirtree.h"
 
TTSSapiGui::TTSSapiGui(TTSSapi* sapi,QDialog* parent) : QDialog(parent)
{
    m_sapi= sapi;
    ui.setupUi(this);
    this->hide();
    connect(ui.reset,SIGNAL(clicked()),this,SLOT(reset()));
    connect(ui.languagecombo,SIGNAL(currentIndexChanged(QString)),this,SLOT(updateVoices(QString)));
    connect(ui.usesapi4,SIGNAL(stateChanged(int)),this,SLOT(useSapi4Changed(int)));
} 

void TTSSapiGui::showCfg()
{
    // try to get config from settings
    ui.ttsoptions->setText(settings->ttsOptions("sapi"));  
    QString selLang = settings->ttsLang("sapi");
    QString selVoice = settings->ttsVoice("sapi");    
    ui.speed->setValue(settings->ttsSpeed("sapi"));
    if(settings->ttsUseSapi4())
        ui.usesapi4->setCheckState(Qt::Checked);
    else
        ui.usesapi4->setCheckState(Qt::Unchecked);
    
     // fill in language combobox
    QStringList languages = settings->allLanguages();
    
    languages.sort();
    ui.languagecombo->clear();
    ui.languagecombo->addItems(languages);
    
    // set saved lang
    ui.languagecombo->setCurrentIndex(ui.languagecombo->findText(selLang));

    // fill in voice combobox      
    updateVoices(selLang);
      
     // set saved lang
    ui.voicecombo->setCurrentIndex(ui.voicecombo->findText(selVoice));  
      
     //show dialog
    this->exec();
    
}


void TTSSapiGui::reset()
{
    ui.ttsoptions->setText("");  
    ui.languagecombo->setCurrentIndex(ui.languagecombo->findText("english"));  
}



void TTSSapiGui::accept(void)
{
    //save settings in user config
    settings->setTTSOptions("sapi",ui.ttsoptions->text());
    settings->setTTSLang("sapi",ui.languagecombo->currentText());
    settings->setTTSVoice("sapi",ui.voicecombo->currentText());
    settings->setTTSSpeed("sapi",ui.speed->value());
    if(ui.usesapi4->checkState() == Qt::Checked)
        settings->setTTSUseSapi4(true);
    else
        settings->setTTSUseSapi4(false);
    // sync settings
    settings->sync();

    this->done(0);
}

void TTSSapiGui::reject(void)
{
    this->done(0);
}

void TTSSapiGui::updateVoices(QString language)
{
    QStringList Voices = m_sapi->getVoiceList(language);
    ui.voicecombo->clear();
    ui.voicecombo->addItems(Voices);    

}

void TTSSapiGui::useSapi4Changed(int)
{
    if(ui.usesapi4->checkState() == Qt::Checked)
        settings->setTTSUseSapi4(true);
    else
        settings->setTTSUseSapi4(false);
    // sync settings
    settings->sync();
    updateVoices(ui.languagecombo->currentText());
   
}

TTSExesGui::TTSExesGui(QDialog* parent) : QDialog(parent)
{
    ui.setupUi(this);
    this->hide();
    connect(ui.reset,SIGNAL(clicked()),this,SLOT(reset()));
    connect(ui.browse,SIGNAL(clicked()),this,SLOT(browse()));
}


void TTSExesGui::reset()
{
    ui.ttspath->setText("");
    ui.ttsoptions->setText("");   
}

void TTSExesGui::showCfg(QString name)
{
    m_name = name;
    // try to get config from settings
    QString exepath =settings->ttsPath(m_name);
    ui.ttsoptions->setText(settings->ttsOptions(m_name));   
    
    if(exepath == "")
    {
     
        //try autodetect tts   
#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
        QStringList path = QString(getenv("PATH")).split(":", QString::SkipEmptyParts);
#elif defined(Q_OS_WIN)
        QStringList path = QString(getenv("PATH")).split(";", QString::SkipEmptyParts);
#endif
        qDebug() << path;
        for(int i = 0; i < path.size(); i++) 
        {
            QString executable = QDir::fromNativeSeparators(path.at(i)) + "/" + m_name;
#if defined(Q_OS_WIN)
            executable += ".exe";
            QStringList ex = executable.split("\"", QString::SkipEmptyParts);
            executable = ex.join("");
#endif
            qDebug() << executable;
            if(QFileInfo(executable).isExecutable())
            {
                exepath= QDir::toNativeSeparators(executable);
                break;
            }
        }
     
    }
    
    ui.ttspath->setText(exepath);
    
     //show dialog
    this->exec();
    
}

void TTSExesGui::accept(void)
{
    //save settings in user config
    settings->setTTSPath(m_name,ui.ttspath->text());
    settings->setTTSOptions(m_name,ui.ttsoptions->text());
    // sync settings
    settings->sync();
    
    this->done(0);
}

void TTSExesGui::reject(void)
{
    this->done(0);
}


void TTSExesGui::browse()
{
    BrowseDirtree browser(this);
    browser.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

    if(QFileInfo(ui.ttspath->text()).isDir())
    {
        browser.setDir(ui.ttspath->text());
    }
    if(browser.exec() == QDialog::Accepted)
    {
        qDebug() << browser.getSelected();
        QString exe = browser.getSelected();
        if(!QFileInfo(exe).isExecutable())
            return;
        ui.ttspath->setText(exe);
    }
}

