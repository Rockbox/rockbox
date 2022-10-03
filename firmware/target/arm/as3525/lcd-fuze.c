/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Linus Nielsen Feltzing
 * Copyright (C) 2008 by Dave Chapman
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

#include "config.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "file.h"
#include "clock-target.h"
#include "dbop-as3525.h"
#include "lcd-fuze.h"

/* The controller is unknown, but some registers appear to be the same as the
   HD66789R */
static bool display_on = false; /* is the display turned on? */

/* Reverse Flag */
static int r_disp_control_rev = R_DISP_CONTROL_NORMAL;

/* Flip flag */
static int r_drv_output_control = R_DRV_OUTPUT_CONTROL_NORMAL;
#ifdef HAVE_LCD_FLIP
static int r_gate_scan_pos = R_GATE_SCAN_POS_NORMAL;
#endif

static int xoffset = 20;

/*** hardware configuration ***/

void lcd_set_invert_display(bool yesno)
{
    r_disp_control_rev = yesno ? R_DISP_CONTROL_REV :
                                 R_DISP_CONTROL_NORMAL;

    if (display_on)
    {
        lcd_write_reg(R_DISP_CONTROL1, 0x0013 | r_disp_control_rev);
    }

}

#ifdef HAVE_LCD_FLIP
/* turn the display upside down  */
void lcd_set_flip(bool yesno)
{
    if (yesno)
    {
        xoffset = 0;
        r_drv_output_control = R_DRV_OUTPUT_CONTROL_FLIPPED;
        r_gate_scan_pos = R_GATE_SCAN_POS_FLIPPED;
    }
    else
    {
        xoffset = 20;
        r_drv_output_control = R_DRV_OUTPUT_CONTROL_NORMAL;
        r_gate_scan_pos = R_GATE_SCAN_POS_NORMAL;
    }

    if (display_on)
    {
        lcd_write_reg(R_GATE_SCAN_POS, r_gate_scan_pos);
        lcd_write_reg(R_DRV_OUTPUT_CONTROL, r_drv_output_control);
    }
}
#endif

void fuze_display_on(void)
{
    /* Initialise in the same way as the original firmare */

    lcd_write_reg(R_DISP_CONTROL1, 0);
    lcd_write_reg(R_POWER_CONTROL4, 0);

    lcd_write_reg(R_POWER_CONTROL2, 0x3704);
    lcd_write_reg(0x14, 0x1a1b);
    lcd_write_reg(R_POWER_CONTROL1, 0x3860);
    lcd_write_reg(R_POWER_CONTROL4, 0x40);

    lcd_write_reg(R_POWER_CONTROL4, 0x60);

    lcd_write_reg(R_POWER_CONTROL4, 0x70);
    lcd_write_reg(R_DRV_OUTPUT_CONTROL, r_drv_output_control);
    lcd_write_reg(R_DRV_WAVEFORM_CONTROL, (7<<8));
    lcd_write_reg(R_ENTRY_MODE, R_ENTRY_MODE_HORZ);
    lcd_write_reg(R_DISP_CONTROL2, 0x01);
    lcd_write_reg(R_FRAME_CYCLE_CONTROL, (1<<10));
    lcd_write_reg(R_EXT_DISP_IF_CONTROL, 0);

    lcd_write_reg(R_GAMMA_FINE_ADJ_POS1, 0x40);
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS2, 0x0687);
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS3, 0x0306);
    lcd_write_reg(R_GAMMA_GRAD_ADJ_POS, 0x104);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG1, 0x0585);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG2, 255+66);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG3, 0x0687+128);
    lcd_write_reg(R_GAMMA_GRAD_ADJ_NEG, 259);
    lcd_write_reg(R_GAMMA_AMP_ADJ_RES_POS, 0);
    lcd_write_reg(R_GAMMA_AMP_AVG_ADJ_RES_NEG, 0);

    lcd_write_reg(R_1ST_SCR_DRV_POS, (LCD_WIDTH - 1));
    lcd_write_reg(R_2ND_SCR_DRV_POS, 0);
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, (LCD_WIDTH - 1));
    lcd_write_reg(R_VERT_RAM_ADDR_POS, 0);
    lcd_write_reg(0x46, (((LCD_WIDTH - 1) + xoffset) << 8) | xoffset);
    lcd_write_reg(0x47, (LCD_HEIGHT - 1));
    lcd_write_reg(0x48, 0x0);

    lcd_write_reg(R_DISP_CONTROL1, 0x11);
    lcd_write_reg(R_DISP_CONTROL1, 0x13 | r_disp_control_rev);

    display_on = true;  /* must be done before calling lcd_update() */
    lcd_update();
}

