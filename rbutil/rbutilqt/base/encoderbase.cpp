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

#include "encoderbase.h"
#include "utils.h"
#include "rbsettings.h"
#include "encoderrbspeex.h"
#include "encoderlame.h"
#include "encoderexe.h"

#include "Logger.h"

/*********************************************************************
* Encoder Base
**********************************************************************/
QMap<QString,QString> EncoderBase::encoderList;

EncoderBase::EncoderBase(QObject *parent): EncTtsSettingInterface(parent)
{

}

// initialize list of encoders
void EncoderBase::initEncodernamesList()
{
    encoderList["rbspeex"] = "Rockbox Speex Encoder";
    encoderList["lame"] = "Lame Mp3 Encoder";
}


// get nice name for a specific encoder
QString EncoderBase::getEncoderName(QString encoder)
{
    if(encoderList.isEmpty())
        initEncodernamesList();
    return encoderList.value(encoder);
}


// get a specific encoder object
EncoderBase* EncoderBase::getEncoder(QObject* parent,QString encoder)
{
    EncoderBase* enc;
    if(encoder == "lame")
    {
        enc = new EncoderLame(parent);
        if (!enc->configOk())
        {
            LOG_WARNING() << "Could not load lame dll, falling back to command "
                             "line lame. This is notably slower.";
            delete enc;
            enc = new EncoderExe(encoder, parent);

        }
        return enc;
    }
    else  // rbspeex is default
    {
        enc = new EncoderRbSpeex(parent);
        return enc;
    }
}


QStringList EncoderBase::getEncoderList()
{
    if(encoderList.isEmpty())
        initEncodernamesList();
    return encoderList.keys();
}

