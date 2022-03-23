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


#ifndef TALKGENERATOR_H
#define TALKGENERATOR_H

#include <QtCore>
#include "progressloglevels.h"

#include "encoderbase.h"
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
    };

    TalkGenerator(QObject* parent);

    Status process(QList<TalkEntry>* list,int wavtrimth = -1);
    QString correctString(const QString& s);

public slots:
    void abort();
    void setLang(const QString& name);

signals:
    void done(bool);
    void logItem(QString, int); //! set logger item
    void logProgress(int, int); //! set progress bar.

private:
    Status voiceList(QList<TalkEntry>* list,int wavetrimth);
    Status encodeList(QList<TalkEntry>* list);

    TTSBase* m_tts;
    EncoderBase* m_enc;

    QString m_lang;

    struct CorrectionItems
    {
        QString search;
        QString replace;
        QString modifier;
    };
    QList<struct CorrectionItems> m_corrections;

    bool m_abort;


};


#endif

