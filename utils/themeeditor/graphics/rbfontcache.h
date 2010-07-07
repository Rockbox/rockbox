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

#ifndef RBFONTCACHE_H
#define RBFONTCACHE_H

#include <QHash>
#include <QVariant>

class RBFontCache
{

public:
    struct CacheInfo
    {
        quint8* imageData;
        quint16* offsetData;
        quint8* widthData;

        QHash<QString, QVariant> header;
    };

    static CacheInfo* lookup(QString key){ return cache.value(key, 0); }
    static void insert(QString key, CacheInfo* data){ cache.insert(key, data); }
    static void clearCache();

private:
    static QHash<QString, CacheInfo*> cache;

};

#endif // RBFONTCACHE_H
