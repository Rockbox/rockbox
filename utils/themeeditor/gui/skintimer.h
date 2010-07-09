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

#ifndef SKINTIMER_H
#define SKINTIMER_H

#include <QWidget>
#include <QTimer>
#include <QToolButton>

#include "devicestate.h"

namespace Ui {
    class SkinTimer;
}

class SkinTimer : public QWidget {
    Q_OBJECT
public:
    static const int millisPerTick;

    SkinTimer(DeviceState* device, QWidget *parent = 0);
    ~SkinTimer();

private slots:
    void start();
    void stop();
    void tick();
    void stateChange();
    void deviceChange();

private:
    void setupUI();

    Ui::SkinTimer *ui;
    DeviceState* device;

    QTimer timer;
    unsigned long int elapsedTime;
    unsigned long int totalTime;

    QList<QToolButton*> playStateButtons;
};

#endif // SKINTIMER_H
