/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 * Copyright (C) 2021-2022 Aidan MacDonald
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 ****************************************************************************/

/* Panel bring-up. The screen primitives are shared with the X1000 in
 * bootloader/ingenic/gui-ingenic.c; only these two are per target. */

#include "x1600bootloader.h"
#include "lcd.h"
#include "backlight.h"
#include "backlight-target.h"
#include "font.h"

static bool lcd_ready = false;

void init_lcd(void)
{
    if(lcd_ready)
        return;

    lcd_init();
    font_init();
    lcd_setfont(FONT_SYSFIXED);

    lcd_clear_display();
    lcd_update();

    backlight_init();

    lcd_ready = true;

}

void gui_shutdown(void)
{
    if(!lcd_ready)
        return;

    backlight_hw_off();
}
