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

#include "rbscreen.h"

#include <QPainter>
#include <QFile>

RBScreen::RBScreen(ProjectModel* project, QGraphicsItem *parent) :
    QGraphicsItem(parent), backdrop(0), project(project)
{

    width = safeSetting(project, "#screenwidth", "300").toInt();
    height = safeSetting(project, "#screenheight", "200").toInt();

    QString bg = safeSetting(project, "background color", "FFFFFF");
    bgColor = stringToColor(bg, Qt::white);

    QString fg = safeSetting(project, "foreground color", "FFFFFF");
    fgColor = stringToColor(fg, Qt::black);

    /* Loading backdrop if available */
    if(project)
    {
        QString base = project->getSetting("themebase", "");
        QString backdropFile = project->getSetting("backdrop", "");

        if(QFile::exists(base + "/backdrops/" + backdropFile))
        {
            backdrop = new QPixmap(base + "/backdrops/" + backdropFile);

            /* If a backdrop has been found, use its width and height */
            if(!backdrop->isNull())
            {
                width = backdrop->width();
                height = backdrop->height();
            }
            else
            {
                delete backdrop;
                backdrop = 0;
            }
        }
    }
}

RBScreen::~RBScreen()
{
    if(backdrop)
        delete backdrop;
}

QPainterPath RBScreen::shape() const
{
    QPainterPath retval;
    retval.addRect(0, 0, width, height);
    return retval;
}

QRectF RBScreen::boundingRect() const
{
    return QRectF(0, 0, width, height);
}

void RBScreen::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                     QWidget *widget)
{
    if(backdrop)
    {
        painter->drawPixmap(0, 0, width, height, *backdrop);
    }
    else
    {
        painter->fillRect(0, 0, width, height, bgColor);
    }
}

QColor RBScreen::stringToColor(QString str, QColor fallback)
{

    QColor retval;

    if(str.length() == 6)
    {
        for(int i = 0; i < 6; i++)
        {
            char c = str[i].toAscii();
            if(!((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') ||
                 isdigit(c)))
            {
                str = "";
                break;
            }
        }
        if(str != "")
            retval = QColor("#" + str);
        else
            retval = fallback;
    }
    else if(str.length() == 1)
    {
        if(isdigit(str[0].toAscii()) && str[0].toAscii() <= '3')
        {
            int shade = 255 * (str[0].toAscii() - '0') / 3;
            retval = QColor(shade, shade, shade);
        }
        else
        {
            retval = fallback;
        }
    }
    else
    {
        retval = fallback;
    }

    return retval;

}
