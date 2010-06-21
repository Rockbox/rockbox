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

#ifndef RBVIEWPORT_H
#define RBVIEWPORT_H

#include "skin_parser.h"

class RBScreen;
class RBRenderInfo;

#include <QGraphicsItem>

class RBViewport : public QGraphicsItem
{
public:
    RBViewport(skin_element* node, const RBRenderInfo& info);
    virtual ~RBViewport();

    QPainterPath shape() const;
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

    void show(){ displayed = true; }

private:
    QRectF size;
    QColor background;
    QColor foreground;

    bool displayed;
    bool customUI;

};

#endif // RBVIEWPORT_H
