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

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QDebug>

#include <cmath>

#include "rbmovable.h"

const double RBMovable::handleSize = 7;

RBMovable::RBMovable(QGraphicsItem* parent)
    : QGraphicsItem(parent), dragMode(None)
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

        painter->fillRect(topLeftHandle(), Qt::green);
        painter->fillRect(topRightHandle(), Qt::green);
        painter->fillRect(bottomLeftHandle(), Qt::green);
        painter->fillRect(bottomRightHandle(), Qt::green);
    }
}

QVariant RBMovable::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if(change == ItemPositionChange)
    {
        QPointF pos = value.toPointF();
        QRectF bound = parentItem()->boundingRect();

        pos.setX(qMax(0., floor(pos.x())));
        pos.setX(qMin(pos.x(), bound.width() - boundingRect().width()));

        pos.setY(qMax(0., floor(pos.y())));
        pos.setY(qMin(pos.y(), bound.height() - boundingRect().height()));


        return pos;
    }

    return QGraphicsItem::itemChange(change, value);
}

void RBMovable::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if(!isSelected())
    {
        QGraphicsItem::mousePressEvent(event);
        return;
    }

    if(topLeftHandle().contains(event->pos()))
    {
        dragStartClick = event->pos() + pos();
        dragStartPos = pos();
        dragStartSize = boundingRect();

        dWidthMin = -1. * pos().x();
        dWidthMax = boundingRect().width() - childrenBoundingRect().right();

        dHeightMin = -1. * pos().y();
        dHeightMax = boundingRect().height() - childrenBoundingRect().bottom();

        dragMode = TopLeft;
    }
    else if(topRightHandle().contains(event->pos()))
    {
        dragStartClick = event->pos() + pos();
        dragStartPos = pos();
        dragStartSize = boundingRect();

        dWidthMin = childrenBoundingRect().width() - boundingRect().width();
        dWidthMin = qMax(dWidthMin, -1. * size.width());
        dWidthMax = parentItem()->boundingRect().width()
                    - boundingRect().width() - pos().x();

        dHeightMin = -1. * pos().y();
        dHeightMax = boundingRect().height() - childrenBoundingRect().bottom();
        dHeightMax = qMin(dHeightMax, boundingRect().height());

        dragMode = TopRight;
    }
    else if(bottomLeftHandle().contains(event->pos()))
    {
        dragStartClick = event->pos() + pos();
        dragStartPos = pos();
        dragStartSize = boundingRect();

        dWidthMin = -1. * pos().x();
        dWidthMax = boundingRect().width() - childrenBoundingRect().right();
        dWidthMax = qMin(dWidthMax, size.width());

        dHeightMin = -1. * (boundingRect().height()
                            - childrenBoundingRect().bottom());
        dHeightMin = qMax(dHeightMin, -1. * boundingRect().height());

        dragMode = BottomLeft;
    }
    else if(bottomRightHandle().contains(event->pos()))
    {
        dragStartClick = event->pos() + pos();
        dragStartPos = pos();
        dragStartSize = boundingRect();

        dWidthMin = -1. * (boundingRect().width()
                           - childrenBoundingRect().right());
        dWidthMin = qMax(dWidthMin, -1. * boundingRect().width());
        dWidthMax = parentItem()->boundingRect().width() -
                    boundingRect().width() - pos().x();

        dHeightMin = -1. * (boundingRect().height()
                            - childrenBoundingRect().bottom());
        dHeightMin = qMax(dHeightMin, -1. * boundingRect().height());
        dHeightMax = parentItem()->boundingRect().height() -
                     boundingRect().height() - pos().y();

        dragMode = BottomRight;
    }
    else
    {
        QGraphicsItem::mousePressEvent(event);
    }
}

