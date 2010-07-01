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

#ifndef RBALBUMART_H
#define RBALBUMART_H

#include <QGraphicsItem>

class RBAlbumArt : public QGraphicsItem
{
public:
    RBAlbumArt(QGraphicsItem* parent, int x, int y, int maxWidth, int maxHeight,
               int artWidth, int artHeight, char hAlign = 'c',
               char vAlign = 'c');

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

    void position(){ this->setPos(size.x(), size.y()); }

private:
    QRectF size;
    int artWidth;
    int artHeight;
    char hAlign;
    char vAlign;
    QPixmap texture;
};

#endif // RBALBUMART_H
