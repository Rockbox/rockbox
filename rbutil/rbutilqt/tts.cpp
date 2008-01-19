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

#include "tts.h"

#include "browsedirtree.h"

static QMap<QString,QString> ttsList;
static QMap<QString,TTSBase*> ttsCache;
 
void initTTSList()
{
    ttsList["espeak"] = "Espeak TTS Engine";
    ttsList["flite"] = "Flite TTS Engine";
    ttsList["swift"] = "Swift TTS Engine";
#if defined(Q_OS_WIN)
    ttsList["sapi"] = "Sapi 5 TTS Engine";
#endif
  
}

// function to get a specific encoder
TTSBase* getTTS(QString ttsname)
{
    // init list if its empty
    if(ttsList.count() == 0) initTTSList();
    
    QString ttsName = ttsList.key(ttsname);
    
    // check cache
    if(ttsCache.contains(ttsName))
        return ttsCache.value(ttsName);
        
    TTSBase* tts;    
    if(ttsName == "sapi")
    {
        tts = new TTSSapi();
        ttsCache[ttsName] = tts; 
        return tts;
    }
    else 
    {
        tts = new TTSExes(ttsName);
        ttsCache[ttsName] = tts; 
        return tts;
    }
}

// get the list of encoders, nice names
QStringList getTTSList()
{
    // init list if its empty
    if(ttsList.count() == 0) initTTSList();

    QStringList ttsNameList;
    QMapIterator<QString, QString> i(ttsList);
    while (i.hasNext()) {
     i.next();
     ttsNameList << i.value();
    }
    
    return ttsNameList;
}


/*********************************************************************
* TTS Base
**********************************************************************/
TTSBase::TTSBase(QWidget *parent): QDialog(parent)
{

}

/*********************************************************************
* General TTS Exes
**********************************************************************/
TTSExes::TTSExes(QString name,QWidget *parent) : TTSBase(parent)
{
    m_name = name;
    
    m_TemplateMap["espeak"] = "\"%exe\" \"%options\" -w \"%wavfile\" \"%text\"";
    m_TemplateMap["flite"] = "\"%exe\" \"%options\" -o \"%wavfile\" \"%text\"";
    m_TemplateMap["swift"] = "\"%exe\" \"%options\" -o \"%wavfile\" \"%text\"";
       
    ui.setupUi(this);
    this->hide();
    connect(ui.reset,SIGNAL(clicked()),this,SLOT(reset()));
    connect(ui.browse,SIGNAL(clicked()),this,SLOT(browse()));
}

bool TTSExes::start(QString *errStr)
{
    userSettings->beginGroup(m_name);
    m_TTSexec = userSettings->value("ttspath","").toString();
    m_TTSOpts = userSettings->value("ttsoptions","").toString();
    userSettings->endGroup();
    
    m_TTSTemplate = m_TemplateMap.value(m_name);

    QFileInfo tts(m_TTSexec);
    if(tts.exists())
    {
        return true;
    }
    else
    {
        *errStr = tr("TTS executable not found");
        return false;
    }
}

bool TTSExes::voice(QString text,QString wavfile)
{
    QString execstring = m_TTSTemplate;

    execstring.replace("%exe",m_TTSexec);
    execstring.replace("%options",m_TTSOpts);
    execstring.replace("%wavfile",wavfile);
    execstring.replace("%text",text);
    //qDebug() << "voicing" << execstring;
    QProcess::execute(execstring);
    return true;

}


void TTSExes::reset()
{
    ui.ttspath->setText("");
    ui.ttsoptions->setText("");   
}

