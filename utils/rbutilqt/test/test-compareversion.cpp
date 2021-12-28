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
        void testCompare_data();
        void testTrim();
        void testTrim_data();
};


struct {
    const char* name;   // dataset name
    const char* first;  // first comparison value
    const char* second; // second comparison value
    const int expected; // expected result
} const compdata[] =
{
    { "trailing-whitespace",                          "1.2.3",                  "1.2.3 ",                     0 },
    { "identical",                                    "1.2.3",                  "1.2.3",                      0 },
    { "leading-whitespace",                           "1.2.3",                  " 1.2.3",                     0 },
    { "last-digit-greater",                           "1.2.3",                  "1.2.4",                      1 },
    { "last-digit-less",                              "1.2.3",                  "1.3.0",                      1 },
    { "all-digits-differ",                            "1.2.3",                  "2.0.0",                      1 },
    { "multidigit-numbers-identical",                 "10.22.33",               "10.22.33",                   0 },
    { "multidigit-last-differs",                      "10.22.33",               "10.23.0",                    1 },
    { "multidigit-next-to-last-differs",              "10.22.33",               "11.0.0",                     1 },
    { "last-additional-digit-with-dot",               "1.2.3",                  "1.2.3.1",                    1 },
    { "last-additional-digit-with-hyphen",            "1.2.3",                  "1.2.3-1",                    1 },
    { "first-additional-digit-with-hyphen",           "1.2.3-1",                "1.2.3.1",                    1 },
    { "first-additional-multidigit-with-hyphen",      "1.2.3-10",               "1.2.3.0",                    1 },
    { "first-additional-with-hyphen-last-dot",        "1.2.3-1",                "1.2.3.10",                   1 },
    { "first-additional-hyphen-last-charsuffix",      "1.2.3-1",                "1.2.3a",                     1 },
    { "last-with-char-suffix",                        "1.2.3",                  "1.2.3a",                     1 },
    { "first-and-last-with-char-suffix",              "1.2.3a",                 "1.2.3b",                     1 },
    { "last-with-char-suffix",                        "1.2.3",                  "1.2.3b",                     1 },
    { "greater-has-less-digits",                      "1.2.3.0",                "2.0.0",                      1 },
    { "less-has-char-suffix-greater-has-less-digits", "1.2.3b",                 "2.0.0",                      1 },
    { "greater-has-more-digits",                      "1.2.3",                  "2.0.0.1",                    1 },
    { "identidal-with-differing-prefix-suffix",       "test-1.2.3",             "test-1.2.3.tar.gz",          0 },
    { "identical-with-filename",                      "1.2.3",                  "test-1.2.3.tar.bz2",         0 },
    { "identical-with-different-filesuffixes",        "test-1.2.3.tar.gz",      "test-1.2.3.tar.bz2",         0 },
    { "identical-with-different-filenames",           "test-1.2.3.tar.gz",      "program-1.2.3.1.tar.bz2",    1 },
    { "last-additional-char-suffix-with-filename",    "program-1.2.3.zip",      "program-1.2.3a.zip",         1 },
    { "less-has-filename",                            "program-1.2.3.tar.bz2",  "2.0.0",                      1 },
    { "filename-64bit-vs-additional-number",          "prog-1.2-64bit.tar.bz2", "prog-1.2.3.tar.bz2",         1 },
    { "filename-64bit-identical",                     "prog-1.2-64bit.tar.bz2", "prog-1.2-64bit.tar.bz2",     0 },
    { "filename-64bit-additional-number",             "prog-1.2-64bit.tar.bz2", "prog-1.2.3-64bit.tar.bz2",   1 },
    { "filename-64bit-different-char-suffixes",       "prog-1.2a-64bit.tar.bz2","prog-1.2b-64bit.tar.bz2",    1 },
    { "filename-64bit-extra-number-and-char-suffix",  "prog-1.2-64bit.tar.bz2", "prog-1.2.3a-64bit.tar.bz2",  1 },
    { "filename-64bit-char-suffix-vs-extra-number",   "prog-1.2a-64bit.tar.bz2","prog-1.2.3-64bit.tar.bz2",   1 },
};

struct {
    const char* name;           // dataset name
    const char* input;          // input string
    const QString expected;     // output string
} const trimdata[] =
{
    { "full-filename-with-64bit",         "prog-1.2-64bit.tar.bz2", "1.2"           },
    { "full-filename",                    "prog-1.2.tar.bz2",       "1.2"           },
    { "version-only",                     "1.2.3",                  "1.2.3"         },
    { "version-with-leading-whitespace",  " 1.2.3",                 "1.2.3"         },
    { "version-with-trailing-whitespace", "1.2.3 ",                 "1.2.3"         },
    { "multidigit-version",               "10.22.33",               "10.22.33"      },
    { "name-version",                     "test-1.2.3",             "1.2.3"         },
    { "name-version-suffix",              "test-1.2.3.tar.gz",      "1.2.3"         },
    { "name-version-increment-suffix",    "prog-1.2a.tar.bz2",      "1.2a"          },
    { "name-version-increment-64bit",     "prog-1.2a-64bit.tar.bz2","1.2a"          },
};


void TestVersionCompare::testCompare_data()
{
    QTest::addColumn<QString>("first");
    QTest::addColumn<QString>("second");
    QTest::addColumn<int>("expected");
    for(size_t i = 0; i < sizeof(compdata) / sizeof(compdata[0]); i++) {
        QTest::newRow(compdata[i].name)
            << compdata[i].first << compdata[i].second << compdata[i].expected;
    }
}


void TestVersionCompare::testCompare()
{
    QFETCH(QString, first);
    QFETCH(QString, second);
    QFETCH(int, expected);

    QCOMPARE(Utils::compareVersionStrings(first, second), expected);
    if(expected != 0) {
        QCOMPARE(Utils::compareVersionStrings(second, first), -expected);
    }
}


void TestVersionCompare::testTrim_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");
    for(size_t i = 0; i < sizeof(trimdata) / sizeof(trimdata[0]); i++) {
        QTest::newRow(trimdata[i].name)
            << trimdata[i].input << trimdata[i].expected;
    }
}


void TestVersionCompare::testTrim()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);

    QCOMPARE(Utils::trimVersionString(input), expected);
}


QTEST_MAIN(TestVersionCompare)

// this include is needed because we don't use a separate header file for the
// test class. It also needs to be at the end.
#include "test-compareversion.moc"

