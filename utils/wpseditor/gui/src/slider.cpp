/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Rostilav Checkan
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

#include "slider.h"
#include <QDebug>
//
Slider::Slider(QWidget *parent, QString caption, int min, int max ):QDialog(parent),mCaption(caption) {
    setupUi ( this );
    connect(horslider, SIGNAL(valueChanged(int)), this, SIGNAL(valueChanged(int)));
    connect(this, SIGNAL(valueChanged(int)), this, SLOT(slotValueChanged(int)));
    setWindowTitle(mCaption);
    horslider->setMinimum(min);
    horslider->setMaximum(max);
}
//
int Slider::value() {
    return horslider->value();
}
void Slider::slotValueChanged(int step) {
    setWindowTitle(tr("%1 = %2 ").arg(mCaption).arg(step));
}



