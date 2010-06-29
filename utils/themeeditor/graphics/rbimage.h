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

#ifndef RBIMAGE_H
#define RBIMAGE_H

#include <QPixmap>
#include <QGraphicsItem>

class RBImage: public QGraphicsItem
{
public:
    RBImage(QString file, int tiles, int x, int y, QGraphicsItem* parent = 0);
    RBImage(const RBImage& other, QGraphicsItem* parent);
    virtual ~RBImage();

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

    void setTile(int tile)
    {
        currentTile = tile;
        if(currentTile > tiles - 1)
            currentTile = tiles -1;
    }


private:
    QPixmap* image;
    int tiles;
    int currentTile;

    QRectF size;

};

#endif // RBIMAGE_H
