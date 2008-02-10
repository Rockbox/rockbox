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


#ifndef AUTODETECTION_H_
#define AUTODETECTION_H_

#include <QtCore>

extern "C" {
    // Ipodpatcher
#include "../ipodpatcher/ipodpatcher.h"
#include "../sansapatcher/sansapatcher.h"
};

class Autodetection :public QObject
{
    Q_OBJECT

public:
    Autodetection(QObject* parent=0);

    bool detect();

    QString getDevice() {return m_device;}
    QString getMountPoint() {return m_mountpoint;}
    QString errdev(void) { return m_errdev; }

private:
    QStringList getMountpoints(void);
    QString resolveMountPoint(QString);
    bool detectUsb(void);
    bool detectAjbrec(QString);

    QString m_device;
    QString m_mountpoint;
    QString m_errdev;
    QList<int> m_usbconid;
};


#endif /*AUTODETECTION_H_*/
