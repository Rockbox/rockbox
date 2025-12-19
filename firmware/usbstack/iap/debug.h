/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025 by Sho Tanimoto
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
#pragma once

/* debug switches */
#define DEBUG_ENABLE_INFO     0
#define DEBUG_ENABLE_ERROR    1
#define DEBUG_LCD_PRINT       0
#define DEBUG_DUMP_TX         0
#define DEBUG_DUMP_RX         0
#define DEBUG_HEXDUMP_NOLIMIT 0

#if DEBUG_ENABLE_INFO == 1 || DEBUG_ENABLE_ERROR == 1
#define LOGF_ENABLE
#include "logf.h"
#endif

void          iap_lcd_scatter(const char* fmt, ...);
unsigned long iap_debug_timestamp(void);
void          iap_debug_reset_timestamp(void);

#if DEBUG_LCD_PRINT == 1
#undef logf
#define logf(...) iap_lcd_scatter(__VA_ARGS__)
#endif
