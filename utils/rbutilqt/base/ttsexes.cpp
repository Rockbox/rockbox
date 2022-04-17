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
#include "ttsexes.h"
#include "utils.h"
#include "rbsettings.h"
#include "Logger.h"

TTSExes::TTSExes(QObject* parent) : TTSBase(parent)
{
    /* default to espeak */
    m_name = "espeak";
    m_capabilities = TTSBase::CanSpeak;
    m_TTSTemplate = "\"%exe\" %options -w \"%wavfile\" -- \"%text\"";
    m_TTSSpeakTemplate = "\"%exe\" %options -- \"%text\"";
}


TTSBase::Capabilities TTSExes::capabilities()
{
    return m_capabilities;
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
    RbSettings::setSubValue(m_name, RbSettings::TtsPath,
            getSetting(eEXEPATH)->current().toString());
    RbSettings::setSubValue(m_name, RbSettings::TtsOptions,
            getSetting(eOPTIONS)->current().toString());
    RbSettings::sync();
}


void TTSExes::loadSettings(void)
{
    m_TTSexec = RbSettings::subValue(m_name, RbSettings::TtsPath).toString();
    if(m_TTSexec.isEmpty()) m_TTSexec = Utils::findExecutable(m_name);
    m_TTSOpts = RbSettings::subValue(m_name, RbSettings::TtsOptions).toString();
}


bool TTSExes::start(QString *errStr)
{
    loadSettings();

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

TTSStatus TTSExes::voice(const QString& text, const QString& wavfile, QString *errStr)
{
    (void) errStr;
    QString execstring;
    if(wavfile.isEmpty() && m_capabilities & TTSBase::CanSpeak) {
        if(m_TTSSpeakTemplate.isEmpty()) {
            LOG_ERROR() << "internal error: TTS announces CanSpeak "
                           "but template empty!";
            return FatalError;
        }
        execstring = m_TTSSpeakTemplate;
    }
    else if(wavfile.isEmpty()) {
        LOG_ERROR() << "no output file passed to voice() "
                       "but TTS can't speak directly.";
        return FatalError;
    }
    else {
        execstring = m_TTSTemplate;
    }

    execstring.replace("%exe",m_TTSexec);
    execstring.replace("%options",m_TTSOpts);
    execstring.replace("%wavfile",wavfile);
    execstring.replace("%text",text);

    QProcess::execute(execstring);

    if(!wavfile.isEmpty() && !QFileInfo(wavfile).isFile()) {
        LOG_ERROR() << "output file does not exist:" << wavfile;
        return FatalError;
    }
    return NoError;

}

bool TTSExes::configOk()
{
    loadSettings();
    if (QFileInfo::exists(m_TTSexec))
        return true;
    else
        return false;
}

