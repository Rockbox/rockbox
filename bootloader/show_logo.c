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
#include "version.h"

#include "bitmaps/rockboxlogo.h"

#if LCD_WIDTH <= 128
#define BOOT_VERFMT "Boot %s"
#else
#define BOOT_VERFMT "Boot Ver. %s"
#endif

/* Ensure TEXT_XPOS is >= 0 */
#define TEXT_WIDTH(l) ((l)*SYSFONT_WIDTH)
#define TEXT_XPOS(w) (((w) > LCD_WIDTH) ? 0 : ((LCD_WIDTH - (w)) / 2))
#define LOGO_XPOS ((LCD_WIDTH - BMPWIDTH_rockboxlogo) / 2)

void show_logo( void )
{
    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    char verstr[strlen(rbversion) + sizeof (BOOT_VERFMT)];
    int len = snprintf(verstr, sizeof(verstr), BOOT_VERFMT, rbversion);
    int text_width = TEXT_WIDTH(len);
    int text_xpos  = TEXT_XPOS(text_width);

#if defined(SANSA_CLIP) || defined(SANSA_CLIPV2) || defined(SANSA_CLIPPLUS)
    /* The top 16 lines of the Sansa Clip screen are yellow, and the bottom 48 
       are blue, so we reverse the usual positioning */
    lcd_putsxy(text_xpos, 0, verstr);
    lcd_bmp(&bm_rockboxlogo, LOGO_XPOS, 16);
#else
    lcd_bmp(&bm_rockboxlogo, LOGO_XPOS, 10);
    lcd_putsxy(text_xpos, LCD_HEIGHT-SYSFONT_HEIGHT, verstr);
#endif

    lcd_update();
}
