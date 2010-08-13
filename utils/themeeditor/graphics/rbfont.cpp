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
#include "rbfontcache.h"
#include "rbtextcache.h"

#include <QFont>
#include <QBrush>
#include <QFile>
#include <QPainter>
#include <QBitmap>
#include <QImage>
#include <QSettings>

#include <QDebug>

quint16 RBFont::maxFontSizeFor16BitOffsets = 0xFFDB;

RBFont::RBFont(QString file)
    : valid(false), imageData(0), offsetData(0), widthData(0)
{

    bool badFile = false;

    /* Attempting to locate the correct file name */
    if(!QFile::exists(file))
    {
        /* Checking in the fonts repository */
        QSettings settings;
        settings.beginGroup("RBFont");

        file = file.split("/").last();
        file = settings.value("fontDir", "").toString() + "/" + file;

        settings.endGroup();

        if(!QFile::exists(file))
        {
            file = ":/fonts/08-Schumacher-Clean.fnt";

            badFile = true;
        }
    }
    header.insert("filename", file);

    /* Checking for a cache entry */
    RBFontCache::CacheInfo* cache = RBFontCache::lookup(file);
    if(cache)
    {
        imageData = cache->imageData;
        offsetData = cache->offsetData;
        widthData = cache->widthData;
        header = cache->header;

        if(!badFile)
            valid = true;

        return;
    }

    /* Opening the file */
    QFile fin(file);
    fin.open(QFile::ReadOnly);

    /* Loading the header info */
    quint8 byte;
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

   /* Loading the image data */
   imageData = new quint8[header.value("nbits").toInt()];
   data.readRawData(reinterpret_cast<char*>(imageData),
                    header.value("nbits").toInt());

   /* Aligning on 16-bit boundary */
   if(header.value("nbits").toInt() % 2 == 1)
       data >> byte;

   /* Loading the offset table if necessary */
   if(header.value("noffset").toInt() > 0)
   {
       int bytesToRead;
       if(header.value("nbits").toInt() > maxFontSizeFor16BitOffsets)
           bytesToRead = 4 * header.value("noffset").toInt();
       else
           bytesToRead = 2 * header.value("noffset").toInt();
       offsetData = new quint16[bytesToRead];
       data.readRawData(reinterpret_cast<char*>(offsetData), bytesToRead);
   }

   /* Loading the width table if necessary */
   if(header.value("nwidth").toInt() > 0)
   {
       widthData = new quint8[header.value("nwidth").toInt()];
       data.readRawData(reinterpret_cast<char*>(widthData),
                        header.value("nwidth").toInt());
   }

   fin.close();

   /* Caching the font data */
   cache = new RBFontCache::CacheInfo;
   cache->imageData = imageData;
   cache->offsetData = offsetData;
   cache->widthData = widthData;
   cache->header = header;
   RBFontCache::insert(file, cache);

   if(!badFile)
       valid = true;

}

RBFont::~RBFont()
{
}

RBText* RBFont::renderText(QString text, QColor color, int viewWidth,
                           QGraphicsItem *parent)
{

    /* Checking for a cache hit first */
    QImage* image = RBTextCache::lookup(header.value("filename").toString()
                                        + text);
    if(image)
        return new RBText(image, viewWidth, parent);

    int firstChar = header.value("firstchar").toInt();
    int height = header.value("height").toInt();
    int maxWidth = header.value("maxwidth").toInt();

    bool extendedSet = header.value("nbits").
                       toUInt() > maxFontSizeFor16BitOffsets;

    /* First we determine the width of the combined text */
    QList<int> widths;
    for(int i = 0; i < text.length(); i++)
    {
        if(widthData)
            widths.append(widthData[text[i].unicode() - firstChar]);
        else
            widths.append(maxWidth);
    }

    int totalWidth = 0;
    for(int i = 0; i < widths.count(); i++)
        totalWidth += widths[i];

    image = new QImage(totalWidth, height, QImage::Format_Indexed8);

    image->setColor(0, qRgba(0,0,0,0));
    image->setColor(1, color.rgb());

    /* Drawing the text */
    int startX = 0;
    for(int i = 0; i < text.length(); i++)
    {
        unsigned int offset;
        if(offsetData)
        {
            if(extendedSet)
                offset = reinterpret_cast<quint32*>(offsetData)[text[i].unicode() - firstChar];
            else
                offset = offsetData[text[i].unicode() - firstChar];
        }
        else
        {
            offset = (text[i].unicode() - firstChar) * maxWidth;
        }

        int bytesHigh = height / 8;
        if(height % 8 > 0)
            bytesHigh++;

        int bytes = bytesHigh * widths[i];

        for(int byte = 0; byte < bytes; byte++)
        {
            int x = startX + byte % widths[i];
            int y = byte / widths[i] * 8;
            quint8 data = imageData[offset];
            quint8 mask = 0x1;
            for(int bit = 0; bit < 8; bit++)
            {
                if(mask & data)
                    image->setPixel(x, y, 1);
                else
                    image->setPixel(x, y, 0);

                y++;
                mask <<= 1;
                if(y >= height)
                    break;
            }

            offset++;
        }

        startX += widths[i];
    }

    RBTextCache::insert(header.value("filename").toString() + text, image);
    return new RBText(image, viewWidth, parent);

}
