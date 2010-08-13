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

#ifndef RBFONT_H
#define RBFONT_H

#include <QString>
#include <QFile>
#include <QGraphicsPixmapItem>
#include <QHash>

#include "rbtext.h"

class RBFont
{
public:
    RBFont(QString file);
    virtual ~RBFont();

    RBText* renderText(QString text, QColor color, int maxWidth,
                                        QGraphicsItem* parent = 0);
    int lineHeight(){ return header.value("height", 0).toInt(); }

    static quint16 maxFontSizeFor16BitOffsets;

    bool isValid(){ return valid; }

private:
    QHash<QString, QVariant> header;
    bool valid;
    quint8* imageData;
    quint16* offsetData;
    quint8* widthData;
};

#endif // RBFONT_H
