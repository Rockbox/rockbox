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

bool TTSExes::start()
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
    m_TTSTemplate = "cscript //nologo \"%exe\" /language:%lang \"%options\"";
    defaultLanguage ="english";
    ui.setupUi(this);
    this->hide();
    connect(ui.reset,SIGNAL(clicked()),this,SLOT(reset()));
}


bool TTSSapi::start()
{    

    userSettings->beginGroup("sapi");
    m_TTSOpts = userSettings->value("ttsoptions","").toString();
    m_TTSLanguage =userSettings->value("ttslanguage","").toString();
    userSettings->endGroup();

    QFile::copy(":/builtin/sapi_voice.vbs",QDir::tempPath() + "/sapi_voice.vbs");
    m_TTSexec = QDir::tempPath() +"/sapi_voice.vbs";
    
    QFileInfo tts(m_TTSexec);
    if(!tts.exists())
        return false;
        
    // create the voice process
    QString execstring = m_TTSTemplate;
    execstring.replace("%exe",m_TTSexec);
    execstring.replace("%options",m_TTSOpts);
    execstring.replace("%lang",m_TTSLanguage);
    qDebug() << "init" << execstring; 
    voicescript = new QProcess(NULL);
    voicescript->start(execstring);
    if(!voicescript->waitForStarted())
        return false;
    return true;
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
    return true;
}


void TTSSapi::reset()
{
    ui.ttsoptions->setText("");  
    ui.ttslanguage->setText(defaultLanguage);  
}

void TTSSapi::showCfg()
{
    // try to get config from settings
    userSettings->beginGroup("sapi");
    ui.ttsoptions->setText(userSettings->value("ttsoptions","").toString());  
    ui.ttslanguage->setText(userSettings->value("ttslanguage",defaultLanguage).toString());     
    userSettings->endGroup();
      
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
        userSettings->setValue("ttslanguage",ui.ttslanguage->text());
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


