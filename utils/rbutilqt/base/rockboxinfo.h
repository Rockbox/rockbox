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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


#ifndef ROCKBOXINFO_H
#define ROCKBOXINFO_H

#include <QString>

class RockboxInfo
{
public:
    RockboxInfo(QString mountpoint, QString fname = "/.rockbox/rockbox-info.txt");

    QString version() {return m_version;}
    QString features(){return m_features;}
    QString targetID() {return m_targetid;}
    QString target() {return m_target;}
    int ram() { return m_ram; }
    int voicefmt() { return m_voicefmt; }
    bool success() { return m_success; }
    QString revision(void) { return m_revision; }
    QString release(void) { return m_release; }
private:
    QString m_version;
    QString m_revision;
    QString m_release;
    QString m_features;
    QString m_targetid;
    QString m_target;
    int m_ram;
    int m_voicefmt;
    bool m_success;
};

#endif

