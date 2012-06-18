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
#include "serverinfo.h"

class TestServerInfo : public QObject
{
    Q_OBJECT
        private slots:
        void testMain();
};

const char* testinfo =
    "[release]\n"
    "archosfmrecorder=3.11.2\n"
    "iaudiom3=3.11.2,http://download.rockbox.org/release/3.11.2/rockbox-iaudiom5-3.11.2.zip\n"
    "sansae200 = 3.11.2\n"
    "iriverh100 = 3.11.2, http://download.rockbox.org/release/3.11.2/rockbox-iriverh100-3.11.2.zip\n"
    "iriverh300 = \n"
    "[release-candidate]\n"
    "gigabeatfx=f9dce96,http://download.rockbox.org/release-candidate/f9dce96/rockbox-gigabeatfx.zip\n"
    "archosfmrecorder=f9dce96\n"
    "archosrecorder = f9dce96\n"
    "iaudiox5=f9dce96,http://download.rockbox.org/release-candidate/f9dce96/rockbox-iaudiox5.zip\n";


struct testvector {
    const char* target;
    ServerInfo::ServerInfos entry;
    const char* expected;
};


const struct testvector testdata[] =
{
    { "archosfmrecorder", ServerInfo::CurReleaseVersion,   "3.11.2" },
    { "iaudiom3",         ServerInfo::CurReleaseVersion,   "3.11.2" },
    { "iaudiom3",         ServerInfo::CurReleaseUrl,       "http://download.rockbox.org/release/3.11.2/rockbox-iaudiom5-3.11.2.zip" },
    { "sansae200",        ServerInfo::CurReleaseVersion,   "3.11.2" },
    { "sansae200",        ServerInfo::CurReleaseUrl,       "http://download.rockbox.org/release/3.11.2/rockbox-sansae200-3.11.2.zip" },
    { "iriverh100",       ServerInfo::CurReleaseVersion,   "3.11.2" },
    { "iriverh100",       ServerInfo::CurReleaseUrl,       "http://download.rockbox.org/release/3.11.2/rockbox-iriverh100-3.11.2.zip" },
    { "iriverh300",       ServerInfo::CurReleaseVersion,   "" },
    { "iriverh300",       ServerInfo::CurReleaseUrl,       "" },
    { "iriverh10",        ServerInfo::CurReleaseVersion,   "" },
    { "iriverh10",        ServerInfo::CurReleaseUrl,       "" },
    { "gigabeatfx",       ServerInfo::RelCandidateVersion, "f9dce96" },
    { "gigabeatfx",       ServerInfo::RelCandidateUrl,     "http://download.rockbox.org/release-candidate/f9dce96/rockbox-gigabeatfx.zip" },
    { "archosfmrecorder", ServerInfo::RelCandidateVersion, "" },
    { "archosfmrecorder", ServerInfo::RelCandidateUrl,     "" },
    { "archosrecorder",   ServerInfo::RelCandidateVersion, "" },
    { "archosrecorder",   ServerInfo::RelCandidateUrl,     "" },
    { "iaudiox5",         ServerInfo::RelCandidateVersion, "f9dce96" },
    { "iaudiox5",         ServerInfo::RelCandidateUrl,     "http://download.rockbox.org/release-candidate/f9dce96/rockbox-iaudiox5.zip" },
    { "iaudiox5.v",       ServerInfo::RelCandidateVersion, "f9dce96" },
    { "iaudiox5.v",       ServerInfo::RelCandidateUrl,     "http://download.rockbox.org/release-candidate/f9dce96/rockbox-iaudiox5.zip" },
    { "iaudiox5.v",       ServerInfo::ManualPdfUrl,        "http://download.rockbox.org/manual/rockbox-iaudiox5.pdf" },
    { "iriverh100",       ServerInfo::ManualHtmlUrl,       "http://download.rockbox.org/manual/rockbox-iriverh100/rockbox-build.html" },
    { "iriverh120",       ServerInfo::ManualZipUrl,        "http://download.rockbox.org/manual/rockbox-iriverh100-html.zip" },
};


void TestServerInfo::testMain()
{
    // create a temporary file for test input. Do not use QSettings() to allow
    // creating different format variations.
    QTemporaryFile tf(this);
    tf.open();
    QString filename = tf.fileName();
    tf.write(testinfo);
    tf.close();

    ServerInfo::readBuildInfo(filename);

    unsigned int i;
    for(i = 0; i < sizeof(testdata) / sizeof(struct testvector); i++) {
        QString result = ServerInfo::platformValue(testdata[i].target, testdata[i].entry).toString();
        QCOMPARE(result, QString(testdata[i].expected));
    }
}


QTEST_MAIN(TestServerInfo)

// this include is needed because we don't use a separate header file for the
// test class. It also needs to be at the end.
#include "test-serverinfo.moc"

