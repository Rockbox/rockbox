/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
*
*   Copyright (C) 2020 by Solomon Peachy
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

#ifndef TTSESPEAKNG_H
#define TTSESPEAKNG_H

#include <QtCore>
#include "ttsexes.h"

class TTSEspeakNG : public TTSExes
{
    Q_OBJECT
    public:
        TTSEspeakNG(QObject* parent=nullptr) : TTSExes(parent)
        {
            m_name = "espeak-ng";

            m_TTSTemplate = "\"%exe\" %options -w \"%wavfile\" -- \"%text\"";
            m_TTSSpeakTemplate = "\"%exe\" %options -- \"%text\"";
            m_capabilities = TTSBase::CanSpeak;
        }
};

#endif
