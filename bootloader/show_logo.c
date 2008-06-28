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

#include "rockboxlogo.h"

int show_logo( void )
{
    char boot_version[32];
    
    snprintf(boot_version, sizeof(boot_version), "Boot Ver. %s", APPSVERSION);

    lcd_clear_display();
    lcd_bitmap(rockboxlogo, 0, 10, BMPWIDTH_rockboxlogo, BMPHEIGHT_rockboxlogo);
    lcd_setfont(FONT_SYSFIXED);
    lcd_putsxy((LCD_WIDTH/2) - ((strlen(boot_version)*SYSFONT_WIDTH)/2),
               LCD_HEIGHT-SYSFONT_HEIGHT, (unsigned char *)boot_version);

    lcd_update();
    
    return 0;
}
