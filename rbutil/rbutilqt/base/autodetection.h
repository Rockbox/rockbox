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


#ifndef AUTODETECTION_H_
#define AUTODETECTION_H_

#include <QtCore>
#include "rbsettings.h"

class Autodetection :public QObject
{
    Q_OBJECT

public:
    Autodetection(QObject* parent=0);

    void setSettings(RbSettings* sett) {settings = sett;}

    bool detect();

    QString getDevice() {return m_device;}
    QString getMountPoint() {return m_mountpoint;}
    QString errdev(void) { return m_errdev; }
    QString incompatdev(void) { return m_incompat; }

private:
    QStringList getMountpoints(void);
    QString resolveMountPoint(QString);
    bool detectUsb(void);
    bool detectAjbrec(QString);

    QString m_device;
    QString m_mountpoint;
    QString m_errdev;
    QString m_incompat;
    QList<int> m_usbconid;
    RbSettings* settings;
};


#endif /*AUTODETECTION_H_*/

