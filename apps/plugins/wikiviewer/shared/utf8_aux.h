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

#ifndef __UTF8_AUX_H_
#define __UTF8_AUX_H_

#include <stdint.h>
#include <stdbool.h>

const unsigned char* utf8decode(const unsigned char *utf8, unsigned short *ucs);
char utf8strcnmp(const unsigned char *s1, const unsigned char *s2, uint16_t n1,
                 uint16_t n2, const bool casesense);

#endif /* __UTF8_AUX_H_ */
