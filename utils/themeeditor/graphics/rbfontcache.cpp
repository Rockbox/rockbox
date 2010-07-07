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

#include "rbfontcache.h"

QHash<QString, RBFontCache::CacheInfo*> RBFontCache::cache;

void RBFontCache::clearCache()
{
    QHash<QString, CacheInfo*>::iterator i;
    for(i = cache.begin(); i != cache.end(); i++)
    {
        CacheInfo* c = *i;
        if(c->imageData)
            delete c->imageData;

        if(c->offsetData)
            delete c->offsetData;

        if(c->widthData)
            delete c->widthData;

        delete c;
    }

    cache.clear();
}
