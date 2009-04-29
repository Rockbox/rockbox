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

#include "encoders.h"

#ifndef CONSOLE
#include "encodersgui.h"
#include "browsedirtree.h"
#else
#include "encodersguicli.h"
#endif


QMap<QString,QString> EncBase::encoderList;
QMap<QString,EncBase*> EncBase::encoderCache;


// initialize list of encoders
void EncBase::initEncodernamesList()
{
    encoderList["rbspeex"] = "Rockbox Speex Encoder";
    encoderList["lame"] = "Lame Mp3 Encoder";
}


// get nice name for a specific encoder
QString EncBase::getEncoderName(QString encoder)
{
    if(encoderList.isEmpty())
        initEncodernamesList();
    return encoderList.value(encoder);
}


// get a specific encoder object
EncBase* EncBase::getEncoder(QString encoder)
{
    // check cache
    if(encoderCache.contains(encoder))
        return encoderCache.value(encoder);

    EncBase* enc;
    if(encoder == "lame")
    {
        enc = new EncExes(encoder);
        encoderCache[encoder] = enc;
        return enc;
    }
    else  // rbspeex is default
    {
        enc = new EncRbSpeex();
        encoderCache[encoder] = enc;
        return enc;
    }
}


QStringList EncBase::getEncoderList()
{
    if(encoderList.isEmpty())
        initEncodernamesList();
    return encoderList.keys();
}


/*********************************************************************
* Encoder Base
**********************************************************************/
EncBase::EncBase(QObject *parent): QObject(parent)
{

}

/*********************************************************************
*  GEneral Exe Encoder
**********************************************************************/
EncExes::EncExes(QString name,QObject *parent) : EncBase(parent)
{
    m_name = name;
    
    m_TemplateMap["lame"] = "\"%exe\" %options \"%input\" \"%output\"";
}

bool EncExes::start()
{
    m_EncExec = settings->subValue(m_name, RbSettings::EncoderPath).toString();
    m_EncOpts = settings->subValue(m_name, RbSettings::EncoderOptions).toString();
    
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



void EncExes::showCfg()
{
#ifndef CONSOLE
    EncExesGui gui;
#else
	EncExesGuiCli gui;
#endif
    gui.setCfg(settings);
    gui.showCfg(m_name);
}

bool EncExes::configOk()
{
    QString path = settings->subValue(m_name, RbSettings::EncoderPath).toString();
    
    if (QFileInfo(path).exists())
        return true;
  
    return false;
}



/*********************************************************************
*  RB SPEEX ENCODER
**********************************************************************/
EncRbSpeex::EncRbSpeex(QObject *parent) : EncBase(parent)
{
   
    defaultQuality = 8.f;
    defaultVolume = 1.f;
    defaultComplexity = 10;
    defaultBand = false;
}


bool EncRbSpeex::start()
{
   
    // try to get config from settings
    quality = settings->subValue("rbspeex", RbSettings::EncoderQuality).toDouble();
    complexity = settings->subValue("rbspeex", RbSettings::EncoderComplexity).toInt();
    volume = settings->subValue("rbspeex", RbSettings::EncoderVolume).toDouble();
    narrowband = settings->subValue("rbspeex", RbSettings::EncoderNarrowBand).toBool();
   

    return true;
}

bool EncRbSpeex::encode(QString input,QString output)
{
    qDebug() << "encoding " << input << " to "<< output;  
    char errstr[512];
    
    FILE *fin,*fout;
    if ((fin = fopen(input.toLocal8Bit(), "rb")) == NULL) {
        qDebug() << "Error: could not open input file\n";
        return false;
    }
    if ((fout = fopen(output.toLocal8Bit(), "wb")) == NULL) {
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


void EncRbSpeex::showCfg()
{
#ifndef CONSOLE
    EncRbSpeexGui gui;
#else
	EncRbSpeexGuiCli gui;
#endif
    gui.setCfg(settings);
    gui.showCfg(defaultQuality,defaultVolume,defaultComplexity,defaultBand);
}

bool EncRbSpeex::configOk()
{
    bool result=true;
    // check config
    
    if(settings->subValue("rbspeex", RbSettings::EncoderVolume).toDouble() <= 0)
        result =false;
    
    if(settings->subValue("rbspeex", RbSettings::EncoderQuality).toDouble() <= 0)
        result =false;
        
    if(settings->subValue("rbspeex", RbSettings::EncoderComplexity).toInt() <= 0)
        result =false;
       
    return result;
}

