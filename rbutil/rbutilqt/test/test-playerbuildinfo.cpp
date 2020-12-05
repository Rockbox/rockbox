/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2020 Dominik Riebeling
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

#include <QtTest>
#include <QObject>
#include "playerbuildinfo.h"
#include "rbsettings.h"

class TestPlayerBuildInfo : public QObject
{
    Q_OBJECT

    private slots:
    void testBuildInfo();
    void testBuildInfo_data();
    void testPlayerInfo();
    void testPlayerInfo_data();
};

const char* testinfo =
    "[release]\n"
    "build_url=https://buildurl/release/%VERSION%/rockbox-%TARGET%-%VERSION%.zip\n"
    "voice_url=https://buildurl/release/%VERSION%/voice-%TARGET%-%VERSION%.zip\n"
    "manual_url=https://buildurl/release/%VERSION%/manual-%TARGET%-%VERSION%.zip\n"
    "source_url=https://buildurl/release/%VERSION%/rockbox-%TARGET%-src-%VERSION%.zip\n"
    "font_url=https://buildurl/release/%VERSION%/fonts-%VERSION%.zip\n"
    "archosfmrecorder=3.11.2\n"
    "iaudiom3=3.11.2,http://dl.rockbox.org/release/3.11.2/rockbox-iaudiom5-3.11.2.zip\n"
    "sansae200 = 3.15\n"
    "iriverh100 = 3.11.2, http://dl.rockbox.org/release/3.11.2/rockbox-iriverh100-3.11.2.zip\n"
    "iriverh120 = 3.3\n"
    "iriverh300 = \n"
    "[release-candidate]\n"
    "build_url=https://buildurl/rc/%VERSION%/rockbox-%TARGET%-%VERSION%.zip\n"
    "gigabeatfx=f9dce96,http://dl.rockbox.org/rc/f9dce96/rockbox-gigabeatfx.zip\n"
    "archosfmrecorder=f9dce96\n"
    "archosrecorder = f9dce96\n"
    "iaudiox5=f9dce96,http://dl.rockbox.org/rc/f9dce96/rockbox-iaudiox5.zip\n"
    "[development]\n"
    "build_url=https://buildurl/dev/rockbox-%TARGET%.zip\n"
    "iriverh100 = be1be79\n"
    "iaudiox5 = be1be76\n"
    "[dailies]\n"
    "timestamp = 20201113\n"
    "rev = 362f7a3\n"
    "[daily]\n"
    "build_url=https://buildurl/daily/rockbox-%TARGET%-%VERSION%.zip\n"
    "iriverh100 = f9dce00\n"
    "[bleeding]\n"
    "timestamp = 20201114T105723Z\n"
    "rev = be1be79\n"
    "[status]\n"
    "archosfmrecorder=3\n"
    "iriverh100=2\n"
    "iriverh300=1\n"
    "iriverh10=0\n"
    "[voices]\n"
    "3.15=english,francais\n"
    "3.11.2=english\n"
    "daily=deutsch,english,francais\n"
    ;

Q_DECLARE_METATYPE(PlayerBuildInfo::BuildInfo);
Q_DECLARE_METATYPE(PlayerBuildInfo::BuildType);
Q_DECLARE_METATYPE(PlayerBuildInfo::DeviceInfo);

