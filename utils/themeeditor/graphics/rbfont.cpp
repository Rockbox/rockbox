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

#include "rbfont.h"

#include <QFont>
#include <QBrush>

RBFont::RBFont(QString file): filename(file)
{
}

RBFont::~RBFont()
{
}

QGraphicsSimpleTextItem* RBFont::renderText(QString text, QColor color,
                                            QGraphicsItem *parent)
{
    QGraphicsSimpleTextItem* retval = new QGraphicsSimpleTextItem(text, parent);
    QFont font;
    font.setFixedPitch(true);
    font.setPixelSize(8);
    retval->setFont(font);
    retval->setBrush(QBrush(color));
    return retval;
}
