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
#include <QFile>

#include <QDebug>

RBFont::RBFont(QString file)
{

    /* Attempting to locate the correct file name */
    if(!QFile::exists(file))
        file = ":/fonts/08-Schumacher-Clean.fnt";
    header.insert("filename", file);

    /* Opening the file */
    QFile fin(file);
    fin.open(QFile::ReadOnly);

    /* Loading the header info */
    quint16 word;
    quint32 dword;

    QDataStream data(&fin);
    data.setByteOrder(QDataStream::LittleEndian);

    /* Grabbing the magic number and version */
    data >> dword;
    header.insert("version", dword);

    /* Max font width */
    data >> word;
    header.insert("maxwidth", word);

    /* Font height */
    data >> word;
    header.insert("height", word);

    /* Ascent */
    data >> word;
    header.insert("ascent", word);

    /* Padding */
    data >> word;

    /* First character code */
    data >> dword;
    header.insert("firstchar", dword);

    /* Default character code */
    data >> dword;
    header.insert("defaultchar", dword);

    /* Number of characters */
   data >> dword;
   header.insert("size", dword);

   /* Bytes of imagebits in file */
   data >> dword;
   header.insert("nbits", dword);

   /* Longs (dword) of offset data in file */
   data >> dword;
   header.insert("noffset", dword);

   /* Bytes of width data in file */
   data >> dword;
   header.insert("nwidth", dword);

    fin.close();

    qDebug() << header ;
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