struct {
    QString target;
    PlayerBuildInfo::BuildInfo item;
    PlayerBuildInfo::BuildType type;
    QString expected;
} testdataBuild[] =
{
    // release builds
    { "iriverh100",       PlayerBuildInfo::BuildVoiceLangs, PlayerBuildInfo::TypeRelease,   "english" },
    { "iriverh300",       PlayerBuildInfo::BuildVersion,    PlayerBuildInfo::TypeRelease,   "" },
    { "iriverh300",       PlayerBuildInfo::BuildUrl,        PlayerBuildInfo::TypeRelease,   "" },
    { "iriverh10",        PlayerBuildInfo::BuildVersion,    PlayerBuildInfo::TypeRelease,   "" },
    { "iriverh10",        PlayerBuildInfo::BuildUrl,        PlayerBuildInfo::TypeRelease,   "" },
    { "archosfmrecorder", PlayerBuildInfo::BuildVersion,    PlayerBuildInfo::TypeRelease,   "3.11.2" },
    { "iaudiom3",         PlayerBuildInfo::BuildVersion,    PlayerBuildInfo::TypeRelease,   "3.11.2" },
    { "iaudiom3",         PlayerBuildInfo::BuildUrl,        PlayerBuildInfo::TypeRelease,   "http://dl.rockbox.org/release/3.11.2/rockbox-iaudiom5-3.11.2.zip" },
    { "sansae200",        PlayerBuildInfo::BuildVersion,    PlayerBuildInfo::TypeRelease,   "3.15" },
    { "sansae200",        PlayerBuildInfo::BuildUrl,        PlayerBuildInfo::TypeRelease,   "https://buildurl/release/3.15/rockbox-sansae200-3.15.zip" },
    { "iriverh100",       PlayerBuildInfo::BuildVersion,    PlayerBuildInfo::TypeRelease,   "3.11.2" },
    { "iriverh100",       PlayerBuildInfo::BuildUrl,        PlayerBuildInfo::TypeRelease,   "http://dl.rockbox.org/release/3.11.2/rockbox-iriverh100-3.11.2.zip" },
    { "iriverh100",       PlayerBuildInfo::BuildVoiceUrl,   PlayerBuildInfo::TypeRelease,   "https://buildurl/release/3.11.2/voice-iriverh100-3.11.2.zip" },
    { "iriverh100",       PlayerBuildInfo::BuildManualUrl,  PlayerBuildInfo::TypeRelease,   "https://buildurl/release/3.11.2/manual-iriverh100-3.11.2.zip" },
    { "iriverh100",       PlayerBuildInfo::BuildSourceUrl,  PlayerBuildInfo::TypeRelease,   "https://buildurl/release/3.11.2/rockbox-iriverh100-src-3.11.2.zip" },
    // h120 uses the same manual as h100.
    { "iriverh120",       PlayerBuildInfo::BuildManualUrl,  PlayerBuildInfo::TypeRelease,   "https://buildurl/release/3.3/manual-iriverh100-3.3.zip" },
    { "iriverh100",       PlayerBuildInfo::BuildFontUrl,    PlayerBuildInfo::TypeRelease,   "https://buildurl/release/3.11.2/fonts-3.11.2.zip" },

    // rc builds
    { "gigabeatfx",       PlayerBuildInfo::BuildVersion,    PlayerBuildInfo::TypeCandidate, "f9dce96" },
    { "gigabeatfx",       PlayerBuildInfo::BuildUrl,        PlayerBuildInfo::TypeCandidate, "http://dl.rockbox.org/rc/f9dce96/rockbox-gigabeatfx.zip" },
    { "archosfmrecorder", PlayerBuildInfo::BuildVersion,    PlayerBuildInfo::TypeCandidate, "f9dce96" },
    { "archosfmrecorder", PlayerBuildInfo::BuildUrl,        PlayerBuildInfo::TypeCandidate, "https://buildurl/rc/f9dce96/rockbox-archosfmrecorder-f9dce96.zip" },
    { "archosrecorder",   PlayerBuildInfo::BuildVersion,    PlayerBuildInfo::TypeCandidate, "f9dce96" },
    { "archosrecorder",   PlayerBuildInfo::BuildUrl,        PlayerBuildInfo::TypeCandidate, "https://buildurl/rc/f9dce96/rockbox-archosrecorder-f9dce96.zip" },
    { "iaudiox5",         PlayerBuildInfo::BuildVersion,    PlayerBuildInfo::TypeCandidate, "f9dce96" },
    { "iaudiox5",         PlayerBuildInfo::BuildUrl,        PlayerBuildInfo::TypeCandidate, "http://dl.rockbox.org/rc/f9dce96/rockbox-iaudiox5.zip" },
    { "iaudiox5.v",       PlayerBuildInfo::BuildVersion,    PlayerBuildInfo::TypeCandidate, "f9dce96" },
    { "iaudiox5.v",       PlayerBuildInfo::BuildUrl,        PlayerBuildInfo::TypeCandidate, "http://dl.rockbox.org/rc/f9dce96/rockbox-iaudiox5.zip" },

    // devel builds
    { "iriverh100",       PlayerBuildInfo::BuildUrl,        PlayerBuildInfo::TypeDevel,     "https://buildurl/dev/rockbox-iriverh100.zip" },
    { "iaudiox5.v",       PlayerBuildInfo::BuildVersion,    PlayerBuildInfo::TypeDevel,     "be1be76" },
    { "iaudiox5.v",       PlayerBuildInfo::BuildUrl,        PlayerBuildInfo::TypeDevel,     "https://buildurl/dev/rockbox-iaudiox5.zip" },

    // daily builds
    { "iriverh100",       PlayerBuildInfo::BuildVoiceLangs, PlayerBuildInfo::TypeDaily,     "deutsch,english,francais" },
    { "iriverh100",       PlayerBuildInfo::BuildVersion,    PlayerBuildInfo::TypeDaily,     "f9dce00" },
    { "iriverh100",       PlayerBuildInfo::BuildUrl,        PlayerBuildInfo::TypeDaily,     "https://buildurl/daily/rockbox-iriverh100-f9dce00.zip" },
};

