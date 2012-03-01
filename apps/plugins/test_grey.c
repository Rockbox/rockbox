/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Jens Arnold
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
#include "plugin.h"
#include "lib/grey.h"
#include "lib/helper.h"
#include "lib/pluginlib_actions.h"

/* this set the context to use with PLA */
static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

#define GREY_QUIT           PLA_EXIT
#define GREY_QUIT2          PLA_CANCEL
#define GREY_OK             PLA_SELECT
#define GREY_PREV           PLA_LEFT
#define GREY_NEXT           PLA_RIGHT
#ifdef HAVE_SCROLLWHEEL
#define GREY_UP             PLA_SCROLL_FWD
#define GREY_UP_REPEAT      PLA_SCROLL_FWD_REPEAT
#define GREY_DOWN           PLA_SCROLL_BACK
#define GREY_DOWN_REPEAT    PLA_SCROLL_BACK_REPEAT
#else
#define GREY_UP             PLA_UP
#define GREY_UP_REPEAT      PLA_UP_REPEAT
#define GREY_DOWN           PLA_DOWN
#define GREY_DOWN_REPEAT    PLA_DOWN_REPEAT
#endif /*HAVE_SCROLLWHEEL*/

#define BLOCK_WIDTH  (LCD_WIDTH/8)
#define BLOCK_HEIGHT (LCD_HEIGHT/8)

#define STEPS 16

GREY_INFO_STRUCT

static const unsigned char dither_matrix[16][16] = {
    {   0,192, 48,240, 12,204, 60,252,  3,195, 51,243, 15,207, 63,255 },
    { 128, 64,176,112,140, 76,188,124,131, 67,179,115,143, 79,191,127 },
    {  32,224, 16,208, 44,236, 28,220, 35,227, 19,211, 47,239, 31,223 },
    { 160, 96,144, 80,172,108,156, 92,163, 99,147, 83,175,111,159, 95 },
    {   8,200, 56,248,  4,196, 52,244, 11,203, 59,251,  7,199, 55,247 },
    { 136, 72,184,120,132, 68,180,116,139, 75,187,123,135, 71,183,119 },
    {  40,232, 24,216, 36,228, 20,212, 43,235, 27,219, 39,231, 23,215 },
    { 168,104,152, 88,164,100,148, 84,171,107,155, 91,167,103,151, 87 },
    {   2,194, 50,242, 14,206, 62,254,  1,193, 49,241, 13,205, 61,253 },
    { 130, 66,178,114,142, 78,190,126,129, 65,177,113,141, 77,189,125 },
    {  34,226, 18,210, 46,238, 30,222, 33,225, 17,209, 45,237, 29,221 },
    { 162, 98,146, 82,174,110,158, 94,161, 97,145, 81,173,109,157, 93 },
    {  10,202, 58,250,  6,198, 54,246,  9,201, 57,249,  5,197, 53,245 },
    { 138, 74,186,122,134, 70,182,118,137, 73,185,121,133, 69,181,117 },
    {  42,234, 26,218, 38,230, 22,214, 41,233, 25,217, 37,229, 21,213 },
    { 170,106,154, 90,166,102,150, 86,169,105,153, 89,165,101,149, 85 }
};

static unsigned char input_levels[STEPS+1];
static unsigned char lcd_levels[STEPS+1];

static unsigned char *gbuf;
static size_t gbuf_size = 0;

static void fill_rastered(int bx, int by, int bw, int bh, int step)
{
    int x, xmax, y, ymax;
    int level;
    
    if (step < 0)
        step = 0;
    else if (step > STEPS)
        step = STEPS;
        
    level = input_levels[step];
    level += (level-1) >> 7;

    for (y = (LCD_HEIGHT/2) + by * BLOCK_HEIGHT, ymax = y + bh * BLOCK_HEIGHT;
         y < ymax; y++)
    {
        for (x = (LCD_WIDTH/2) + bx * BLOCK_WIDTH, xmax = x + bw * BLOCK_WIDTH;
             x < xmax; x++)
        {
            grey_set_foreground((level > dither_matrix[y & 0xf][x & 0xf])
                                ? 255 : 0);
            grey_drawpixel(x, y);
        }
    }
}

/* plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    bool done = false;
    int cur_step = 1;
    int button, i, j, l, fd;
    unsigned char filename[MAX_PATH];

    /* standard stuff */
    (void)parameter;
    
    gbuf = (unsigned char *) rb->plugin_get_buffer(&gbuf_size);

    if (!grey_init(gbuf, gbuf_size,
                   GREY_BUFFERED|GREY_RAWMAPPED|GREY_ON_COP,
                   LCD_WIDTH, LCD_HEIGHT, NULL))
    {
        rb->splash(HZ, "Not enough memory.");
        return PLUGIN_ERROR;
    }
    for (i = 0; i <= STEPS; i++)
        input_levels[i] = lcd_levels[i] = (255 * i + (STEPS/2)) / STEPS;

    backlight_ignore_timeout();

    grey_set_background(0); /* set background to black */
    grey_clear_display();
    grey_show(true);

    rb->lcd_setfont(FONT_SYSFIXED);

    while (!done)
    {
        fill_rastered(-3, -3, 2, 2, cur_step - 1);
        fill_rastered(-1, -3, 2, 2, cur_step);
        fill_rastered(1, -3, 2, 2, cur_step + 1);
        fill_rastered(-3, -1, 2, 2, cur_step);
        grey_set_foreground(lcd_levels[cur_step]);
        grey_fillrect(LCD_WIDTH/2-BLOCK_WIDTH, LCD_HEIGHT/2-BLOCK_HEIGHT,
                      2*BLOCK_WIDTH, 2*BLOCK_HEIGHT);
        fill_rastered(1, -1, 2, 2, cur_step);
        fill_rastered(-3, 1, 2, 2, cur_step + 1);
        fill_rastered(-1, 1, 2, 2, cur_step);
        fill_rastered(1, 1, 2, 2, cur_step - 1);
        grey_update();

        button = pluginlib_getaction(TIMEOUT_BLOCK, plugin_contexts,
                          ARRAYLEN(plugin_contexts));
        switch (button)
        {
            case GREY_PREV:
                if (cur_step > 0)
                    cur_step--;
                break;

            case GREY_NEXT:
                if (cur_step < STEPS)
                    cur_step++;
                break;

            case GREY_UP:
            case GREY_UP_REPEAT:
                l = lcd_levels[cur_step];
                if (l < 255)
                {
                    l++;
                    for (i = cur_step; i <= STEPS; i++)
                        if (lcd_levels[i] < l)
                            lcd_levels[i] = l;
                }
                break;

            case GREY_DOWN:
            case GREY_DOWN_REPEAT:
                l = lcd_levels[cur_step];
                if (l > 0)
                {
                    l--;
                    for (i = cur_step; i >= 0; i--)
                        if (lcd_levels[i] > l)
                            lcd_levels[i] = l;
                }
                break;
                
            case GREY_OK:

                /* dump result in form suitable for lcdlinear[] */
                rb->create_numbered_filename(filename, HOME_DIR, "test_grey_",
                                             ".txt", 2 IF_CNFN_NUM_(, NULL));
                fd = rb->open(filename, O_RDWR|O_CREAT|O_TRUNC, 0666);
                if (fd >= 0)
                {
                    rb->fdprintf(fd, "Adjusted grey shades\n");
                    for (i = 0; i <= STEPS; i++)
                         rb->fdprintf(fd, "%3d: %3d\n", input_levels[i],
                                      lcd_levels[i]);
                    rb->fdprintf(fd,"\n\nInterpolated lcdlinear matrix\n");

                    for (i = 1; i <= STEPS; i++)
                    {
                        for (j=0; j < STEPS; j++)
                        {
                            rb->fdprintf(fd, "%3d, ", 
                                lcd_levels[i-1] + 
                                ((lcd_levels[i] - lcd_levels[i-1])*j)/STEPS);
                        }
                        rb->fdprintf(fd, "\n");
                    }
                    rb->close(fd);
                }
                /* fall through */

            case GREY_QUIT:
            case GREY_QUIT2:
                done = true;
                break;
        }
    }

    grey_release();
    backlight_use_settings();
    return PLUGIN_OK;
}
