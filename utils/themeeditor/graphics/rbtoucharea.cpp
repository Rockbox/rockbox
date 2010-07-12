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
#include <QDebug>
#include <QGraphicsSceneMouseEvent>

RBTouchArea::RBTouchArea(int width, int height, QString action,
                         const RBRenderInfo& info)
                             : QGraphicsItem(info.screen()),
                             size(QRectF(0, 0, width, height)), action(action),
                             device(info.device())
{
    debug = info.device()->data("showtouch").toBool();
    setZValue(5);
    setCursor(Qt::PointingHandCursor);
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

void RBTouchArea::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if(action[0] == '&')
        action = action.right(action.count() - 1);

    action = action.toLower();

    if(action == "play")
    {
        if(device->data("?mp").toInt() == 2)
            device->setData("mp", "Play");
        else
            device->setData("mp", "Pause");
    }
    else if(action == "ffwd")
    {
        device->setData("mp", "Fast Forward");
    }
    else if(action == "rwd")
    {
        device->setData("mp", "Rewind");
    }
    else if(action == "repmode")
    {
        int index = device->data("?mm").toInt();
        index = (index + 1) % 5;
        device->setData("mm", index);
    }
    else if(action == "shuffle")
    {
        device->setData("ps", !device->data("ps").toBool());
    }
    else if(action == "volup")
    {
        device->setData("pv", device->data("pv").toInt() + 1);
    }
    else if(action == "voldown")
    {
        device->setData("pv", device->data("pv").toInt() - 1);
    }
    else
    {
        event->ignore();
        return;
    }

    event->accept();
}