void TTSExes::showCfg()
{
    // try to get config from settings
    userSettings->beginGroup(m_name);
    QString exepath =userSettings->value("ttspath","").toString();
    ui.ttsoptions->setText(userSettings->value("ttsoptions","").toString());   
    userSettings->endGroup();
    
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

void TTSExes::accept(void)
{
    if(userSettings != NULL)
    {
        //save settings in user config
        userSettings->beginGroup(m_name);
        userSettings->setValue("ttspath",ui.ttspath->text());
        userSettings->setValue("ttsoptions",ui.ttsoptions->text());
        userSettings->endGroup();
        // sync settings
        userSettings->sync();
    }
    this->close();
}

void TTSExes::reject(void)
{
    this->close();
}

bool TTSExes::configOk()
{
    userSettings->beginGroup(m_name);
    QString path = userSettings->value("ttspath","").toString();
    userSettings->endGroup();
    
    if (QFileInfo(path).exists())
        return true;
  
    return false;
}

void TTSExes::browse()
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

/*********************************************************************
* TTS Sapi
**********************************************************************/
TTSSapi::TTSSapi(QWidget *parent) : TTSBase(parent)
{
    m_TTSTemplate = "cscript //nologo \"%exe\" /language:%lang /voice:\"%voice\" /speed:%speed \"%options\"";
    defaultLanguage ="english";
    ui.setupUi(this);
    this->hide();
    connect(ui.reset,SIGNAL(clicked()),this,SLOT(reset()));
    connect(ui.languagecombo,SIGNAL(currentIndexChanged(QString)),this,SLOT(updateVoices(QString)));
}


bool TTSSapi::start(QString *errStr)
{    

    userSettings->beginGroup("sapi");
    m_TTSOpts = userSettings->value("ttsoptions","").toString();
    m_TTSLanguage =userSettings->value("ttslanguage","").toString();
    m_TTSVoice=userSettings->value("ttsvoice","").toString();
    m_TTSSpeed=userSettings->value("ttsspeed","").toString();
    userSettings->endGroup();

    QFile::remove(QDir::tempPath() +"/sapi_voice.vbs");
    QFile::copy(":/builtin/sapi_voice.vbs",QDir::tempPath() + "/sapi_voice.vbs");
    m_TTSexec = QDir::tempPath() +"/sapi_voice.vbs";
    
    QFileInfo tts(m_TTSexec);
    if(!tts.exists())
    {
        *errStr = tr("Could not copy the Sapi-script");
        return false;
    }    
    // create the voice process
    QString execstring = m_TTSTemplate;
    execstring.replace("%exe",m_TTSexec);
    execstring.replace("%options",m_TTSOpts);
    execstring.replace("%lang",m_TTSLanguage);
    execstring.replace("%voice",m_TTSVoice);
    execstring.replace("%speed",m_TTSSpeed);
    
    qDebug() << "init" << execstring; 
    voicescript = new QProcess(NULL);
    //connect(voicescript,SIGNAL(readyReadStandardError()),this,SLOT(error()));
    
    voicescript->start(execstring);
    if(!voicescript->waitForStarted())
    {
        *errStr = tr("Could not start the Sapi-script");
        return false;
    }
    
    if(!voicescript->waitForReadyRead(100))  
    {
        *errStr = voicescript->readAllStandardError();
        if(*errStr != "")
            return false;    
    }
    return true;
}


QStringList TTSSapi::getVoiceList(QString language)
{
    QStringList result;
 
    QFile::copy(":/builtin/sapi_voice.vbs",QDir::tempPath() + "/sapi_voice.vbs");
    m_TTSexec = QDir::tempPath() +"/sapi_voice.vbs";
    
    QFileInfo tts(m_TTSexec);
    if(!tts.exists())
        return result;
        
    // create the voice process
    QString execstring = "cscript //nologo \"%exe\" /language:%lang /listvoices";;
    execstring.replace("%exe",m_TTSexec);
    execstring.replace("%lang",language);
    qDebug() << "init" << execstring; 
    voicescript = new QProcess(NULL);
    voicescript->start(execstring);
    if(!voicescript->waitForStarted())
        return result;
 
    voicescript->waitForReadyRead();
    
    QString dataRaw = voicescript->readAllStandardError().data();
    result = dataRaw.split(",",QString::SkipEmptyParts);
    result.sort();
    result.removeFirst();
    
    delete voicescript;
    QFile::setPermissions(QDir::tempPath() +"/sapi_voice.vbs",QFile::ReadOwner |QFile::WriteOwner|QFile::ExeOwner 
                                                             |QFile::ReadUser| QFile::WriteUser| QFile::ExeUser
                                                             |QFile::ReadGroup  |QFile::WriteGroup	|QFile::ExeGroup
                                                             |QFile::ReadOther  |QFile::WriteOther	|QFile::ExeOther );
    QFile::remove(QDir::tempPath() +"/sapi_voice.vbs");
    
    return result;
}

void TTSSapi::updateVoices(QString language)
{
    QStringList Voices = getVoiceList(language);  
    ui.voicecombo->clear();
    ui.voicecombo->addItems(Voices);    
   

}

bool TTSSapi::voice(QString text,QString wavfile)
{
    QString query = "SPEAK\t"+wavfile+"\t"+text+"\r\n";
    qDebug() << "voicing" << query;
    voicescript->write(query.toUtf8());
    voicescript->write("SYNC\tbla\r\n");
    voicescript->waitForReadyRead();
    return true;
}

bool TTSSapi::stop()
{   
    QString query = "QUIT\r\n";
    voicescript->write(query.toUtf8());
    voicescript->waitForFinished();
    delete voicescript;
    QFile::setPermissions(QDir::tempPath() +"/sapi_voice.vbs",QFile::ReadOwner |QFile::WriteOwner|QFile::ExeOwner 
                                                             |QFile::ReadUser| QFile::WriteUser| QFile::ExeUser
                                                             |QFile::ReadGroup  |QFile::WriteGroup	|QFile::ExeGroup
                                                             |QFile::ReadOther  |QFile::WriteOther	|QFile::ExeOther );
    QFile::remove(QDir::tempPath() +"/sapi_voice.vbs");
    return true;
}


void TTSSapi::reset()
{
    ui.ttsoptions->setText("");  
    ui.languagecombo->setCurrentIndex(ui.languagecombo->findText(defaultLanguage));  
}

void TTSSapi::showCfg()
{
    // try to get config from settings
    userSettings->beginGroup("sapi");
    ui.ttsoptions->setText(userSettings->value("ttsoptions","").toString());  
    QString selLang = userSettings->value("ttslanguage",defaultLanguage).toString();
    QString selVoice = userSettings->value("ttsvoice","").toString();    
    ui.speed->setValue(userSettings->value("ttsspeed",0).toInt());
    userSettings->endGroup();
      
     // fill in language combobox

    deviceSettings->beginGroup("languages");
    QStringList keys = deviceSettings->allKeys();
    QStringList languages;
    for(int i =0 ; i < keys.size();i++)
    {
        languages << deviceSettings->value(keys.at(i)).toString();
    }
    deviceSettings->endGroup();
    
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

void TTSSapi::accept(void)
{
    if(userSettings != NULL)
    {
        //save settings in user config
        userSettings->beginGroup("sapi");
        userSettings->setValue("ttsoptions",ui.ttsoptions->text());
        userSettings->setValue("ttslanguage",ui.languagecombo->currentText());
        userSettings->setValue("ttsvoice",ui.voicecombo->currentText());
        userSettings->setValue("ttsspeed",QString("%1").arg(ui.speed->value()));
        userSettings->endGroup();
        // sync settings
        userSettings->sync();
    }
    this->close();
}

void TTSSapi::reject(void)
{
    this->close();
}

bool TTSSapi::configOk()
{
    return true;
}


