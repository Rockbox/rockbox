/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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

#include "system.h"
#include "debug-target.h"
#include "dma-imx233.h"
#include "action.h"
#include "lcd.h"
#include "font.h"

static struct
{
    const char *name;
    unsigned chan;
} dbg_channels[] =
{
    { "i2c", APB_I2C },
    { "dac", APB_AUDIO_DAC },
    { "ssp1", APB_SSP(1) },
    { "ssp2", APB_SSP(2) },
};

bool dbg_hw_info_dma(void)
{
    lcd_setfont(FONT_SYSFIXED);
    
    while(1)
    {
        int button = get_action(CONTEXT_STD, HZ / 25);
        switch(button)
        {
            case ACTION_STD_NEXT:
            case ACTION_STD_PREV:
            case ACTION_STD_OK:
            case ACTION_STD_MENU:
                lcd_setfont(FONT_UI);
                return true;
            case ACTION_STD_CANCEL:
                lcd_setfont(FONT_UI);
                return false;
        }
        
        lcd_clear_display();

        lcd_putsf(0, 0, "I G F C E name bar apb ahb");
        for(unsigned i = 0; i < ARRAYLEN(dbg_channels); i++)
        {
            struct imx233_dma_info_t info = imx233_dma_get_info(dbg_channels[i].chan, DMA_INFO_ALL);
            lcd_putsf(0, i + 1, "%c %c %c %c %c %4s %x %x %x",
                info.int_enabled ? 'i' : ' ',
                info.gated ? 'g' : ' ',
                info.freezed ? 'f' : ' ',
                info.int_cmdcomplt ? 'c' : ' ',
                info.int_error ? 'e' : ' ',
                dbg_channels[i].name, info.bar, info.apb_bytes, info.ahb_bytes);
        }
        
        lcd_update();
        yield();
    }
}

bool dbg_hw_info(void)
{
    return dbg_hw_info_dma() && dbg_hw_target_info();
}

bool dbg_ports(void)
{
    return false;
}
