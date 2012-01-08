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

#ifndef ENCODERS_H
#define ENCODERS_H

#include <QtCore>

#include "encttssettings.h"


class EncoderBase : public EncTtsSettingInterface
{
    Q_OBJECT
    public:
        EncoderBase(QObject *parent );

        //! Child class should encode a wav file 
        virtual bool encode(QString input,QString output) =0;
        //! Child class should do startup
        virtual bool start()=0;
        //! Child class should stop
        virtual bool stop()=0;

        // settings
        //! Child class should return true when configuration is ok
        virtual bool configOk()=0;
        //! Child class should fill in settingsList
        virtual void generateSettings() = 0;
        //! Child class should commit from SettingsList to permanent storage
        virtual void saveSettings() = 0;

        // static functions
        static QString getEncoderName(QString name);
        static EncoderBase* getEncoder(QObject* parent,QString name);
        static QStringList getEncoderList(void);

    private:
        static void initEncodernamesList(void);

    protected:
        static QMap<QString,QString> encoderList;
};

#endif

