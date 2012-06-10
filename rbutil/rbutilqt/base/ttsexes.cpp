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

#include "ttsexes.h"
#include "utils.h"
#include "rbsettings.h"

TTSExes::TTSExes(QString name,QObject* parent) : TTSBase(parent)
{
    m_name = name;

    m_TemplateMap["espeak"] = "\"%exe\" %options -w \"%wavfile\" -- \"%text\"";
    m_TemplateMap["flite"] = "\"%exe\" %options -o \"%wavfile\" -t \"%text\"";
    m_TemplateMap["swift"] = "\"%exe\" %options -o \"%wavfile\" -- \"%text\"";

}

TTSBase::Capabilities TTSExes::capabilities()
{
    return RunInParallel;
}

void TTSExes::generateSettings()
{
    loadSettings();
    insertSetting(eEXEPATH, new EncTtsSetting(this, EncTtsSetting::eSTRING,
        tr("Path to TTS engine:"), m_TTSexec, EncTtsSetting::eBROWSEBTN));
    insertSetting(eOPTIONS, new EncTtsSetting(this, EncTtsSetting::eSTRING,
        tr("TTS engine options:"), m_TTSOpts));
}

void TTSExes::saveSettings()
{
    RbSettings::setSubValue(m_name,RbSettings::TtsPath,
            getSetting(eEXEPATH)->current().toString());
    RbSettings::setSubValue(m_name,RbSettings::TtsOptions,
            getSetting(eOPTIONS)->current().toString());
    RbSettings::sync();
}


void TTSExes::loadSettings(void)
{
    m_TTSexec = RbSettings::subValue(m_name,RbSettings::TtsPath).toString();
    if(m_TTSexec.isEmpty()) m_TTSexec = Utils::findExecutable(m_name);
    m_TTSOpts = RbSettings::subValue(m_name,RbSettings::TtsOptions).toString();
}


bool TTSExes::start(QString *errStr)
{
    loadSettings();
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

TTSStatus TTSExes::voice(QString text,QString wavfile, QString *errStr)
{
    (void) errStr;
    QString execstring = m_TTSTemplate;

    execstring.replace("%exe",m_TTSexec);
    execstring.replace("%options",m_TTSOpts);
    execstring.replace("%wavfile",wavfile);
    execstring.replace("%text",text);

    QProcess::execute(execstring);

    if(!QFileInfo(wavfile).isFile()) {
        qDebug() << "[TTSExes] output file does not exist:" << wavfile;
        return FatalError;
    }
    return NoError;

}

bool TTSExes::configOk()
{
    loadSettings();
    if (QFileInfo(m_TTSexec).exists())
        return true;
    else
        return false;
}

