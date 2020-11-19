/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
*
*   Copyright (C) 2012 by Dominik Riebeling
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#ifndef TTSESPEAK_H
#define TTSESPEAK_H

#include <QtCore>
#include "ttsexes.h"

class TTSEspeak : public TTSExes
{
    Q_OBJECT
    public:
        TTSEspeak(QObject* parent=nullptr) : TTSExes(parent)
        {
            m_name = "espeak";

            /* default to espeak */
            m_TTSTemplate = "\"%exe\" %options -w \"%wavfile\" -- \"%text\"";
            m_TTSSpeakTemplate = "\"%exe\" %options -- \"%text\"";
            m_capabilities = TTSBase::CanSpeak;
        }
};

#endif
