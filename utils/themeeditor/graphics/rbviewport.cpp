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
#include <cmath>

#include "rbviewport.h"
#include "rbscreen.h"
#include "rbrenderinfo.h"
#include "parsetreemodel.h"
#include "tag_table.h"
#include "skin_parser.h"

/* Pixels/second of text scrolling */
const double RBViewport::scrollRate = 30;

RBViewport::RBViewport(skin_element* node, const RBRenderInfo& info)
    : QGraphicsItem(info.screen()), foreground(info.screen()->foreground()),
    background(info.screen()->background()), textOffset(0,0),
    screen(info.screen()), textAlign(Left), showStatusBar(false),
    statusBarTexture(":/render/statusbar.png"),
    leftGraphic(0), centerGraphic(0), rightGraphic(0), scrollTime(0)
{
    if(!node->tag)
    {
        /* Default viewport takes up the entire screen */
        size = QRectF(0, 0, info.screen()->getWidth(),
                      info.screen()->getHeight());
        customUI = false;
        font = screen->getFont(1);

        if(info.model()->rowCount(QModelIndex()) > 1)
        {
            /* If there is more than one viewport in the document */
            textOffset.setX(-1);
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
        if(x < 0)
            x = info.screen()->boundingRect().right() + x;
        y = node->params[param++].data.numeric;
        if(y < 0)
            y = info.screen()->boundingRect().bottom() + y;

        if(node->params[param].type == skin_tag_parameter::DEFAULT)
            w = info.screen()->getWidth() - x;
        else
            w = node->params[param].data.numeric;
        if(w < 0)
            w = info.screen()->getWidth() + w - x;

        if(node->params[++param].type == skin_tag_parameter::DEFAULT)
            h = info.screen()->getHeight() - y;
        else
            h = node->params[param].data.numeric;
        if(h < 0)
            h = info.screen()->getHeight() + h - y;

        /* Adjusting to screen coordinates if necessary */
        if(screen->parentItem() != 0)
        {
            x -= screen->parentItem()->pos().x();
            y -= screen->parentItem()->pos().y();
        }

        if(node->params[++param].type == skin_tag_parameter::DEFAULT)
            font = screen->getFont(1);
        else
            font = screen->getFont(node->params[param].data.numeric);

        setPos(x, y);
        size = QRectF(0, 0, w, h);
    }

    debug = info.device()->data("showviewports").toBool();
    lineHeight = font->lineHeight();
    if(customUI)
        screen->setCustomUI(this);
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

    if(showStatusBar)
        painter->fillRect(QRectF(0, 0, size.width(), 8), statusBarTexture);
}

void RBViewport::newLine()
{
    if(textOffset.x() < 0)
        return;

    if(leftText != "")
        alignLeft();

    if(centerText != "")
        alignCenter();

    if(rightText != "")
        alignRight();

    textOffset.setY(textOffset.y() + lineHeight);
    textOffset.setX(0);
    textAlign = Left;

    leftText.clear();
    rightText.clear();
    centerText.clear();

    leftGraphic = 0;
    centerGraphic = 0;
    rightGraphic = 0;

    scrollTime = 0;
}

void RBViewport::write(QString text)
{
    if(textOffset.x() < 0)
        return;

    if(textAlign == Left)
    {
        leftText.append(text);
    }
    else if(textAlign == Center)
    {
        centerText.append(text);
    }
    else if(textAlign == Right)
    {
        rightText.append(text);
    }
}

void RBViewport::showPlaylist(const RBRenderInfo &info, int start,
                              skin_element *id3, skin_element *noId3)
{
    /* Determining whether ID3 info is available */
    skin_element* root = id3;

    /* The line will be a linked list */
    if(root->children_count > 0)
        root = root->children[0];

    int song = start + info.device()->data("pp").toInt();
    int numSongs = info.device()->data("pe").toInt();
    int halfWay = (numSongs - song) / 2 + 1 + song;

    while(song <= numSongs && textOffset.y() + lineHeight < size.height())
    {
        if(song == halfWay)
        {
            root = noId3;
            if(root->children_count > 0)
                root = root->children[0];
        }
        skin_element* current = root;
        while(current)
        {

            if(current->type == TEXT)
            {
                write(QString((char*)current->data));
            }

            if(current->type == TAG)
            {
                QString tag(current->tag->name);
                if(tag == "pp")
                {
                    write(QString::number(song));
                }
                else if(tag == "pt")
                {
                    write(QObject::tr("00:00"));
                }
                else if(tag[0] == 'i' || tag[0] == 'f')
                {
                    if(song == info.device()->data("pp").toInt())
                    {
                        write(info.device()->data(tag).toString());
                    }
                    else
                    {
                        /* If we're not on the current track, use the next
                         * track info
                         */
                        if(tag[0] == 'i')
                            tag = QString("I") + tag.right(1);
                        else
                            tag = QString("F") + tag.right(1);
                        write(info.device()->data(tag).toString());
                    }
                }
            }

            current = current->next;
        }
        newLine();
        song++;
    }
}

void RBViewport::alignLeft()
{
    int y = textOffset.y();

    if(leftGraphic)
        delete leftGraphic;

    leftGraphic = font->renderText(leftText, foreground, size.width(), this);
    leftGraphic->setPos(0, y);

    /* Setting scroll position if necessary */
    int difference = leftGraphic->realWidth()
                     - leftGraphic->boundingRect().width();
    if(difference > 0)
    {
        /* Subtracting out complete cycles */
        double totalTime = 2 * difference / scrollRate;
        scrollTime -= totalTime * std::floor(scrollTime / totalTime);

        /* Calculating the offset */
        if(scrollTime < static_cast<double>(difference) / scrollRate)
        {
            leftGraphic->setOffset(scrollRate * scrollTime);
        }
        else
        {
            scrollTime -= static_cast<double>(difference) / scrollRate;
            leftGraphic->setOffset(difference - scrollRate * scrollTime);
        }
    }
}

void RBViewport::alignCenter()
{
    int y = textOffset.y();
    int x = 0;

    if(centerGraphic)
        delete centerGraphic;

    centerGraphic = font->renderText(centerText, foreground, size.width(),
                                     this);

    if(centerGraphic->boundingRect().width() < size.width())
    {
        x = size.width() - centerGraphic->boundingRect().width();
        x /= 2;
    }
    else
    {
        x = 0;
    }

    centerGraphic->setPos(x, y);

    /* Setting scroll position if necessary */
    int difference = centerGraphic->realWidth()
                     - centerGraphic->boundingRect().width();
    if(difference > 0)
    {
        /* Subtracting out complete cycles */
        double totalTime = 2 * difference / scrollRate;
        scrollTime -= totalTime * std::floor(scrollTime / totalTime);

        /* Calculating the offset */
        if(scrollTime < static_cast<double>(difference) / scrollRate)
        {
            centerGraphic->setOffset(scrollRate * scrollTime);
        }
        else
        {
            scrollTime -= static_cast<double>(difference) / scrollRate;
            centerGraphic->setOffset(difference - scrollRate * scrollTime);
        }
    }

}

void RBViewport::alignRight()
{
    int y = textOffset.y();
    int x = 0;

    if(rightGraphic)
        delete rightGraphic;

    rightGraphic = font->renderText(rightText, foreground, size.width(), this);

    if(rightGraphic->boundingRect().width() < size.width())
        x = size.width() - rightGraphic->boundingRect().width();
    else
        x = 0;

    rightGraphic->setPos(x, y);

    /* Setting scroll position if necessary */
    int difference = rightGraphic->realWidth()
                     - rightGraphic->boundingRect().width();
    if(difference > 0)
    {
        /* Subtracting out complete cycles */
        double totalTime = 2 * difference / scrollRate;
        scrollTime -= totalTime * std::floor(scrollTime / totalTime);

        /* Calculating the offset */
        if(scrollTime < static_cast<double>(difference) / scrollRate)
        {
            rightGraphic->setOffset(scrollRate * scrollTime);
        }
        else
        {
            scrollTime -= static_cast<double>(difference) / scrollRate;
            rightGraphic->setOffset(difference - scrollRate * scrollTime);
        }
    }
}

