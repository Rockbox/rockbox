/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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

#ifndef MWDB_H
#define MWDB_H
#include <inttypes.h>


int mwdb_findarticle(const char* filename,
                     char* artnme,
                     uint16_t artnmelen,
                     uint32_t *res_lo,
                     uint32_t *res_hi,
                     bool progress,
                     bool promptname,
                     bool casesense
                     );

int mwdb_loadarticle(const char * filename,
                     void * scratchmem,
                     void * articlemem,
                     uint32_t articlememlen,
                     uint32_t *articlelen,
                     uint32_t res_lo,
                     uint32_t res_hi);

#endif
