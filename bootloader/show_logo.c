/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Barry Wardell
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
#include "lcd.h"
#include "lcd-remote.h"
#include "font.h"
#include <stdio.h>
#include <string.h>

#include "bitmaps/rockboxlogo.h"

#if LCD_WIDTH <= 128
#define BOOT_VERSION ("Boot " APPSVERSION)
#else
#define BOOT_VERSION ("Boot Ver. " APPSVERSION)
#endif

/* Ensure TEXT_XPOS is >= 0 */
#define TEXT_WIDTH ((sizeof(BOOT_VERSION)-1)*SYSFONT_WIDTH)
#define TEXT_XPOS ((TEXT_WIDTH > LCD_WIDTH) ? 0 : ((LCD_WIDTH - TEXT_WIDTH) / 2))

int show_logo( void )
{
    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

#ifdef SANSA_CLIP
    /* The top 16 lines of the Sansa Clip screen are yellow, and the bottom 48 
       are blue, so we reverse the usual positioning */
    lcd_putsxy(TEXT_XPOS, 0, BOOT_VERSION);
    lcd_bitmap(rockboxlogo, 0, 16, BMPWIDTH_rockboxlogo, BMPHEIGHT_rockboxlogo);
#else
    lcd_bitmap(rockboxlogo, 0, 10, BMPWIDTH_rockboxlogo, BMPHEIGHT_rockboxlogo);
    lcd_putsxy(TEXT_XPOS, LCD_HEIGHT-SYSFONT_HEIGHT, BOOT_VERSION);
#endif

    lcd_update();
    
    return 0;
}
