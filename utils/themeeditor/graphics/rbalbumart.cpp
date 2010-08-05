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

#include "rbalbumart.h"

#include <QPainter>

#include "parsetreenode.h"

RBAlbumArt::RBAlbumArt(QGraphicsItem *parent, int x, int y, int maxWidth,
                       int maxHeight, int artWidth, int artHeight,
                       ParseTreeNode* node, char hAlign, char vAlign)
                           : RBMovable(parent),artWidth(artWidth),
                           artHeight(artHeight), hAlign(hAlign), vAlign(vAlign),
                           texture(":/render/albumart.png"), node(node)
{
    size = QRectF(0, 0, maxWidth, maxHeight);
    setFlag(ItemSendsGeometryChanges, false);

    setPos(x, y);
    hide();
}

void RBAlbumArt::paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QRectF drawArea;

    /* Making sure the alignment flags are sane */
    if(hAlign != 'c' && hAlign != 'l' && hAlign != 'r')
        hAlign = 'c';
    if(vAlign != 'c' && vAlign != 't' && vAlign != 'b')
        vAlign = 'c';

    if(artWidth <= size.width() && artHeight <= size.height())
    {
        /* If the art is smaller than the viewport, just center it up */
        drawArea.setX((size.width() - artWidth) / 2);
        drawArea.setY((size.height() - artHeight) / 2);
        drawArea.setWidth(artWidth);
        drawArea.setHeight(artHeight);
    }
    else
    {
        /* Otherwise, figure out our scale factor, and which dimension needs
         * to be scaled, and how to align said dimension
         */
        double xScale = size.width() / artWidth;
        double yScale = size.height() / artHeight;
        double scale = xScale < yScale ? xScale : yScale;

        double scaleWidth = artWidth * scale;
        double scaleHeight = artHeight * scale;

        if(hAlign == 'l')
            drawArea.setX(0);
        else if(hAlign == 'c')
            drawArea.setX((size.width() - scaleWidth) / 2 );
        else
            drawArea.setX(size.width() - scaleWidth);

        if(vAlign == 't')
            drawArea.setY(0);
        else if(vAlign == 'c')
            drawArea.setY((size.height() - scaleHeight) / 2);
        else
            drawArea.setY(size.height() - scaleHeight);

        drawArea.setWidth(scaleWidth);
        drawArea.setHeight(scaleHeight);

    }

    painter->fillRect(drawArea, texture);

    RBMovable::paint(painter, option, widget);
}

void RBAlbumArt::saveGeometry()
{

    QPointF origin = pos();
    QRectF bounds = boundingRect();

    node->modParam(static_cast<int>(origin.x()), 0);
    node->modParam(static_cast<int>(origin.y()), 1);
    node->modParam(static_cast<int>(bounds.width()), 2);
    node->modParam(static_cast<int>(bounds.height()), 3);
}
