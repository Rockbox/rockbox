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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef ENCODEREXES_H
#define ENCODEREXES_H

#include <QtCore>
#include "encoderbase.h"

class EncoderExe : public EncoderBase
{
    enum ESettings
    {
        eEXEPATH,
        eEXEOPTIONS
    };

    Q_OBJECT
    public:
        EncoderExe(QString name,QObject *parent = nullptr);
        bool encode(QString input,QString output);
        bool start();
        bool stop() {return true;}

        // setting
        bool configOk();
        void generateSettings();
        void saveSettings();

    private:
        QString m_name;
        QString m_EncExec;
        QString m_EncOpts;
};
#endif

