/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2010 Dominik Riebeling
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
#include "utils.h"


class TestVersionCompare : public QObject
{
    Q_OBJECT
        private slots:
        void testCompare();
        void testTrim();
};


struct compvector {
    const char* first;
    const char* second;
    const int expected;
};

const struct compvector compdata[] =
{
    { "1.2.3",                  "1.2.3 ",                       0 },
    { "1.2.3",                  " 1.2.3",                       0 },
    { "1.2.3",                  "1.2.4",                        1 },
    { "1.2.3",                  "1.3.0",                        1 },
    { "1.2.3",                  "2.0.0",                        1 },
    { "10.22.33",               "10.22.33",                     0 },
    { "10.22.33",               "10.23.0",                      1 },
    { "10.22.33",               "11.0.0",                       1 },
    { "1.2.3",                  "1.2.3.1",                      1 },
    { "1.2.3",                  "1.2.3-1",                      1 },
    { "1.2.3-1",                "1.2.3.1",                      1 },
    { "1.2.3-10",               "1.2.3.0",                      1 },
    { "1.2.3-1",                "1.2.3.10",                     1 },
    { "1.2.3-1",                "1.2.3a",                       1 },
    { "1.2.3",                  "1.2.3a",                       1 },
    { "1.2.3a",                 "1.2.3b",                       1 },
    { "1.2.3",                  "1.2.3b",                       1 },
    { "1.2.3.0",                "2.0.0",                        1 },
    { "1.2.3b",                 "2.0.0",                        1 },
    { "1.2.3",                  "2.0.0.1",                      1 },
    { "test-1.2.3",             "test-1.2.3.tar.gz",            0 },
    { "1.2.3",                  "test-1.2.3.tar.bz2",           0 },
    { "test-1.2.3.tar.gz",      "test-1.2.3.tar.bz2",           0 },
    { "test-1.2.3.tar.gz",      "program-1.2.3.1.tar.bz2",      1 },
    { "program-1.2.3.zip",      "program-1.2.3a.zip",           1 },
    { "program-1.2.3.tar.bz2",  "2.0.0",                        1 },
    { "prog-1.2-64bit.tar.bz2", "prog-1.2.3.tar.bz2",           1 },
    { "prog-1.2-64bit.tar.bz2", "prog-1.2-64bit.tar.bz2",       0 },
    { "prog-1.2-64bit.tar.bz2", "prog-1.2.3-64bit.tar.bz2",     1 },
    { "prog-1.2a-64bit.tar.bz2","prog-1.2b-64bit.tar.bz2",      1 },
    { "prog-1.2-64bit.tar.bz2", "prog-1.2.3a-64bit.tar.bz2",    1 },
    { "prog-1.2a-64bit.tar.bz2","prog-1.2.3-64bit.tar.bz2",     1 },
};

struct trimvector {
    const char* input;
    const QString expected;
};

const struct trimvector trimdata[] =
{
    { "prog-1.2-64bit.tar.bz2", "1.2"           },
    { "prog-1.2.tar.bz2",       "1.2"           },
    { "1.2.3",                  "1.2.3"         },
    { " 1.2.3",                 "1.2.3"         },
    { "1.2.3 ",                 "1.2.3"         },
    { "10.22.33",               "10.22.33"      },
    { "test-1.2.3",             "1.2.3"         },
    { "1.2.3",                  "1.2.3"         },
    { "test-1.2.3.tar.gz",      "1.2.3"         },
    { "prog-1.2-64bit.tar.bz2", "1.2"           },
    { "prog-1.2a.tar.bz2",      "1.2a"          },
    { "prog-1.2a-64bit.tar.bz2","1.2a"          },
};

void TestVersionCompare::testCompare()
{
    unsigned int i;
    for(i = 0; i < sizeof(compdata) / sizeof(struct compvector); i++) {
        QCOMPARE(Utils::compareVersionStrings(compdata[i].first,
                compdata[i].second), compdata[i].expected);
        // inverse test possible because function return values are symmetrical.
        if(compdata[i].expected != 0)
            QCOMPARE(Utils::compareVersionStrings(compdata[i].second,
                    compdata[i].first), -compdata[i].expected);
    }
}

void TestVersionCompare::testTrim()
{
    for(int i = 0; i < sizeof(trimdata) / sizeof(struct trimvector); i++) {
        QCOMPARE(Utils::trimVersionString(trimdata[i].input), trimdata[i].expected);
    }
}


QTEST_MAIN(TestVersionCompare)

// this include is needed because we don't use a separate header file for the
// test class. It also needs to be at the end.
#include "test-compareversion.moc"

