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

#include <QPainter>
#include <QDebug>

#include "rbmovable.h"

RBMovable::RBMovable(QGraphicsItem* parent)
    : QGraphicsItem(parent)
{
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
}

RBMovable::~RBMovable()
{
}

void RBMovable::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                      QWidget *widget)
{
    if(isSelected())
    {
        painter->setBrush(Qt::NoBrush);
        QPen pen;
        pen.setStyle(Qt::DashLine);
        pen.setColor(Qt::green);
        painter->setPen(pen);
        painter->drawRect(boundingRect());
    }
}

QVariant RBMovable::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if(change == ItemPositionChange)
    {
        QPointF pos = value.toPointF();
        QRectF bound = parentItem()->boundingRect();

        pos.setX(qMax(0., pos.x()));
        pos.setX(qMin(pos.x(), bound.width() - boundingRect().width()));

        pos.setY(qMax(0., pos.y()));
        pos.setY(qMin(pos.y(), bound.height() - boundingRect().height()));

        saveGeometry();

        return pos;
    }

    return QGraphicsItem::itemChange(change, value);
}

