/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 Amaury Pouly
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
#ifndef __ATJ_TABLES__
#define __ATJ_TABLES__

uint8_t g_check_block_A_table[1024];
uint8_t g_decode_B_table[20];
uint32_t g_crypto_table[8];
uint32_t g_crypto_table2[8];
uint32_t g_crypto_key6[8];
uint32_t g_crypto_key3[6];
uint32_t g_crypto_key4[6];
uint32_t g_crypto_key5[6];
uint32_t g_atj_ec233_a[8];
uint32_t g_atj_ec163_a[6];

#endif // __ATJ_TABLES__
