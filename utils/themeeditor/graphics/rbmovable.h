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

#ifndef RBMOVABLE_H
#define RBMOVABLE_H

#include <QGraphicsItem>

/*
 * This is a base class for scene elements that can be moved around and
 * resized. It adds some basic functionality for showing a border around
 * selected items and ensuring that they don't get moved out of their parent's
 * bounding rect, as well as resizing them. It includes one pure virtual
 * function, saveGeometry(), that is responsible for syncing the changed
 * geometry back to the parse tree to be code gen'd into the file.
 */

class RBMovable : public QGraphicsItem
{
public:
    RBMovable(QGraphicsItem* parent);
    ~RBMovable();

    virtual void paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *option, QWidget *widget);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    virtual QRectF boundingRect() const{ return size; }

protected:
    virtual QVariant itemChange(GraphicsItemChange change,
                                const QVariant &value);
    /* Responsible for updating the parse tree */
    virtual void saveGeometry() = 0;

    QRectF size;

private:
    static const double handleSize;

    QRectF topLeftHandle();
    QRectF topRightHandle();
    QRectF bottomLeftHandle();
    QRectF bottomRightHandle();

    enum{
        None,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight
    } dragMode;
    QPointF dragStartPos;
    QRectF dragStartSize;
    QPointF dragStartClick;
    double dHeightMin;
    double dHeightMax;
    double dWidthMin;
    double dWidthMax;

};

#endif // RBMOVABLE_H
