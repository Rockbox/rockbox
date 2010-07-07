/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: rbfont.cpp 27301 2010-07-05 22:15:17Z bieber $
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

#ifndef RBTEXT_H
#define RBTEXT_H

#include <QGraphicsItem>
#include <QImage>

class RBText : public QGraphicsItem
{
public:
    RBText(QImage* image, int maxWidth, QGraphicsItem* parent);

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

    int realWidth(){ return image->width(); }
    void setOffset(int offset){ this->offset = offset; }

private:
    QImage* image;
    int maxWidth;
    int offset;

};

#endif // RBTEXT_H
