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

#include "skintimer.h"
#include "ui_skintimer.h"

const int SkinTimer::millisPerTick = 10;

SkinTimer::SkinTimer(DeviceState* device, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SkinTimer),
    device(device)
{
    ui->setupUi(this);
}

SkinTimer::~SkinTimer()
{
    delete ui;
}

void SkinTimer::start()
{

}

void SkinTimer::stop()
{

}

void SkinTimer::tick()
{

}

void SkinTimer::stateChange()
{

}
