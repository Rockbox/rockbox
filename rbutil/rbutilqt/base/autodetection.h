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


#ifndef AUTODETECTION_H_
#define AUTODETECTION_H_

#include <QObject>
#include <QString>
#include <QList>

class Autodetection :public QObject
{
    Q_OBJECT

public:
    Autodetection(QObject* parent=0);

    enum PlayerStatus {
        PlayerOk,
        PlayerIncompatible,
        PlayerMtpMode,
        PlayerWrongFilesystem,
        PlayerError,
    };

    struct Detected {
        QString device;
        QString mountpoint;
        enum PlayerStatus status;
    };

    bool detect();

    QList<struct Detected> detected(void);

private:
    QString resolveMountPoint(QString);
    bool detectUsb(void);
    bool detectAjbrec(QString);

    QList<struct Detected> m_detected;
    QString m_device;
    QString m_mountpoint;
    QString m_errdev;
    QString m_usberr;
    QString m_incompat;
    QList<int> m_usbconid;
};


#endif /*AUTODETECTION_H_*/

