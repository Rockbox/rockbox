/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id$
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

TTSFestivalGui::TTSFestivalGui(TTSFestival* api, QDialog* parent) :
	QDialog(parent), festival(api)
{
    ui.setupUi(this);
    this->setModal(true);
    this->setDisabled(true);
    this->show();

    connect(ui.clientButton, SIGNAL(clicked()), this, SLOT(onBrowseClient()));
    connect(ui.serverButton, SIGNAL(clicked()), this, SLOT(onBrowseServer()));

    connect(ui.refreshButton, SIGNAL(clicked()), this, SLOT(onRefreshButton()));
    connect(ui.voicesBox, SIGNAL(activated(QString)), this, SLOT(updateDescription(QString)));
    connect(ui.showDescriptionCheckbox, SIGNAL(stateChanged(int)), this, SLOT(onShowDescription(int)));
}

void TTSFestivalGui::showCfg()
{
	qDebug() << "show\tpaths: " << settings->ttsPath("festival") << "\n"
			<< "\tvoice: " << settings->ttsVoice("festival");

	// will populate the voices if the paths are correct,
	// otherwise, it will require the user to press Refresh
	updateVoices();

	// try to get config from settings
	QStringList paths = settings->ttsPath("festival").split(":");
	if(paths.size() == 2)
	{
		ui.serverPath->setText(paths[0]);
		ui.clientPath->setText(paths[1]);
	}

	this->setEnabled(true);
    this->exec();
}

void TTSFestivalGui::accept(void)
{
    //save settings in user config
	QString newPath = QString("%1:%2").arg(ui.serverPath->text().trimmed()).arg(ui.clientPath->text().trimmed());
	qDebug() << "set\tpaths: " << newPath << "\n\tvoice: " << ui.voicesBox->currentText();
	settings->setTTSPath("festival", newPath);
    settings->setTTSVoice("festival", ui.voicesBox->currentText());

    settings->sync();

    this->done(0);
}

void TTSFestivalGui::reject(void)
{
    this->done(0);
}

void TTSFestivalGui::onBrowseClient()
{
	BrowseDirtree browser(this);
	browser.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

	QFileInfo currentPath(ui.clientPath->text().trimmed());
	if(currentPath.isDir())
	{
		browser.setDir(ui.clientPath->text());
	}
	else if (currentPath.isFile())
	{
		browser.setDir(currentPath.dir().absolutePath());
	}
	if(browser.exec() == QDialog::Accepted)
	{
		qDebug() << browser.getSelected();
		QString exe = browser.getSelected();
		if(!QFileInfo(exe).isExecutable())
			return;
		ui.clientPath->setText(exe);
	}
}

void TTSFestivalGui::onBrowseServer()
{
	BrowseDirtree browser(this);
	browser.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

	QFileInfo currentPath(ui.serverPath->text().trimmed());
	if(currentPath.isDir())
	{
		browser.setDir(ui.serverPath->text());
	}
	else if (currentPath.isFile())
	{
		browser.setDir(currentPath.dir().absolutePath());
	}
	if(browser.exec() == QDialog::Accepted)
	{
		qDebug() << browser.getSelected();
		QString exe = browser.getSelected();
		if(!QFileInfo(exe).isExecutable())
			return;
		ui.serverPath->setText(exe);
	}
}

void TTSFestivalGui::onRefreshButton()
{
	/* Temporarily commit the settings so that we get the new path when we check for voices */
	QString newPath = QString("%1:%2").arg(ui.serverPath->text().trimmed()).arg(ui.clientPath->text().trimmed());
	QString oldPath = settings->ttsPath("festival");
	qDebug() << "new path: " << newPath << "\n" << "old path: " << oldPath << "\nuse new: " << (newPath != oldPath);

	if(newPath != oldPath)
	{
		qDebug() << "Using new paths for getVoiceList";
		settings->setTTSPath("festival", newPath);
		settings->sync();
	}

	updateVoices();

	if(newPath != oldPath)
	{
		settings->setTTSPath("festival", oldPath);
		settings->sync();
	}
}

void TTSFestivalGui::onShowDescription(int state)
{
	if(state == Qt::Unchecked)
		ui.descriptionLabel->setText("");
	else
		updateDescription(ui.voicesBox->currentText());
}

void TTSFestivalGui::updateVoices()
{
	ui.voicesBox->clear();
	ui.voicesBox->addItem(tr("Loading.."));

	QStringList voiceList = festival->getVoiceList();
	ui.voicesBox->clear();
	ui.voicesBox->addItems(voiceList);

	ui.voicesBox->setCurrentIndex(ui.voicesBox->findText(settings->ttsVoice("festival")));

	updateDescription(settings->ttsVoice("festival"));
}

void TTSFestivalGui::updateDescription(QString value)
{
	if(ui.showDescriptionCheckbox->checkState() == Qt::Checked)
	{
		ui.descriptionLabel->setText(tr("Querying festival"));
		ui.descriptionLabel->setText(festival->getVoiceInfo(value));
	}
}
