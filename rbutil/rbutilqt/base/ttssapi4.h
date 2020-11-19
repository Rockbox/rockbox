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

#ifndef TTSSAPI4_H
#define TTSSAPI4_H

#include "ttsbase.h"
#include "ttssapi.h"

class TTSSapi4: public TTSSapi
{
    Q_OBJECT
    public:
        TTSSapi4(QObject* parent=nullptr) : TTSSapi(parent)
        {
            m_TTSTemplate = "cscript //nologo \"%exe\" "
                "/language:%lang /voice:\"%voice\" "
                "/speed:%speed \"%options\" /sapi4";
            m_TTSVoiceTemplate = "cscript //nologo \"%exe\" "
                "/language:%lang /listvoices /sapi4";
            m_TTSType = "sapi4";
        }

};

#endif
