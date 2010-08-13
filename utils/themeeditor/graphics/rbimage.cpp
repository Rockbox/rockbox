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
#include <QFile>
#include <QBitmap>

#include "rbimage.h"
#include "parsetreenode.h"
#include <rbscene.h>

RBImage::RBImage(QString file, int tiles, int x, int y, ParseTreeNode* node,
                 QGraphicsItem* parent)
                     : RBMovable(parent), tiles(tiles), currentTile(0),
                     node(node)
{
    /* Prevents RBMovable from interfering with initial position setting */
    setFlag(ItemSendsGeometryChanges, false);

    if(QFile::exists(file))
    {
        image = new QPixmap(file);

        if(image->isNull())
        {
            delete image;
            image = 0;
            return;
        }
        else
        {
            image->setMask(image->createMaskFromColor(QColor(255,0,255)));

        }

        size = QRectF(0, 0, image->width(), image->height() / tiles);
        setPos(x, y);

    }
    else
    {
        RBScene* s = dynamic_cast<RBScene*>(scene());
        s->addWarning(QObject::tr("Image not found: ") + file);

        size = QRectF(0, 0, 0, 0);
        image = 0;
    }
}

RBImage::RBImage(const RBImage &other, QGraphicsItem* parent)
    : RBMovable(parent), tiles(other.tiles), currentTile(other.currentTile),
    node(other.node)
{
    if(other.image)
        image = new QPixmap(*(other.image));
    else
        image = 0;
    size = other.size;
    setPos(other.x(), other.y());
}

RBImage::~RBImage()
{
    if(image)
        delete image;
}

void RBImage::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                    QWidget *widget)
{
    if(!image)
        return;

    painter->drawPixmap(size, *image, QRect(0, currentTile * image->height()
                                            / tiles, image->width(),
                                            image->height() / tiles));

    RBMovable::paint(painter, option, widget);
}



void RBImage::saveGeometry()
{
    QPointF origin = pos();

    node->modParam(static_cast<int>(origin.x()), 2);
    node->modParam(static_cast<int>(origin.y()), 3);
}
