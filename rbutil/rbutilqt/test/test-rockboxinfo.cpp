/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2012 Dominik Riebeling
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
#include "rockboxinfo.h"


class TestRockboxInfo : public QObject
{
    Q_OBJECT
        private slots:
        void testVersion();
        void testVersion_data();
        void testMemory();
        void testMemory_data();
        void testTarget();
        void testTarget_data();
        void testFeatures();
        void testFeatures_data();
};


void TestRockboxInfo::testVersion_data()
{
    struct {
        const char* input;
        const char* revision;
        const char* version;
        const char* release;
    } const testdata[] =
    {
        /* Input string               revision    full version       release version */
        { "Version: r29629-110321",   "29629",    "r29629-110321",   "" },
        { "Version: r29629M-110321",  "29629M",   "r29629M-110321",  "" },
        { "Version: 3.10",            "",         "3.10",            "3.10" },
        { "Version:\t3.10",           "",         "3.10",            "3.10" },
        { "#Version: r29629-110321",  "",         "",                "" },
        { "Version: e5b1b0f-120218",  "e5b1b0f",  "e5b1b0f-120218",  "" },
        { "Version: e5b1b0fM-120218", "e5b1b0fM", "e5b1b0fM-120218", "" },
        { "#Version: e5b1b0f-120218", "",         "",                "" },
        { "Version: 3448f5b-120310",  "3448f5b",  "3448f5b-120310",  "" },
    };


    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("revision");
    QTest::addColumn<QString>("version");
    QTest::addColumn<QString>("release");
    unsigned int i;
    for(i = 0; i < sizeof(testdata) / sizeof(testdata[0]); i++) {
        for (size_t i = 0; i < sizeof(testdata) / sizeof(testdata[0]); i++) {
            QTest::newRow(testdata[i].input)
                << testdata[i].input << testdata[i].revision
                << testdata[i].version << testdata[i].release;
        }
    }
}


void TestRockboxInfo::testVersion()
{
    QFETCH(QString, input);
    QFETCH(QString, revision);
    QFETCH(QString, version);
    QFETCH(QString, release);
    QTemporaryFile tf(this);
    tf.open();
    QString filename = tf.fileName();
    tf.write(input.toLatin1());
    tf.write("\n");
    tf.close();

    RockboxInfo info("", filename);
    QCOMPARE(info.version(), QString(version));
    QCOMPARE(info.revision(), QString(revision));
    QCOMPARE(info.release(), QString(release));
}

void TestRockboxInfo::testTarget_data()
{
    QTest::addColumn<QString>("target");
    QTest::newRow("sansae200") << "sansae200";
    QTest::newRow("gigabeats") << "gigabeats";
    QTest::newRow("iriverh100") << "iriverh100";
    QTest::newRow("unknown") << "unknown";
}

void TestRockboxInfo::testTarget()
{
    int j;
    QStringList prefix;
    prefix << "Target: "; // << "Target:\t" << "Target:   ";
    for(j = 0; j < prefix.size(); ++j) {
        QFETCH(QString, target);
        QTemporaryFile tf(this);
        tf.open();
        QString filename = tf.fileName();
        tf.write(prefix.at(j).toLatin1());
        tf.write(target.toLatin1());
        tf.write("\n");
        tf.close();

        RockboxInfo info("", filename);
        QCOMPARE(info.target(), target);
    }
}

void TestRockboxInfo::testMemory_data()
{
    QTest::addColumn<QString>("memory");
    QTest::newRow("8") << "8";
    QTest::newRow("16") << "16";
    QTest::newRow("32") << "32";
    QTest::newRow("64") << "64";
}

void TestRockboxInfo::testMemory()
{
    int j;
    QStringList prefix;
    prefix << "Memory: " << "Memory:\t" << "Memory:   ";
    for(j = 0; j < prefix.size(); ++j) {
        QFETCH(QString, memory);
        QTemporaryFile tf(this);
        tf.open();
        QString filename = tf.fileName();
        tf.write(prefix.at(j).toLatin1());
        tf.write(memory.toLatin1());
        tf.write("\n");
        tf.close();

        RockboxInfo info("", filename);
        QCOMPARE(info.ram(), memory.toInt());
    }
}

void TestRockboxInfo::testFeatures_data()
{
    QTest::addColumn<QString>("features");
    QTest::newRow("1") << "backlight_brightness:button_light:dircache:flash_storage";
    QTest::newRow("2") << "pitchscreen:multivolume:multidrive_usb:quickscreen";
}

void TestRockboxInfo::testFeatures()
{
    int j;
    QStringList prefix;
    prefix << "Features: " << "Features:\t" << "Features:   ";
    for(j = 0; j < prefix.size(); ++j) {
        QFETCH(QString, features);
        QTemporaryFile tf(this);
        tf.open();
        QString filename = tf.fileName();
        tf.write(prefix.at(j).toLatin1());
        tf.write(features.toLatin1());
        tf.write("\n");
        tf.close();

        RockboxInfo info("", filename);
        QCOMPARE(info.features(), features);
    }
}

QTEST_MAIN(TestRockboxInfo)

// this include is needed because we don't use a separate header file for the
// test class. It also needs to be at the end.
#include "test-rockboxinfo.moc"

