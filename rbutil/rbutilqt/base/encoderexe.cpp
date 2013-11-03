/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtCore>
#include "encoderexe.h"
#include "rbsettings.h"
#include "utils.h"
#include "Logger.h"

EncoderExe::EncoderExe(QString name,QObject *parent) : EncoderBase(parent)
{
    m_name = name;

    m_TemplateMap["lame"] = "\"%exe\" %options \"%input\" \"%output\"";

}



void EncoderExe::generateSettings()
{
    QString exepath =RbSettings::subValue(m_name,RbSettings::EncoderPath).toString();
    if(exepath == "") exepath = Utils::findExecutable(m_name);

    insertSetting(eEXEPATH,new EncTtsSetting(this,EncTtsSetting::eSTRING,
            tr("Path to Encoder:"),exepath,EncTtsSetting::eBROWSEBTN));
    insertSetting(eEXEOPTIONS,new EncTtsSetting(this,EncTtsSetting::eSTRING,
            tr("Encoder options:"),RbSettings::subValue(m_name,RbSettings::EncoderOptions)));
}

void EncoderExe::saveSettings()
{
    RbSettings::setSubValue(m_name,RbSettings::EncoderPath,getSetting(eEXEPATH)->current().toString());
    RbSettings::setSubValue(m_name,RbSettings::EncoderOptions,getSetting(eEXEOPTIONS)->current().toString());
    RbSettings::sync();
}

bool EncoderExe::start()
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

bool EncoderExe::encode(QString input,QString output)
{
    QString execstring = m_EncTemplate;

    execstring.replace("%exe",m_EncExec);
    execstring.replace("%options",m_EncOpts);
    execstring.replace("%input",input);
    execstring.replace("%output",output);
    LOG_INFO() << "cmd: " << execstring;
    int result = QProcess::execute(execstring);
    return (result == 0) ? true : false;
}


bool EncoderExe::configOk()
{
    QString path = RbSettings::subValue(m_name, RbSettings::EncoderPath).toString();

    if (QFileInfo(path).exists())
        return true;

    return false;
}

