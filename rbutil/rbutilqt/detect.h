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


#ifndef DETECT_H
#define DETECT_H

#include <QString>
#include <QUrl>
#include "rbsettings.h"

class Detect 
{
public:
    Detect() {}
    
#if defined(Q_OS_WIN32)
    enum userlevel { ERR, GUEST, USER, ADMIN };
    static enum userlevel userPermissions(void);
    static QString userPermissionsString(void);
#endif
    
    static QString userName(void);
    static QString osVersionString(void);
    static QList<uint32_t> listUsbIds(void);
    static QMap<uint32_t, QString> listUsbDevices(void);

    static QUrl systemProxy(void);
    static QString installedVersion(QString mountpoint);
    static int installedTargetId(QString mountpoint);

    static bool check(RbSettings* settings,bool permission,int targetId);

};
#endif

