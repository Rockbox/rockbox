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

 
#ifndef TTSBASE_H
#define TTSBASE_H

#include <QtCore>

#include "encttssettings.h"

enum TTSStatus{ FatalError, NoError, Warning };
class TTSBase : public EncTtsSettingInterface
{
    Q_OBJECT
    public:
        enum Capability { None = 0, RunInParallel = 1, CanSpeak = 2 };
        Q_DECLARE_FLAGS(Capabilities, Capability)

        TTSBase(QObject *parent);
        //! Child class should generate a clip
        virtual TTSStatus voice(QString text,QString wavfile, QString* errStr) =0;
        //! Child class should do startup
        virtual bool start(QString *errStr) =0;
        //! child class should stop
        virtual bool stop() =0;

        virtual QString voiceVendor(void) = 0;
        // configuration
        //! Child class should return true, when configuration is good
        virtual bool configOk()=0;
         //! Child class should generate and insertSetting(..) its settings
        virtual void generateSettings() = 0;
        //! Chlid class should commit the Settings to permanent storage
        virtual void saveSettings() = 0;

        virtual Capabilities capabilities() = 0;

        // static functions
        static TTSBase* getTTS(QObject* parent,QString ttsname);
        static QStringList getTTSList();
        static QString getTTSName(QString tts);

    private:
        //inits the tts List
        static void initTTSList();

    protected:
        static QMap<QString,QString> ttsList;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(TTSBase::Capabilities)

#endif
