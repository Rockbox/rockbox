/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id: detect.h 17769 2008-06-23 20:31:44Z Domonoky $
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

#endif
};

