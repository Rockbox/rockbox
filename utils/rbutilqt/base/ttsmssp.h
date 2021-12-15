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

#ifndef TTSMSSP_H
#define TTSMSSP_H

#include "ttsbase.h"
#include "ttssapi.h"

class TTSMssp: public TTSSapi
{
    Q_OBJECT
    public:
        TTSMssp(QObject* parent=nullptr) : TTSSapi(parent)
        {
            m_TTSTemplate = "cscript //nologo \"%exe\" "
                "/language:%lang /voice:\"%voice\" "
                "/speed:%speed \"%options\" /mssp";
            m_TTSVoiceTemplate = "cscript //nologo \"%exe\" "
                "/language:%lang /listvoices /mssp";
            m_TTSType = "mssp";
        }

};

#endif
