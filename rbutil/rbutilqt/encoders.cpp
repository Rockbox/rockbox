/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: talkfile.cpp 15212 2007-10-19 21:49:07Z domonoky $
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
    userSettings->beginGroup(m_name);
    m_EncExec = userSettings->value("encoderpath","").toString();
    m_EncOpts = userSettings->value("encoderoptions","").toString();
    userSettings->endGroup();
    
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
    userSettings->beginGroup(m_name);
    QString exepath =userSettings->value("encoderpath","").toString();
    ui.encoderoptions->setText(userSettings->value("encoderoptions","").toString());   
    userSettings->endGroup();
    
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
    if(userSettings != NULL)
    {
        //save settings in user config
        userSettings->beginGroup(m_name);
        userSettings->setValue("encoderpath",ui.encoderpath->text());
        userSettings->setValue("encoderoptions",ui.encoderoptions->text());
        userSettings->endGroup();
        // sync settings
        userSettings->sync();
    }
    this->close();
}

void EncExes::reject(void)
{
    this->close();
}

bool EncExes::configOk()
{
    userSettings->beginGroup(m_name);
    QString path = userSettings->value("encoderpath","").toString();
    userSettings->endGroup();
    
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
    defaultComplexity =3;
    defaultBand = false;

}


bool EncRbSpeex::start()
{
    // no user config
    if(userSettings == NULL)
    {
        return false;
    }
    // try to get config from settings
    userSettings->beginGroup("rbspeex");
    quality = userSettings->value("quality",defaultQuality).toDouble();
    complexity = userSettings->value("complexity",defaultComplexity).toInt();
    volume =userSettings->value("volume",defaultVolume).toDouble();
    narrowband = userSettings->value("narrowband",false).toBool();
   
    userSettings->endGroup();

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
    userSettings->beginGroup("rbspeex");
    ui.volume->setValue(userSettings->value("volume",defaultVolume).toDouble());
    ui.quality->setValue(userSettings->value("quality",defaultQuality).toDouble());
    ui.complexity->setValue(userSettings->value("complexity",defaultComplexity).toInt());
    
    if(userSettings->value("narrowband","False").toString() == "True")
        ui.narrowband->setCheckState(Qt::Checked);
    else
        ui.narrowband->setCheckState(Qt::Unchecked);

    userSettings->endGroup();

    //show dialog
    this->exec();
}

void EncRbSpeex::accept(void)
{
    if(userSettings != NULL)
    {
        //save settings in user config
        userSettings->beginGroup("rbspeex");
        userSettings->setValue("volume",ui.volume->value());
        userSettings->setValue("quality",ui.quality->value());
        userSettings->setValue("complexity",ui.complexity->value());
        userSettings->setValue("narrowband",ui.narrowband->isChecked() ? true : false);

        userSettings->endGroup();
        // sync settings
        userSettings->sync();
    }
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
    userSettings->beginGroup("rbspeex"); 
    
    if(userSettings->value("volume","null").toDouble() <= 0)
        result =false;
    
    if(userSettings->value("quality","null").toDouble() <= 0)
        result =false;
        
    if(userSettings->value("complexity","null").toInt() <= 0)
        result =false;
        
    userSettings->endGroup();
       
    return result;
}

