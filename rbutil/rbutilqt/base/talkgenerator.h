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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


#ifndef TALKGENERATOR_H
#define TALKGENERATOR_H

#include <QtCore>
#include "progressloggerinterface.h"

#include "encoders.h"
#include "ttsbase.h"

//! \brief Talk generator, generates .wav and .talk files out of a list.
class TalkGenerator :public QObject
{
    Q_OBJECT
public:
    enum Status
    {
        eOK,
        eWARNING,
        eERROR
    };

    struct TalkEntry
    {
        QString toSpeak;
        QString wavfilename;
        QString talkfilename;
        QString target;
        bool voiced;
        bool encoded;

      /* We need the following members because 
       * 1) the QtConcurrent entry points are all static methods (and we 
       * need to communicate with the TalkGenerator)
       * 2) we are not guaranteed to go through the list in any 
       * particular order, so we can't use the progress slot 
       * for error checking */
      struct
      {
        EncBase* encoder;
        TTSBase* tts;
        TalkGenerator* generator; 
        int wavtrim;
      } refs;
    };

    TalkGenerator(QObject* parent);
    Status process(QList<TalkEntry>* list,int wavtrimth = -1);

public slots:
    void abort();
    void encProgress(int value);
    void ttsProgress(int value);

signals:
    void done(bool);
    void logItem(QString, int); //! set logger item
    void logProgress(int, int); //! set progress bar.

private:
    QFutureWatcher<void> encFutureWatcher;
    QFutureWatcher<void> ttsFutureWatcher;
    void encFailEntry(const TalkEntry& entry);
    void ttsFailEntry(const TalkEntry& entry, TTSStatus status, QString error);

    Status voiceList(QList<TalkEntry>* list,int wavetrimth);
    Status encodeList(QList<TalkEntry>* list);

    static void encEntryPoint(TalkEntry& entry);
    static void ttsEntryPoint(TalkEntry& entry);

    TTSBase* m_tts;
    EncBase* m_enc;

    bool m_ttsWarnings;
    bool m_userAborted;
};


#endif

