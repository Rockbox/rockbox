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
    : QGraphicsItem(info.screen()), font(info.screen()->getFont(0)),
    foreground(info.screen()->foreground()),
    background(info.screen()->background()), textOffset(0,0),
    screen(info.screen())
{
    if(!node->tag)
    {
        /* Default viewport takes up the entire screen */
        size = QRectF(0, 0, info.screen()->getWidth(),
                      info.screen()->getHeight());
        customUI = false;

        if(info.model()->rowCount(QModelIndex()) > 1)
        {
            /* If there is more than one viewport in the document */
            setVisible(false);
        }
        else
        {
            setVisible(true);
        }
    }
    else
    {
        int param = 0;
        QString ident;
        int x,y,w,h;
        /* Rendering one of the other types of viewport */
        switch(node->tag->name[1])
        {
        case '\0':
            customUI = false;
            param = 0;
            break;

        case 'l':
            /* A preloaded viewport definition */
            ident = node->params[0].data.text;
            customUI = false;
            if(!screen->viewPortDisplayed(ident))
                hide();
            info.screen()->loadViewport(ident, this);
            param = 1;
            break;

        case 'i':
            /* Custom UI Viewport */
            customUI = true;
            param = 1;
            if(node->params[0].type == skin_tag_parameter::DEFAULT)
            {
                setVisible(true);
            }
            else
            {
                hide();
                info.screen()->loadViewport(ident, this);
            }
            break;
        }
        /* Now we grab the info common to all viewports */
        x = node->params[param++].data.numeric;
        y = node->params[param++].data.numeric;

        if(node->params[param].type == skin_tag_parameter::DEFAULT)
            w = info.screen()->getWidth() - x;
        else
            w = node->params[param].data.numeric;

        if(node->params[++param].type == skin_tag_parameter::DEFAULT)
            h = info.screen()->getHeight() - y;
        else
            h = node->params[param].data.numeric;

        setPos(x, y);
        size = QRectF(0, 0, w, h);
        debug = info.device()->data("showviewports").toBool();
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
    if(!screen->hasBackdrop() && background != screen->background())
    {
        painter->fillRect(size, QBrush(background));
    }

    painter->setBrush(Qt::NoBrush);
    painter->setPen(customUI ? Qt::blue : Qt::red);
    if(debug)
        painter->drawRect(size);
}

void RBViewport::newLine()
{
    if(textOffset.x() > 0)
    {
        textOffset.setY(textOffset.y() + lineHeight);
        textOffset.setX(0);
    }
}

void RBViewport::write(QString text)
{
    QGraphicsItem* graphic = font->renderText(text, foreground, this);
    graphic->setPos(textOffset.x(), textOffset.y());
    textOffset.setX(textOffset.x() + graphic->boundingRect().width());
    lineHeight = font->lineHeight();
}
