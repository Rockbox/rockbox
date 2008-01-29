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

#ifndef CONSOLE
#include "encodersgui.h"
#endif

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



void EncExes::showCfg()
{
    EncExesGui gui;
    gui.setCfg(settings);
    gui.showCfg(m_name);
}

bool EncExes::configOk()
{
    QString path = settings->encoderPath(m_name);
    
    if (QFileInfo(path).exists())
        return true;
  
    return false;
}



/*********************************************************************
*  RB SPEEX ENCODER
**********************************************************************/
EncRbSpeex::EncRbSpeex(QWidget *parent) : EncBase(parent)
{
   
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


void EncRbSpeex::showCfg()
{
    EncRbSpeexGui gui;
    gui.setCfg(settings);
    gui.showCfg(defaultQuality,defaultVolume,defaultComplexity,defaultBand);
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

