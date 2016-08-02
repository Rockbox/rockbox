/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
#ifndef __TARGET_H__
#define __TARGET_H__

#include "protocol.h"

/* do target specific init */
void target_init(void);
/* get descriptor, set buffer to NULL on error */
void target_get_desc(int desc, void **buffer);
/* pack all descriptors for config desc */
void target_get_config_desc(void *buffer, int *size);
/* Wait a very short time (us<=1000) */
void target_udelay(int us);
/* Wait for a short time (ms <= 1000) */
void target_mdelay(int ms);
/* Read a n-bit word atomically */
uint8_t target_read8(const void *addr);
uint16_t target_read16(const void *addr);
uint32_t target_read32(const void *addr);
/* Write a n-bit word atomically */
void target_write8(void *addr, uint8_t val);
void target_write16(void *addr, uint16_t val);
void target_write32(void *addr, uint32_t val);
#ifdef CONFIG_FLUSH_CACHES
/* flush cache: commit dcache and invalidate icache */
void target_flush_caches(void);
#endif

/* mandatory for all targets */
extern struct hwstub_target_desc_t target_descriptor;

#endif /* __TARGET_H__ */
