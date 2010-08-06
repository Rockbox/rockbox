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

#include "parsetreenode.h"
#include "rbprogressbar.h"
#include "projectmodel.h"

RBProgressBar::RBProgressBar(RBViewport *parent, const RBRenderInfo &info,
                             ParseTreeNode* node, bool pv)
                                 :RBMovable(parent), node(node)
{
    int paramCount = node->getElement()->params_count;
    skin_tag_parameter* params = node->getElement()->params;

    /* First we set everything to defaults */
    bitmap = 0;
    color = parent->getFGColor();
    int x = 0;
    int y = parent->getTextOffset();
    int w = parent->boundingRect().width();
    int h = 6;

    /* Now we change defaults if the parameters are there */

    if(paramCount > 0 && params[0].type != skin_tag_parameter::DEFAULT)
    {
        x = params[0].data.number;
    }

    if(paramCount > 1 && params[1].type != skin_tag_parameter::DEFAULT)
    {
        y = params[1].data.number;
    }

    if(paramCount > 2 && params[2].type != skin_tag_parameter::DEFAULT)
    {
        w = params[2].data.number;
    }

    if(paramCount > 3 && params[3].type != skin_tag_parameter::DEFAULT)
    {
        h = params[3].data.number;
    }

    if(paramCount > 4 && params[4].type != skin_tag_parameter::DEFAULT)
    {
        QString imPath(params[4].data.text);
        imPath = info.settings()->value("imagepath", "") + "/" + imPath;
        bitmap = new QPixmap(imPath);
        if(bitmap->isNull())
        {
            delete bitmap;
            bitmap = 0;
        }
    }
    size = QRectF(0, 0, w, h);

    /* Finally, we scale the width according to the amount played */
    int percent;
    if(pv)
    {
        percent = (info.device()->data("pv").toInt() + 50) * 100 / 56;
    }
    else
    {
        percent = info.device()->data("px").toInt();
    }
    if(percent > 100)
        percent = 100;
    if(percent < 0)
        percent = 0;

    w = w * percent / 100;

    renderSize = QRectF(0, 0, w, h);
    setPos(x, y);
    parent->addTextOffset(h);
}

RBProgressBar::~RBProgressBar()
{
    if(bitmap)
        delete bitmap;
}

void RBProgressBar::paint(QPainter *painter,
                           const QStyleOptionGraphicsItem *option,
                           QWidget *widget)
{
    renderSize.setHeight(size.height());

    if(bitmap && !bitmap->isNull())
    {
        painter->drawPixmap(renderSize, *bitmap, renderSize);
    }
    else
    {
        painter->fillRect(renderSize, color);
    }

    RBMovable::paint(painter, option, widget);
}

void RBProgressBar::saveGeometry()
{
    QPointF origin = pos();
    QRectF bounds = boundingRect();

    node->modParam(static_cast<int>(origin.x()), 0);
    node->modParam(static_cast<int>(origin.y()), 1);
    node->modParam(static_cast<int>(bounds.width()), 2);
    node->modParam(static_cast<int>(bounds.height()), 3);

    if(!bitmap)
        node->modParam(QVariant(), 4);
}
