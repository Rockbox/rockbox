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
#include <QStringList>

class Autodetection :public QObject
{
    Q_OBJECT

public:
    Autodetection(QObject* parent=nullptr);

    enum PlayerStatus {
        PlayerOk,
        PlayerIncompatible,
        PlayerMtpMode,
        PlayerWrongFilesystem,
        PlayerError,
        PlayerAmbiguous,
    };

    struct Detected {
        QString device;
        QStringList usbdevices;
        QString mountpoint;
        enum PlayerStatus status;
    };

    bool detect();

    QList<struct Detected> detected(void) { return m_detected; }

private:
    QString resolveMountPoint(QString);
    void detectUsb(void);
    void mergeMounted(void);
    void mergePatcher(void);
    QString detectAjbrec(QString);
    int findDetectedDevice(QString device);
    void updateDetectedDevice(struct Detected& entry);

    QList<struct Detected> m_detected;
    QList<int> m_usbconid;
};


#endif /*AUTODETECTION_H_*/

