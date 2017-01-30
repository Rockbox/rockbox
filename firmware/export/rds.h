/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2011 by Bertrik Sikken
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
#ifndef RDS_H
#define RDS_H

#include <stdint.h>
#include <stdbool.h>
#include "time.h"

void rds_init(void);
void rds_reset(void);

void rds_sync(void);
void rds_process(const uint16_t data[4]);

uint16_t rds_get_pi(void);
size_t rds_get_ps(char *dst, size_t dstsize);
size_t rds_get_rt(char *dst, size_t dstsize);
time_t rds_get_ct(void);

#endif /* RDS_H */
