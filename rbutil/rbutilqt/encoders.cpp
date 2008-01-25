/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: encoders.cpp 15212 2007-10-19 21:49:07Z domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "encoders.h"
#include "browsedirtree.h"


static QMap<QString,QString> encoderList;
static QMap<QString,EncBase*> encoderCache;
 
void initEncoderList()
{
    encoderList["rbspeex"] = "Rockbox Speex Encoder";
    encoderList["lame"] = "Lame Mp3 Encoder";
}

// function to get a specific encoder
EncBase* getEncoder(QString encname)
{
  // init list if its empty
    if(encoderList.count() == 0) initEncoderList();
    
    QString encoder = encoderList.key(encname);
    
    // check cache
    if(encoderCache.contains(encoder))
        return encoderCache.value(encoder);
        
    EncBase* enc;    
    if(encoder == "rbspeex")
    {
        enc = new EncRbSpeex();
        encoderCache[encoder] = enc; 
        return enc;
    }
    else if(encoder == "lame")
    {
        enc = new EncExes(encoder);
        encoderCache[encoder] = enc; 
        return enc;
    }
    else
        return NULL;
}

// get the list of encoders, nice names
QStringList getEncoderList()
{
  // init list if its empty
    if(encoderList.count() == 0) initEncoderList();
    
    QStringList encList;
    QMapIterator<QString, QString> i(encoderList);
    while (i.hasNext()) {
     i.next();
     encList << i.value();
    }
    
    return encList;
}


/*********************************************************************
* Encoder Base
**********************************************************************/
EncBase::EncBase(QWidget *parent): QDialog(parent)
{

}

/*********************************************************************
*  GEneral Exe Encoder
**********************************************************************/
EncExes::EncExes(QString name,QWidget *parent) : EncBase(parent)
{
    m_name = name;
    
    m_TemplateMap["lame"] = "\"%exe\" %options \"%input\" \"%output\"";
    
    ui.setupUi(this);
       this->hide();
    connect(ui.reset,SIGNAL(clicked()),this,SLOT(reset()));
    connect(ui.browse,SIGNAL(clicked()),this,SLOT(browse()));
}

bool EncExes::start()
{
    m_EncExec = settings->encoderPath(m_name);
    m_EncOpts = settings->encoderOptions(m_name);
    
    m_EncTemplate = m_TemplateMap.value(m_name);

    QFileInfo enc(m_EncExec);
    if(enc.exists())
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool EncExes::encode(QString input,QString output)
{
    //qDebug() << "encoding..";
    QString execstring = m_EncTemplate;

    execstring.replace("%exe",m_EncExec);
    execstring.replace("%options",m_EncOpts);
    execstring.replace("%input",input);
    execstring.replace("%output",output);
    qDebug() << execstring;
    QProcess::execute(execstring);
    return true;
}

void EncExes::reset()
{
    ui.encoderpath->setText("");
    ui.encoderoptions->setText("");   
}

void EncExes::showCfg()
{
    // try to get config from settings
    QString exepath =settings->encoderPath(m_name);
    ui.encoderoptions->setText(settings->encoderOptions(m_name));   
    
    if(exepath == "")
    {
     
        // try to autodetect encoder
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
            if(QFileInfo(executable).isExecutable()) 
            {
                qDebug() << "found:" << executable;
                exepath = QDir::toNativeSeparators(executable);
                break;
            }
        }
    }
    
    ui.encoderpath->setText(exepath);
    
     //show dialog
    this->exec();
    
}

void EncExes::accept(void)
{
    //save settings in user config
    settings->setEncoderPath(m_name,ui.encoderpath->text());
    settings->setEncoderOptions(m_name,ui.encoderoptions->text());
    
    // sync settings
    settings->sync();
    this->close();
}

void EncExes::reject(void)
{
    this->close();
}

bool EncExes::configOk()
{
    QString path = settings->encoderPath(m_name);
    
    if (QFileInfo(path).exists())
        return true;
  
    return false;
}

void EncExes::browse()
{
    BrowseDirtree browser(this);
    browser.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

    if(QFileInfo(ui.encoderpath->text()).isDir())
    {
        browser.setDir(ui.encoderpath->text());
    }
    if(browser.exec() == QDialog::Accepted)
    {
        qDebug() << browser.getSelected();
        QString exe = browser.getSelected();
        if(!QFileInfo(exe).isExecutable())
            return;
        ui.encoderpath->setText(exe);
    }
}

/*********************************************************************
*  RB SPEEX ENCODER
**********************************************************************/
EncRbSpeex::EncRbSpeex(QWidget *parent) : EncBase(parent)
{
    ui.setupUi(this);
    this->hide();
    connect(ui.reset,SIGNAL(clicked()),this,SLOT(reset()));

    defaultQuality = 8.f;
    defaultVolume = 1.f;
    defaultComplexity = 10;
    defaultBand = false;
}


bool EncRbSpeex::start()
{
   
    // try to get config from settings
    quality = settings->encoderQuality("rbspeex");
    complexity = settings->encoderComplexity("rbspeex");
    volume = settings->encoderVolume("rbspeex");
    narrowband = settings->encoderNarrowband("rbspeex");
   

    return true;
}

bool EncRbSpeex::encode(QString input,QString output)
{
    //qDebug() << "encoding
    char errstr[512];
    
    FILE *fin,*fout;
    if ((fin = fopen(input.toUtf8(), "rb")) == NULL) {
        qDebug() << "Error: could not open input file\n";
        return false;
    }
    if ((fout = fopen(output.toUtf8(), "wb")) == NULL) {
        qDebug() << "Error: could not open output file\n";
        return false;
    }


    int ret = encode_file(fin, fout, quality, complexity, narrowband, volume,
                      errstr, sizeof(errstr));
    fclose(fout);
    fclose(fin);

    if (!ret) {
        /* Attempt to delete unfinished output */
        qDebug() << "Error:" << errstr;
        QFile(output).remove();
        return false;
    }
    return true;
}

void EncRbSpeex::reset()
{
    ui.volume->setValue(defaultVolume);
    ui.quality->setValue(defaultQuality);
    ui.complexity->setValue(defaultComplexity);
    ui.narrowband->setChecked(Qt::Unchecked); 
}

void EncRbSpeex::showCfg()
{
    //fill in the usersettings
    ui.volume->setValue(settings->encoderVolume("rbspeex"));
    ui.quality->setValue(settings->encoderQuality("rbspeex"));
    ui.complexity->setValue(settings->encoderComplexity("rbspeex"));
    
    if(settings->encoderNarrowband("rbspeex"))
        ui.narrowband->setCheckState(Qt::Checked);
    else
        ui.narrowband->setCheckState(Qt::Unchecked);

    //show dialog
    this->exec();
}

void EncRbSpeex::accept(void)
{
    //save settings in user config
    settings->setEncoderVolume("rbspeex",ui.volume->value());
    settings->setEncoderQuality("rbspeex",ui.quality->value());
    settings->setEncoderComplexity("rbspeex",ui.complexity->value());
    settings->setEncoderNarrowband("rbspeex",ui.narrowband->isChecked() ? true : false);

    // sync settings
    settings->sync();
    this->close();
}

void EncRbSpeex::reject(void)
{
    this->close();
}


bool EncRbSpeex::configOk()
{
    bool result=true;
    // check config
    
    if(settings->encoderVolume("rbspeex") <= 0)
        result =false;
    
    if(settings->encoderQuality("rbspeex") <= 0)
        result =false;
        
    if(settings->encoderComplexity("rbspeex") <= 0)
        result =false;
       
    return result;
}

