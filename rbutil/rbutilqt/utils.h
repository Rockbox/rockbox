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


#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QUrl>

#if defined(Q_OS_WIN32)
enum userlevel { ERR, GUEST, USER, ADMIN };
enum userlevel getUserPermissions(void);
QString getUserPermissionsString(void);
#endif
QString getUserName(void);
QString getOsVersionString(void);
QList<uint32_t> listUsbIds(void);

bool recRmdir( const QString &dirName );
QString resolvePathCase(QString path);

QUrl systemProxy(void);
QString installedVersion(QString mountpoint);
int installedTargetId(QString mountpoint);

#endif

