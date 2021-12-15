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
#include "ttsbase.h"

#include "ttsfestival.h"
#include "ttssapi.h"
#include "ttssapi4.h"
#include "ttsmssp.h"
#include "ttsexes.h"
#include "ttsespeak.h"
#include "ttsespeakng.h"
#include "ttsflite.h"
#include "ttsmimic.h"
#include "ttsswift.h"
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
#if !defined(Q_OS_WIN)
    ttsList["espeak"] = tr("Espeak TTS Engine");
    ttsList["espeakng"] = tr("Espeak-ng TTS Engine");
    ttsList["mimic"] = tr("Mimic TTS Engine");
#endif
    ttsList["flite"] = tr("Flite TTS Engine");
    ttsList["swift"] = tr("Swift TTS Engine");
#if defined(Q_OS_WIN)
#if 0 /* SAPI4 has been disabled since long. Keep support for now. */
    ttsList["sapi4"] = tr("SAPI4 TTS Engine");
#endif
    ttsList["sapi"] = tr("SAPI5 TTS Engine");
    ttsList["mssp"] = tr("MS Speech Platform");
#endif
#if defined(Q_OS_LINUX)
    ttsList["festival"] = tr("Festival TTS Engine");
#endif
#if defined(Q_OS_MACX)
    ttsList["carbon"] = tr("OS X System Engine");
#endif
}

// function to get a specific encoder
TTSBase* TTSBase::getTTS(QObject* parent,QString ttsName)
{

    TTSBase* tts = nullptr;
#if defined(Q_OS_WIN)
    if(ttsName == "sapi")
        tts = new TTSSapi(parent);
    else if (ttsName == "sapi4")
        tts = new TTSSapi4(parent);
    else if (ttsName == "mssp")
        tts = new TTSMssp(parent);
    else
#elif defined(Q_OS_LINUX)
    if (ttsName == "festival")
        tts = new TTSFestival(parent);
    else
#elif defined(Q_OS_MACX)
    if(ttsName == "carbon")
        tts = new TTSCarbon(parent);
    else
#endif
    if(ttsName == "espeak")
        tts = new TTSEspeak(parent);
    else if(ttsName == "espeakng")
        tts = new TTSEspeakNG(parent);
    else if(ttsName == "mimic")
        tts = new TTSMimic(parent);
    else if(ttsName == "flite")
        tts = new TTSFlite(parent);
    else if(ttsName == "swift")
        tts = new TTSSwift(parent);
    else if(ttsName == "user")
        tts = new TTSExes(parent);

    return tts;
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
