/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Franklin Wei, Benjamin Brown
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#ifndef __INTERN_H__
#define __INTERN_H__

#include "plugin.h"
#include "string.h"
#include "awendian.h"
#include "util.h"

#define assert(c) (c?(void)0:error("Assertion failed line %d, file %s", __LINE__, __FILE__))

struct Ptr {
    uint8_t* pc;
};

struct Point {
    int16_t x, y;
};

static inline uint8_t scriptPtr_fetchByte(struct Ptr* p) {
    return *p->pc++;
}

static inline uint16_t scriptPtr_fetchWord(struct Ptr* p) {
    uint16_t i = READ_BE_UINT16(p->pc);
    p->pc += 2;
    return i;
}

#endif
