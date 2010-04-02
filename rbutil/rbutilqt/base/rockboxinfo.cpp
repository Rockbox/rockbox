/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id$
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

RockboxInfo::RockboxInfo(QString mountpoint)
{
    qDebug() << "[RockboxInfo] trying to find rockbox-info at" << mountpoint;
    QFile file(mountpoint + "/.rockbox/rockbox-info.txt");
    m_success = false;
    if(!file.exists())
        return;

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    // read file contents
    while (!file.atEnd())
    {
        QString line = file.readLine();

        if(line.contains("Version:"))
        {
            m_version = line.remove("Version:").trimmed();
        }
        else if(line.contains("Target: "))
        {
            m_target = line.remove("Target: ").trimmed();
        }
        else if(line.contains("Features:"))
        {
            m_features = line.remove("Features:").trimmed();
        }
        else if(line.contains("Target id:"))
        {
            m_targetid = line.remove("Target id:").trimmed();
        }
        else if(line.contains("Memory:"))
        {
            m_ram = line.remove("Memory:").trimmed().toInt();
        }
    }

    file.close();
    m_success = true;
    return;
}