void RBMovable::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{

    QPointF absPos;
    QPointF dPos;
    QPointF dMouse;
    switch(dragMode)
    {
    case None:
        QGraphicsItem::mouseMoveEvent(event);
        break;

    case TopLeft:
        /* Dragging from the top left corner */
        absPos = event->pos() + pos();
        dMouse = QPointF(floor(absPos.x() - dragStartClick.x()),
                         floor(absPos.y() - dragStartClick.y()));

        dPos.setX(qMin(dMouse.x(), dWidthMax));
        dPos.setX(qMax(dPos.x(), dWidthMin));

        dPos.setY(qMin(dMouse.y(), dHeightMax));
        dPos.setY(qMax(dPos.y(), dHeightMin));

        prepareGeometryChange();

        setPos(dragStartPos + dPos);

        size.setWidth(dragStartSize.width() - dPos.x());
        size.setHeight(dragStartSize.height() - dPos.y());

        break;

    case TopRight:
        /* Dragging from the top right corner */
        absPos = event->pos() + pos();
        dMouse = QPointF(floor(absPos.x() - dragStartClick.x()),
                         floor(absPos.y() - dragStartClick.y()));

        dPos.setX(qMin(dMouse.x(), dWidthMax));
        dPos.setX(qMax(dPos.x(), dWidthMin));

        dPos.setY(qMin(dMouse.y(), dHeightMax));
        dPos.setY(qMax(dPos.y(), dHeightMin));

        prepareGeometryChange();

        setPos(dragStartPos.x(), dragStartPos.y() + dPos.y());

        size.setWidth(dragStartSize.width() + dPos.x());
        size.setHeight(dragStartSize.height() - dPos.y());

        break;

    case BottomLeft:
        /* Dragging from the bottom left corner */
        absPos = event->pos() + pos();
        dMouse = QPointF(floor(absPos.x() - dragStartClick.x()),
                         floor(absPos.y() - dragStartClick.y()));

        dPos.setX(qMin(dMouse.x(), dWidthMax));
        dPos.setX(qMax(dPos.x(), dWidthMin));

        dPos.setY(qMin(dMouse.y(), dHeightMax));
        dPos.setY(qMax(dPos.y(), dHeightMin));

        prepareGeometryChange();
        setPos(dragStartPos.x() + dPos.x(), dragStartPos.y());
        size.setHeight(dragStartSize.height() + dPos.y());
        size.setWidth(dragStartSize.width() - dPos.x());

        break;

    case BottomRight:
        /* Dragging from the bottom right corner */
        absPos = event->pos() + pos();
        dMouse = QPointF(floor(absPos.x() - dragStartClick.x()),
                         floor(absPos.y() - dragStartClick.y()));

        dPos.setX(qMin(dMouse.x(), dWidthMax));
        dPos.setX(qMax(dPos.x(), dWidthMin));

        dPos.setY(qMin(dMouse.y(), dHeightMax));
        dPos.setY(qMax(dPos.y(), dHeightMin));

        prepareGeometryChange();

        size.setWidth(dragStartSize.width() + dPos.x());
        size.setHeight(dragStartSize.height() + dPos.y());

        break;
    }
}

void RBMovable::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseReleaseEvent(event);

    dragMode = None;

    if(isSelected())
    {
        saveGeometry();
    }
}

QRectF RBMovable::topLeftHandle()
{
    QRectF bounds = boundingRect();
    double width = qMin(bounds.width() / 2. - .5, handleSize);
    double height = qMin(bounds.height() / 2. - .5, handleSize);
    double size = qMin(width, height);

    return QRectF(0, 0, size, size);
}

QRectF RBMovable::topRightHandle()
{
    QRectF bounds = boundingRect();
    double width = qMin(bounds.width() / 2. - .5, handleSize);
    double height = qMin(bounds.height() / 2. - .5, handleSize);
    double size = qMin(width, height);

    return QRectF(bounds.width() - size, 0, size, size);
}

QRectF RBMovable::bottomLeftHandle()
{
    QRectF bounds = boundingRect();
    double width = qMin(bounds.width() / 2. - .5, handleSize);
    double height = qMin(bounds.height() / 2. - .5, handleSize);
    double size = qMin(width, height);

    return QRectF(0, bounds.height() - size, size, size);
}

QRectF RBMovable::bottomRightHandle()
{
    QRectF bounds = boundingRect();
    double width = qMin(bounds.width() / 2. - .5, handleSize);
    double height = qMin(bounds.height() / 2. - .5, handleSize);
    double size = qMin(width, height);

    return QRectF(bounds.width() - size, bounds.height() - size, size, size);
}
