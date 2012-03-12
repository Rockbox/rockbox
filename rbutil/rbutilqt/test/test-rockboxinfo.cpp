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
        void testMemory();
        void testTarget();
        void testFeatures();
};


void TestRockboxInfo::testVersion()
{
    struct testvector {
        const char* versionline;
        const char* revisionstring;
        const char* versionstring;
        const char* releasestring;
    };

    const struct testvector testdata[] =
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


    unsigned int i;
    for(i = 0; i < sizeof(testdata) / sizeof(struct testvector); i++) {
        QTemporaryFile tf(this);
        tf.open();
        QString filename = tf.fileName();
        tf.write(testdata[i].versionline);
        tf.write("\n");
        tf.close();

        RockboxInfo info("", filename);
        QCOMPARE(info.version(), QString(testdata[i].versionstring));
        QCOMPARE(info.revision(), QString(testdata[i].revisionstring));
        QCOMPARE(info.release(), QString(testdata[i].releasestring));
    }
}


void TestRockboxInfo::testTarget()
{
    int i, j;
    QStringList targets;
    targets << "sansae200" << "gigabeats" << "iriverh100" << "unknown";
    QStringList prefix;
    prefix << "Target: "; // << "Target:\t" << "Target:   ";
    for(j = 0; j < prefix.size(); ++j) {
        for(i = 0; i < targets.size(); i++) {
            QTemporaryFile tf(this);
            tf.open();
            QString filename = tf.fileName();
            tf.write(prefix.at(j).toLocal8Bit());
            tf.write(targets.at(i).toLocal8Bit());
            tf.write("\n");
            tf.close();

            RockboxInfo info("", filename);
            QCOMPARE(info.target(), targets.at(i));
        }
    }
}


void TestRockboxInfo::testMemory()
{
    int i, j;
    QStringList memsizes;
    memsizes << "8" << "16" << "32" << "64";
    QStringList prefix;
    prefix << "Memory: " << "Memory:\t" << "Memory:   ";
    for(j = 0; j < prefix.size(); ++j) {
        for(i = 0; i < memsizes.size(); i++) {
            QTemporaryFile tf(this);
            tf.open();
            QString filename = tf.fileName();
            tf.write(prefix.at(j).toLocal8Bit());
            tf.write(memsizes.at(i).toLocal8Bit());
            tf.write("\n");
            tf.close();

            RockboxInfo info("", filename);
            QCOMPARE(info.ram(), memsizes.at(i).toInt());
        }
    }
}


void TestRockboxInfo::testFeatures()
{
    int i, j;
    QStringList features;
    features << "backlight_brightness:button_light:dircache:flash_storage"
             << "pitchscreen:multivolume:multidrive_usb:quickscreen";
    QStringList prefix;
    prefix << "Features: " << "Features:\t" << "Features:   ";
    for(j = 0; j < prefix.size(); ++j) {
        for(i = 0; i < features.size(); i++) {
            QTemporaryFile tf(this);
            tf.open();
            QString filename = tf.fileName();
            tf.write(prefix.at(j).toLocal8Bit());
            tf.write(features.at(i).toLocal8Bit());
            tf.write("\n");
            tf.close();

            RockboxInfo info("", filename);
            QCOMPARE(info.features(), features.at(i));
        }
    }
}

QTEST_MAIN(TestRockboxInfo)

// this include is needed because we don't use a separate header file for the
// test class. It also needs to be at the end.
#include "test-rockboxinfo.moc"

