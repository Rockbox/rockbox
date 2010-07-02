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

#ifndef RBPROGRESSBAR_H
#define RBPROGRESSBAR_H

#include <QGraphicsItem>
#include <QPixmap>

#include "rbrenderinfo.h"
#include "rbviewport.h"
#include "devicestate.h"
#include "skin_parser.h"

class RBProgressBar : public QGraphicsItem
{
public:
    RBProgressBar(RBViewport* parent, const RBRenderInfo& info,
                  int paramCount, skin_tag_parameter* params);
    virtual ~RBProgressBar();

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

private:
    QPixmap* bitmap;
    QColor color;
    QRectF size;

};

#endif // RBPROGRESSBAR_H