#if defined(HAVE_LCD_ENABLE)
void lcd_enable(bool on)
{
    if (display_on == on)
        return;

    if(on)
    {
        lcd_write_reg(R_START_OSC, 1);
        lcd_write_reg(R_POWER_CONTROL1, 0);
        lcd_write_reg(R_POWER_CONTROL2, 0x3704);
        lcd_write_reg(0x14, 0x1a1b);
        lcd_write_reg(R_POWER_CONTROL1, 0x3860);
        lcd_write_reg(R_POWER_CONTROL4, 0x40);
        lcd_write_reg(R_POWER_CONTROL4, 0x60);
        lcd_write_reg(R_POWER_CONTROL4, 112);
        lcd_write_reg(R_DISP_CONTROL1, 0x11);
        lcd_write_reg(R_DISP_CONTROL1, 0x13 | r_disp_control_rev);
        lcd_write_reg(R_DRV_OUTPUT_CONTROL, r_drv_output_control);
        lcd_write_reg(R_GATE_SCAN_POS, r_gate_scan_pos);
        display_on = true;
        lcd_update();      /* Resync display */
        send_event(LCD_EVENT_ACTIVATION, NULL);
        sleep(0);

    }
    else
    {
        lcd_write_reg(R_DISP_CONTROL1, 0x22);
        lcd_write_reg(R_DISP_CONTROL1, 0);
        lcd_write_reg(R_POWER_CONTROL1, 1);
        display_on = false;
    }
}
#endif

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
bool lcd_active(void)
{
    return display_on;
}
#endif

/*** update functions ***/

/* FIXME : find the datasheet for this RENESAS controller so we identify the
 * registers used in windowing code (not present in HD66789R) */

/* Set horizontal window addresses */
static void lcd_window_x(int xmin, int xmax)
{
    xmin += xoffset;
    xmax += xoffset;
    lcd_write_reg(0x46, (xmax << 8) | xmin);
    lcd_write_reg(0x20, xmin);
}

/* Set vertical window addresses */
static void lcd_window_y(int ymin, int ymax)
{
    lcd_write_reg(0x47, ymax);
    lcd_write_reg(0x48, ymin);
    lcd_write_reg(R_RAM_ADDR_SET, ymin);
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    if (!display_on)
        return;

    lcd_write_reg(R_ENTRY_MODE, R_ENTRY_MODE_HORZ);

    lcd_window_x(0, LCD_WIDTH - 1);
    lcd_window_y(0, LCD_HEIGHT - 1);

    lcd_write_cmd(R_WRITE_DATA_2_GRAM);

    dbop_write_data(FBADDR(0,0), LCD_WIDTH*LCD_HEIGHT);
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    const fb_data *ptr;

    if (!display_on)
        return;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= LCD_WIDTH) || 
        (y >= LCD_HEIGHT) || (x + width <= 0) || (y + height <= 0))
        return;

    if (x < 0)
    {   /* clip left */
        width += x;
        x = 0;
    }
    if (y < 0)
    {   /* clip top */
        height += y;
        y = 0;
    }
    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x; /* clip right */
    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y; /* clip bottom */

    lcd_write_reg(R_ENTRY_MODE, R_ENTRY_MODE_HORZ);

    /* we need to make x and width even to enable 32bit transfers */
    width = (width + (x & 1) + 1) & ~1;
    x &= ~1;

    lcd_window_x(x, x + width - 1);
    lcd_window_y(y, y + height -1);

    lcd_write_cmd(R_WRITE_DATA_2_GRAM);

    ptr = FBADDR(x,y);

    do
    {
        dbop_write_data(ptr, width);
        ptr += LCD_WIDTH;
    }
    while (--height > 0);
}
