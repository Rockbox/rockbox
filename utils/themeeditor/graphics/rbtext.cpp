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

#include "rbtext.h"

#include <QPainter>

RBText::RBText(QImage* image, int maxWidth, QGraphicsItem *parent)
    :QGraphicsItem(parent), image(image), maxWidth(maxWidth), offset(0)
{
}

QRectF RBText::boundingRect() const
{
    if(image->width() < maxWidth)
        return QRectF(0, 0, image->width(), image->height());
    else
        return QRectF(0, 0, maxWidth, image->height());
}

void RBText::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                   QWidget *widget)
{
    /* Making sure the offset is within bounds */
    if(image->width() > maxWidth)
        if(offset > image->width() - maxWidth)
            offset = image->width() - maxWidth;

    if(image->width() < maxWidth)
        painter->drawImage(0, 0, *image, 0, 0, image->width(), image->height());
    else
        painter->drawImage(0, 0, *image, offset, 0, maxWidth, image->height());
}
