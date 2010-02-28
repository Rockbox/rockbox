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

#include "ttsbase.h"

#include "ttsfestival.h"
#include "ttssapi.h"
#include "ttsexes.h"
#if defined(Q_OS_MACX)
#include "ttscarbon.h"
#endif

// list of tts names and identifiers
QMap<QString,QString> TTSBase::ttsList;

TTSBase::TTSBase(QObject* parent): EncTtsSettingInterface(parent)
{

}

// static functions
void TTSBase::initTTSList()
{
    ttsList["espeak"] = "Espeak TTS Engine";
    ttsList["flite"] = "Flite TTS Engine";
    ttsList["swift"] = "Swift TTS Engine";
#if defined(Q_OS_WIN)
    ttsList["sapi"] = "Sapi TTS Engine";
#endif
#if defined(Q_OS_LINUX)
    ttsList["festival"] = "Festival TTS Engine";
#endif
#if defined(Q_OS_MACX)
    ttsList["carbon"] = "OS X System Engine";
#endif
}

// function to get a specific encoder
TTSBase* TTSBase::getTTS(QObject* parent,QString ttsName)
{

    TTSBase* tts;
#if defined(Q_OS_WIN)
    if(ttsName == "sapi")
    {
        tts = new TTSSapi(parent);
        return tts;
    }
    else
#endif
#if defined(Q_OS_LINUX)
    if (ttsName == "festival")
    {
        tts = new TTSFestival(parent);
        return tts;
    }
    else
#endif
#if defined(Q_OS_MACX)
    if(ttsName == "carbon")
    {
        tts = new TTSCarbon(parent);
        return tts;
    }
    else
#endif
    if (true) // fix for OS other than WIN or LINUX
    {
        tts = new TTSExes(ttsName,parent);
        return tts;
    }
}

// get the list of encoders, nice names
QStringList TTSBase::getTTSList()
{
    // init list if its empty
    if(ttsList.count() == 0)
        initTTSList();

    return ttsList.keys();
}

// get nice name of a specific tts
QString TTSBase::getTTSName(QString tts)
{
    if(ttsList.isEmpty())
        initTTSList();
    return ttsList.value(tts);
}
