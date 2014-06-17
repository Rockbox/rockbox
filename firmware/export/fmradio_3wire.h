/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * Physical interface of the Philips TEA5767
 *
 * Copyright (C) 2014 by Szymon Dziok
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
#ifndef FMRADIO_TRIWIRE_H
#define FMRADIO_TRIWIRE_H

#include "config.h" /* for INIT_ATTR */
#include <stdbool.h>

void fmradio_3wire_init(void) INIT_ATTR;
void fmradio_3wire_enable(bool enable);
int fmradio_3wire_write(const unsigned char* buf);
int fmradio_3wire_read(unsigned char* buf);

#endif
