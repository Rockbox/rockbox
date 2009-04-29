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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QUrl>

bool recRmdir( const QString &dirName );
QString resolvePathCase(QString path);
qulonglong filesystemFree(QString path);
QString findExecutable(QString name); 

class RockboxInfo
{
public:
    RockboxInfo(QString mountpoint);
    bool open();
    
    QString version() {return m_version;}
    QString features(){return m_features;}
    QString targetID() {return m_targetid;}
    QString target() {return m_target;}
private:
    QString m_path;
    QString m_version;
    QString m_features;
    QString m_targetid;
    QString m_target;
};

#endif

