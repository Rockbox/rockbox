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
#include "utils.h"
#include "rbsettings.h"

/*********************************************************************
* Encoder Base
**********************************************************************/
QMap<QString,QString> EncBase::encoderList;

EncBase::EncBase(QObject *parent): EncTtsSettingInterface(parent)
{

}

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
EncBase* EncBase::getEncoder(QObject* parent,QString encoder)
{
    EncBase* enc;
    if(encoder == "lame")
    {
        enc = new EncExes(encoder,parent);
        return enc;
    }
    else  // rbspeex is default
    {
        enc = new EncRbSpeex(parent);
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
*  GEneral Exe Encoder
**********************************************************************/
EncExes::EncExes(QString name,QObject *parent) : EncBase(parent)
{
    m_name = name;

    m_TemplateMap["lame"] = "\"%exe\" %options \"%input\" \"%output\"";

}



void EncExes::generateSettings()
{
    QString exepath =RbSettings::subValue(m_name,RbSettings::EncoderPath).toString();
    if(exepath == "") exepath = findExecutable(m_name);

    insertSetting(eEXEPATH,new EncTtsSetting(this,EncTtsSetting::eSTRING,
            tr("Path to Encoder:"),exepath,EncTtsSetting::eBROWSEBTN));
    insertSetting(eEXEOPTIONS,new EncTtsSetting(this,EncTtsSetting::eSTRING,
            tr("Encoder options:"),RbSettings::subValue(m_name,RbSettings::EncoderOptions)));
}

void EncExes::saveSettings()
{
    RbSettings::setSubValue(m_name,RbSettings::EncoderPath,getSetting(eEXEPATH)->current().toString());
    RbSettings::setSubValue(m_name,RbSettings::EncoderOptions,getSetting(eEXEOPTIONS)->current().toString());
    RbSettings::sync();
}

bool EncExes::start()
{
    m_EncExec = RbSettings::subValue(m_name, RbSettings::EncoderPath).toString();
    m_EncOpts = RbSettings::subValue(m_name, RbSettings::EncoderOptions).toString();

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
    int result = QProcess::execute(execstring);
    return (result == 0) ? true : false;
}


bool EncExes::configOk()
{
    QString path = RbSettings::subValue(m_name, RbSettings::EncoderPath).toString();

    if (QFileInfo(path).exists())
        return true;

    return false;
}

/*********************************************************************
*  RB SPEEX ENCODER
**********************************************************************/
EncRbSpeex::EncRbSpeex(QObject *parent) : EncBase(parent)
{

}

void EncRbSpeex::generateSettings()
{
    insertSetting(eVOLUME,new EncTtsSetting(this,EncTtsSetting::eDOUBLE,
        tr("Volume:"),RbSettings::subValue("rbspeex",RbSettings::EncoderVolume),1.0,10.0));
    insertSetting(eQUALITY,new EncTtsSetting(this,EncTtsSetting::eDOUBLE,
        tr("Quality:"),RbSettings::subValue("rbspeex",RbSettings::EncoderQuality),0,10.0));
    insertSetting(eCOMPLEXITY,new EncTtsSetting(this,EncTtsSetting::eINT,
        tr("Complexity:"),RbSettings::subValue("rbspeex",RbSettings::EncoderComplexity),0,10));
    insertSetting(eNARROWBAND,new EncTtsSetting(this,EncTtsSetting::eBOOL,
        tr("Use Narrowband:"),RbSettings::subValue("rbspeex",RbSettings::EncoderNarrowBand)));
}

void EncRbSpeex::saveSettings()
{
    //save settings in user config
    RbSettings::setSubValue("rbspeex",RbSettings::EncoderVolume,
                            getSetting(eVOLUME)->current().toDouble());
    RbSettings::setSubValue("rbspeex",RbSettings::EncoderQuality,
                            getSetting(eQUALITY)->current().toDouble());
    RbSettings::setSubValue("rbspeex",RbSettings::EncoderComplexity,
                            getSetting(eCOMPLEXITY)->current().toInt());
    RbSettings::setSubValue("rbspeex",RbSettings::EncoderNarrowBand,
                            getSetting(eNARROWBAND)->current().toBool());

    RbSettings::sync();
}

bool EncRbSpeex::start()
{

    // try to get config from settings
    quality = RbSettings::subValue("rbspeex", RbSettings::EncoderQuality).toDouble();
    complexity = RbSettings::subValue("rbspeex", RbSettings::EncoderComplexity).toInt();
    volume = RbSettings::subValue("rbspeex", RbSettings::EncoderVolume).toDouble();
    narrowband = RbSettings::subValue("rbspeex", RbSettings::EncoderNarrowBand).toBool();


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

bool EncRbSpeex::configOk()
{
    bool result=true;
    // check config

    if(RbSettings::subValue("rbspeex", RbSettings::EncoderVolume).toDouble() <= 0)
        result =false;

    if(RbSettings::subValue("rbspeex", RbSettings::EncoderQuality).toDouble() <= 0)
        result =false;

    if(RbSettings::subValue("rbspeex", RbSettings::EncoderComplexity).toInt() <= 0)
        result =false;

    return result;
}

