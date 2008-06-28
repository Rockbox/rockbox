/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Michael Sevakis
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

#ifndef GENERAL_H
#define GENERAL_H

#include <stdbool.h>
#include <stddef.h>

/* round a signed/unsigned 32bit value to the closest of a list of values */
/* returns the index of the closest value */
int round_value_to_list32(unsigned long value,
                          const unsigned long list[],
                          int count,
                          bool signd);

int make_list_from_caps32(unsigned long src_mask,
                          const unsigned long *src_list,
                          unsigned long caps_mask,
                          unsigned long *caps_list);

size_t align_buffer(void **start, size_t size, size_t align);

#endif /* GENERAL_H */
