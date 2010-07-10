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

#include "rbtoucharea.h"
#include "rbscreen.h"
#include "devicestate.h"

#include <QPainter>

RBTouchArea::RBTouchArea(int width, int height, QString action,
                         const RBRenderInfo& info)
                             : QGraphicsItem(info.screen()),
                             size(QRectF(0, 0, width, height)), action(action)
{
    debug = info.device()->data("showtouch").toBool();
    setZValue(5);
}

RBTouchArea::~RBTouchArea()
{
}

QRectF RBTouchArea::boundingRect() const
{
    return size;
}

void RBTouchArea::paint(QPainter *painter,
                        const QStyleOptionGraphicsItem *option,
                        QWidget *widget)
{
    if(debug)
    {
        QColor fill = Qt::green;
        fill.setAlpha(50);
        painter->setBrush(fill);
        painter->setPen(Qt::NoPen);
        painter->drawRect(size);
    }
}
