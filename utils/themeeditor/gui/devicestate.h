/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Robert Bieber
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

#ifndef DEVICESTATE_H
#define DEVICESTATE_H

#include <QWidget>
#include <QMap>
#include <QPair>
#include <QVariant>
#include <QTabWidget>

#include "skin_parser.h"

class DeviceState : public QWidget {

    Q_OBJECT

public:
    enum InputType
    {
        Text,
        Slide,
        Spin,
        DSpin,
        Combo,
        Check
    };

    DeviceState(QWidget *parent = 0);
    virtual ~DeviceState();

    QVariant data(QString tag, int paramCount = 0,
                  skin_tag_parameter* params = 0);
    void setData(QString tag, QVariant data);

signals:
    void settingsChanged();

private slots:
    void input();

private:
    static QString fileName(QString path, bool extension);
    static QString directory(QString path, int level);
    static QString secsToString(int secs);

    QMap<QString, QPair<InputType, QWidget*> > inputs;
    QTabWidget tabs;
};

#endif // DEVICESTATE_H