struct {
    QString target;
    PlayerBuildInfo::DeviceInfo item;
    QString expected;
} testdataPlayer[] =
{
    { "archosfmrecorder", PlayerBuildInfo::BuildStatus,      "3"             },
    { "iriverh10",        PlayerBuildInfo::BuildStatus,      "0"             },
    { "iriverh100",       PlayerBuildInfo::BuildStatus,      "2"             },
    { "iriverh300",       PlayerBuildInfo::BuildStatus,      "1"             },
    { "archosfmrecorder", PlayerBuildInfo::BuildStatus,      "3"             },
    { "archosfmrecorder", PlayerBuildInfo::DisplayName,      "Jukebox Recorder FM"},
    { "archosfmrecorder", PlayerBuildInfo::BootloaderMethod, "none"          },
    { "archosfmrecorder", PlayerBuildInfo::BootloaderName,   ""              },
    { "archosfmrecorder", PlayerBuildInfo::BootloaderFile,   ""              },
    { "archosfmrecorder", PlayerBuildInfo::BootloaderFilter, ""              },
    { "archosfmrecorder", PlayerBuildInfo::Encoder,          "lame"          },
    { "archosfmrecorder", PlayerBuildInfo::Brand,            "Archos"        },
    { "archosfmrecorder", PlayerBuildInfo::PlayerPicture,    "archosfmrecorder"},
    { "iriverh100",       PlayerBuildInfo::BuildStatus,      "2"             },
    { "iriverh100",       PlayerBuildInfo::BootloaderMethod, "hex"           },
    { "iriverh100",       PlayerBuildInfo::BootloaderFilter, "*.hex *.zip"   },
    { "ipodmini2g",       PlayerBuildInfo::Encoder,          "rbspeex"       },
    { "078174b1",         PlayerBuildInfo::DisplayName,      "Sansa View"    },
    { "de",               PlayerBuildInfo::LanguageInfo,     "deutsch,Deutsch" },
    { "en_US",            PlayerBuildInfo::LanguageInfo,     "english-us,English (US)" },
};

void TestPlayerBuildInfo::testBuildInfo_data()
{
    QTest::addColumn<QString>("target");
    QTest::addColumn<PlayerBuildInfo::BuildInfo>("item");
    QTest::addColumn<PlayerBuildInfo::BuildType>("type");
    QTest::addColumn<QString>("expected");
    for (size_t i = 0; i < sizeof(testdataBuild) / sizeof(testdataBuild[0]); i++)
        QTest::newRow("") << testdataBuild[i].target << testdataBuild[i].item
                          << testdataBuild[i].type << testdataBuild[i].expected;
}


void TestPlayerBuildInfo::testBuildInfo()
{
    // create a temporary file for test input. Do not use QSettings() to allow
    // creating different format variations.
    QTemporaryFile tf(this);
    tf.open();
    QString filename = tf.fileName();
    tf.write(testinfo);
    tf.close();

    PlayerBuildInfo::instance()->setBuildInfo(filename);

    QFETCH(QString, target);
    QFETCH(PlayerBuildInfo::BuildInfo, item);
    QFETCH(PlayerBuildInfo::BuildType, type);
    QFETCH(QString, expected);

    RbSettings::setValue(RbSettings::CurrentPlatform, target);
    QVariant result = PlayerBuildInfo::instance()->value(item, type);
    if(result.canConvert(QMetaType::QString))
        QCOMPARE(result.toString(), QString(expected));
    else
        QCOMPARE(result.toStringList().join(","), QString(expected));
}


// NOTE: These tests rely on rbutil.ini
void TestPlayerBuildInfo::testPlayerInfo_data()
{
    QTest::addColumn<QString>("target");
    QTest::addColumn<PlayerBuildInfo::DeviceInfo>("item");
    QTest::addColumn<QString>("expected");
    for (size_t i = 0; i < sizeof(testdataPlayer) / sizeof(testdataPlayer[0]); i++)
        QTest::newRow("") << testdataPlayer[i].target << testdataPlayer[i].item
                          << testdataPlayer[i].expected;
}

void TestPlayerBuildInfo::testPlayerInfo()
{
    // create a temporary file for test input. Do not use QSettings() to allow
    // creating different format variations.
    QTemporaryFile tf(this);
    tf.open();
    QString filename = tf.fileName();
    tf.write(testinfo);
    tf.close();

    PlayerBuildInfo::instance()->setBuildInfo(filename);

    QFETCH(QString, target);
    QFETCH(PlayerBuildInfo::DeviceInfo, item);
    QFETCH(QString, expected);

    QVariant result = PlayerBuildInfo::instance()->value(item, target);
    if(result.canConvert(QMetaType::QString))
        QCOMPARE(result.toString(), QString(expected));
    else
        QCOMPARE(result.toStringList().join(","), QString(expected));
}


QTEST_MAIN(TestPlayerBuildInfo)

// this include is needed because we don't use a separate header file for the
// test class. It also needs to be at the end.
#include "test-playerbuildinfo.moc"

