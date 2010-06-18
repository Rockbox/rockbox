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
#include <QPainterPath>

#include "rbviewport.h"
#include "rbscreen.h"
#include "rbrenderinfo.h"
#include "parsetreemodel.h"
#include "tag_table.h"
#include "skin_parser.h"

RBViewport::RBViewport(skin_element* node, const RBRenderInfo& info)
    : QGraphicsItem(info.screen())
{
    if(!node->tag)
    {
        /* Default viewport takes up the entire screen */
        size = QRectF(0, 0, info.screen()->getWidth(),
                      info.screen()->getHeight());

        if(info.model()->rowCount(QModelIndex()) > 1)
        {
            /* If there is more than one viewport in the document */
            displayed = false;
        }
        else
        {
            displayed = true;
        }
    }
    else
    {
        int x, y, w, h;
        /* Parsing one of the other types of viewport */
        switch(node->tag->name[1])
        {
        case '\0':
            /* A normal viewport definition */
            x = node->params[0].data.numeric;
            y = node->params[1].data.numeric;

            if(node->params[2].type == skin_tag_parameter::DEFAULT)
                w = info.screen()->getWidth() - x;
            else
                w = node->params[2].data.numeric;

            if(node->params[3].type == skin_tag_parameter::DEFAULT)
                h = info.screen()->getHeight() - y;
            else
                h = node->params[3].data.numeric;

            size = QRectF(x, y, w, h);
            displayed = true;
            break;

        case 'l':
            /* Preloaded viewport */
            break;

        case 'i':
            /* Custom UI Viewport */
            break;

        }
    }
}

RBViewport::~RBViewport()
{
}

QPainterPath RBViewport::shape() const
{
    QPainterPath retval;
    retval.addRect(size);
    return retval;
}

QRectF RBViewport::boundingRect() const
{
    return size;
}

void RBViewport::paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if(displayed)
        painter->fillRect(size, Qt::red);
}

