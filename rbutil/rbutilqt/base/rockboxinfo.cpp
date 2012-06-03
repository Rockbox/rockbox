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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "rockboxinfo.h"

#include <QtCore>
#include <QDebug>

RockboxInfo::RockboxInfo(QString mountpoint, QString fname)
{
    qDebug() << "[RockboxInfo] Getting version info from rockbox-info.txt";
    QFile file(mountpoint + "/" + fname);
    m_success = false;
    m_voicefmt = 400; // default value for compatibility
    if(!file.exists())
        return;

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    // read file contents
    QRegExp hash("^Version:\\s+(r?)([0-9a-fM]+)");
    QRegExp version("^Version:\\s+(\\S.*)");
    QRegExp release("^Version:\\s+([0-9\\.]+)\\s*$");
    QRegExp target("^Target:\\s+(\\S.*)");
    QRegExp features("^Features:\\s+(\\S.*)");
    QRegExp targetid("^Target id:\\s+(\\S.*)");
    QRegExp memory("^Memory:\\s+(\\S.*)");
    QRegExp voicefmt("^Voice format:\\s+(\\S.*)");
    while (!file.atEnd())
    {
        QString line = file.readLine().trimmed();

        if(version.indexIn(line) >= 0) {
            m_version = version.cap(1);
        }
        if(release.indexIn(line) >= 0) {
            m_release = release.cap(1);
        }
        if(hash.indexIn(line) >= 0) {
            // git hashes are usually at least 7 characters.
            // svn revisions are expected to be at least 4 digits.
            if(hash.cap(2).size() > 3)
                m_revision = hash.cap(2);
        }
        else if(target.indexIn(line) >= 0) {
            m_target = target.cap(1);
        }
        else if(features.indexIn(line) >= 0) {
            m_features = features.cap(1);
        }
        else if(targetid.indexIn(line) >= 0) {
            m_targetid = targetid.cap(1);
        }
        else if(memory.indexIn(line) >= 0) {
            m_ram = memory.cap(1).toInt();
        }
        else if(voicefmt.indexIn(line) >= 0) {
            m_voicefmt = voicefmt.cap(1).toInt();
        }
    }

    file.close();
    m_success = true;
    return;
}

