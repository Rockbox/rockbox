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

const int SkinTimer::millisPerTick = 250;

SkinTimer::SkinTimer(DeviceState* device, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SkinTimer),
    device(device)
{
    ui->setupUi(this);
    setupUI();
}

SkinTimer::~SkinTimer()
{
    delete ui;
}

void SkinTimer::setupUI()
{
    playStateButtons.append(ui->playButton);
    playStateButtons.append(ui->pauseButton);
    playStateButtons.append(ui->rwndButton);
    playStateButtons.append(ui->ffwdButton);

    QObject::connect(ui->startButton, SIGNAL(clicked()),
                     this, SLOT(start()));
    QObject::connect(ui->stopButton, SIGNAL(clicked()),
                     this, SLOT(stop()));
    QObject::connect(&timer, SIGNAL(timeout()),
                     this, SLOT(tick()));
    for(int i = 0; i < playStateButtons.count(); i++)
        QObject::connect(playStateButtons[i], SIGNAL(toggled(bool)),
                         this, SLOT(stateChange()));
    QObject::connect(device, SIGNAL(settingsChanged()),
                     this, SLOT(deviceChange()));

    int playState = device->data("?mp").toInt();
    switch(playState)
    {
    default:
    case 1:
        ui->playButton->setChecked(true);
        break;
    case 2:
        ui->pauseButton->setChecked(true);
        break;
    case 3:
        ui->ffwdButton->setChecked(true);
        break;
    case 4:
        ui->rwndButton->setChecked(true);
        break;
    }
}

void SkinTimer::start()
{
    ui->startButton->setEnabled(false);
    ui->stopButton->setEnabled(true);
    ui->speedBox->setEnabled(false);
    ui->durationBox->setEnabled(false);

    totalTime = ui->durationBox->value() * 1000;
    elapsedTime = 0;

    timer.setInterval(millisPerTick);
    ui->statusBar->setValue(0);
    timer.start();
}

void SkinTimer::stop()
{
    ui->startButton->setEnabled(true);
    ui->stopButton->setEnabled(false);
    ui->speedBox->setEnabled(true);
    ui->durationBox->setEnabled(true);

    timer.stop();
}

void SkinTimer::tick()
{

    elapsedTime += millisPerTick * ui->speedBox->value();
    if(elapsedTime >= totalTime)
    {
        ui->statusBar->setValue(100);
        stop();
    }

    /* Calculating the simulated time elapsed */
    double dTime = millisPerTick * ui->speedBox->value() / 1000;

    /* Adding to the device's simtime */
    device->setData("simtime", device->data("simtime").toDouble() + dTime);

    /* Adding to the song time depending on mode*/
    double songTime = device->data("?pc").toDouble();
    double trackTime = device->data("?pt").toDouble();
    if(ui->playButton->isChecked())
        songTime += dTime;
    else if(ui->rwndButton->isChecked())
        songTime -= 2 * dTime;
    else if(ui->ffwdButton->isChecked())
        songTime += 2 * dTime;

    if(songTime > trackTime)
    {
        songTime = trackTime;
        ui->pauseButton->setChecked(true);
    }
    if(songTime < 0)
    {
        songTime = 0;
        ui->pauseButton->setChecked(true);
    }

    device->setData("?pc", songTime);

    /* Updating the status bar */
    ui->statusBar->setValue(elapsedTime * 100 / totalTime);
}

void SkinTimer::stateChange()
{
    if(ui->playButton->isChecked())
        device->setData("mp", "Play");
    else if(ui->pauseButton->isChecked())
        device->setData("mp", "Pause");
    else if(ui->rwndButton->isChecked())
        device->setData("mp", "Rewind");
    else if(ui->ffwdButton->isChecked())
        device->setData("mp", "Fast Forward");
}

void SkinTimer::deviceChange()
{
    int playState = device->data("?mp").toInt();
    switch(playState)
    {
    case 1:
        ui->playButton->setChecked(true);
        break;
    case 2:
        ui->pauseButton->setChecked(true);
        break;
    case 3:
        ui->ffwdButton->setChecked(true);
        break;
    case 4:
        ui->rwndButton->setChecked(true);
        break;
    default:
        break;
    }
}
