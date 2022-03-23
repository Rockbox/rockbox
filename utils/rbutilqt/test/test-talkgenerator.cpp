/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2022 Dominik Riebeling
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

#include <QtTest/QtTest>
#include <QObject>
#include "talkgenerator.h"


class TestTalkGenerator : public QObject
{
    Q_OBJECT
        private slots:
        void testCorrectString();
        void testCorrectString_data();
};



void TestTalkGenerator::testCorrectString_data()
{
    struct {
        const char* language;
        const char* from;
        const char* to;
    } const correctdata[] =
    {
        { "english", "dummy text",              "dummy text"                  },
        { "deutsch", "alkaline",                "alkalein"                    },
        { "deutsch", "Batterie Typ Alkaline",   "Batterie Typ alkalein"       },
        { "deutsch", "512 kilobytes frei",      "512 kilobeits frei"          },
        // AT&T engine only.
        { "deutsch", "alphabet",                "alphabet"                    },

        // espeak
        {"svenska", "5 ampere", "5 ampär" },
        {"svenska", "bokmärken...", "bok-märken..." },
    };

    QTest::addColumn<QString>("language");
    QTest::addColumn<QString>("from");
    QTest::addColumn<QString>("to");
    for(size_t i = 0; i < sizeof(correctdata) / sizeof(correctdata[0]); i++) {
        QTest::newRow(correctdata[i].from)
            << correctdata[i].language << correctdata[i].from << correctdata[i].to;
    }
}

void TestTalkGenerator::testCorrectString()
{
    QFETCH(QString, language);
    QFETCH(QString, from);
    QFETCH(QString, to);

    TalkGenerator t(this);
    t.setLang(language);
    QString corrected = t.correctString(from);
    QCOMPARE(corrected, to);
}


QTEST_MAIN(TestTalkGenerator)

// this include is needed because we don't use a separate header file for the
// test class. It also needs to be at the end.
#include "test-talkgenerator.moc"

