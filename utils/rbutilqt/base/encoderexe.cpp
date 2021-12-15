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

EncoderExe::EncoderExe(QString name, QObject *parent) : EncoderBase(parent),
    m_name(name)
{
}


void EncoderExe::generateSettings()
{
    QString exepath = RbSettings::subValue(m_name,RbSettings::EncoderPath).toString();
    if(exepath.isEmpty()) exepath = Utils::findExecutable(m_name);

    insertSetting(eEXEPATH, new EncTtsSetting(this, EncTtsSetting::eSTRING,
            tr("Path to Encoder:"), exepath, EncTtsSetting::eBROWSEBTN));
    insertSetting(eEXEOPTIONS, new EncTtsSetting(this, EncTtsSetting::eSTRING,
            tr("Encoder options:"), RbSettings::subValue(m_name, RbSettings::EncoderOptions)));
}

void EncoderExe::saveSettings()
{
    RbSettings::setSubValue(m_name, RbSettings::EncoderPath, getSetting(eEXEPATH)->current().toString());
    RbSettings::setSubValue(m_name, RbSettings::EncoderOptions, getSetting(eEXEOPTIONS)->current().toString());
    RbSettings::sync();
}

bool EncoderExe::start()
{
    m_EncExec = RbSettings::subValue(m_name, RbSettings::EncoderPath).toString();
    m_EncOpts = RbSettings::subValue(m_name, RbSettings::EncoderOptions).toString();

    QFileInfo enc(m_EncExec);
    return enc.exists();
}

bool EncoderExe::encode(QString input,QString output)
{
    if (!configOk())
        return false;

    QStringList args;
    args << m_EncOpts;
    args << input;
    args << output;
    int result = QProcess::execute(m_EncExec, args);
    return result == 0;
}


bool EncoderExe::configOk()
{
    QString path = RbSettings::subValue(m_name, RbSettings::EncoderPath).toString();

    return QFileInfo::exists(path);
}

