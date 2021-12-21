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

#include <QRegularExpression>
#include <QString>
#include <QFile>
#include "Logger.h"

RockboxInfo::RockboxInfo(QString mountpoint, QString fname) :
    m_ram(0),
    m_voicefmt(0),
    m_success(false)
{
    LOG_INFO() << "Getting version info from rockbox-info.txt";
    QFile file(mountpoint + "/" + fname);
    m_voicefmt = 400; // default value for compatibility
    if(!file.exists())
        return;

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    // read file contents
    QRegularExpression parts("^([A-Z][a-z ]+):\\s+(.*)");

    while (!file.atEnd())
    {
        QString line = file.readLine().trimmed();

        auto match = parts.match(line);
        if(!match.isValid())
        {
            continue;
        }

        if(match.captured(1) == "Version") {
            m_version = match.captured(2);

            if(match.captured(2).contains(".")) {
                // version number
                m_release = match.captured(2);
            }
            if(match.captured(2).contains("-")) {
                // hash-date format. Revision is first part.
                m_revision = match.captured(2).split("-").at(0);
                if(m_revision.startsWith("r")) {
                    m_revision.remove(0, 1);
                }
            }
        }
        else if(match.captured(1) == "Target") {
            m_target = match.captured(2);
        }
        else if(match.captured(1) == "Features") {
            m_features = match.captured(2);
        }
        else if(match.captured(1) == "Target id") {
            m_targetid = match.captured(2);
        }
        else if(match.captured(1) == "Memory") {
            m_ram = match.captured(2).toInt();
        }
        else if(match.captured(1) == "Voice format") {
            m_voicefmt = match.captured(2).toInt();
        }
    }

    file.close();
    m_success = true;
    return;
}

