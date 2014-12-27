/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Franklin Wei
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
#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_exit.h"
#include <stdbool.h>

/* data headers go below */
#include "length.h"
#include "mass.h"
#include "temperature.h"
#include "volume.h"

/* this code here was pretty much copy+pasted from superdom.c */
#define NUM_BOX_HEIGHT  (LCD_HEIGHT/6)
#define NUM_BOX_WIDTH   (LCD_WIDTH/6)

#define NUM_MARGIN_X    (LCD_WIDTH-4*NUM_BOX_WIDTH)/2
#define NUM_MARGIN_Y    (LCD_HEIGHT-4*NUM_BOX_HEIGHT)/2

const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

enum {
    RET_VAL_OK,
    RET_VAL_USB,
    RET_VAL_QUIT_ERR, /* quit or error */
};

static int get_number(const char* param, const char *abbr, double* value, bool negative, double max)
{
    char *button_labels[4][4] = {
        { "7",   "8", "9",   "" },
        { "4",   "5", "6",   "" },
        { "1",   "2", "3",   "" },
        { "DEL", "0", "+/-", "" }
    };

    if(!negative)
        button_labels[3][2]=".";

    int i, j, x=1, y=1;
    int height, width;
    int button = 0, ret = RET_VAL_OK;
    bool done = false;
    bool past_decimal;

    if((unsigned long long)*value!=*value)
        past_decimal = true;
    else
        past_decimal = false;

    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_clear_display();
    rb->lcd_getstringsize("DEL", &width, &height);

    if(width > NUM_BOX_WIDTH || height > NUM_BOX_HEIGHT)
        rb->lcd_setfont(FONT_SYSFIXED);

    /* Draw a 4x4 grid */
    for(i=0;i<=4;i++)
    {  /* Vertical lines */
        rb->lcd_vline(NUM_MARGIN_X+(NUM_BOX_WIDTH*i), NUM_MARGIN_Y,
                      NUM_MARGIN_Y+(4*NUM_BOX_HEIGHT));
    }

    /* Horizontal lines */
    for(i=0;i<=4;i++)
    {
        rb->lcd_hline(NUM_MARGIN_X, NUM_MARGIN_X+(((i==0 || i==4)?4:3)*NUM_BOX_WIDTH),
                      NUM_MARGIN_Y+(NUM_BOX_HEIGHT*i));
    }

    for(i=0;i<4;i++)
    {
        for(j=0;j<4;j++)
        {
            rb->lcd_getstringsize(button_labels[i][j], &width, &height);
            rb->lcd_putsxy(NUM_MARGIN_X+(j*NUM_BOX_WIDTH)+NUM_BOX_WIDTH/2-width/2,
                           NUM_MARGIN_Y+(i*NUM_BOX_HEIGHT)+NUM_BOX_HEIGHT/2-height/2,
                           button_labels[i][j]);
        }
    }

    rb->lcd_getstringsize("OK", &width, &height);
    rb->lcd_putsxy(NUM_MARGIN_X+(3*NUM_BOX_WIDTH)+NUM_BOX_WIDTH/2-width/2,
                   NUM_MARGIN_Y+(1.5*NUM_BOX_HEIGHT)+NUM_BOX_HEIGHT/2-height/2,
                   "OK");
    rb->lcd_putsxyf(NUM_MARGIN_X+10, NUM_MARGIN_Y+4*NUM_BOX_HEIGHT+10,"%.3f %s",*value, abbr);
    rb->lcd_getstringsize(param, &width, &height);

    if(width < LCD_WIDTH)
        rb->lcd_putsxy((LCD_WIDTH-width)/2, (NUM_MARGIN_Y-height)/2, param);
    else
        rb->lcd_puts_scroll(0, (NUM_MARGIN_Y/height-1)/2, param);

    rb->lcd_set_drawmode(DRMODE_COMPLEMENT);

    rb->lcd_fillrect(NUM_MARGIN_X+(NUM_BOX_WIDTH*x),
                     NUM_MARGIN_Y+(NUM_BOX_HEIGHT*y),
                     NUM_BOX_WIDTH+1, NUM_BOX_HEIGHT+1);

    rb->lcd_set_drawmode(DRMODE_SOLID);

    rb->lcd_update();

    while(!done)
    {
        button=pluginlib_getaction(-1, plugin_contexts, ARRAYLEN(plugin_contexts));
        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
        if(x==3)
        {
            rb->lcd_fillrect(NUM_MARGIN_X+(NUM_BOX_WIDTH*3),
                             NUM_MARGIN_Y,
                             NUM_BOX_WIDTH+1,
                             NUM_BOX_HEIGHT*4+1);
        }
        else
        {
            rb->lcd_fillrect(NUM_MARGIN_X+(NUM_BOX_WIDTH*x),
                             NUM_MARGIN_Y+(NUM_BOX_HEIGHT*y),
                             NUM_BOX_WIDTH+1, NUM_BOX_HEIGHT+1);
        }
        rb->lcd_set_drawmode(DRMODE_SOLID);
        switch(button)
        {
        case PLA_SELECT:
            if(y!=3 && x!=3)
            {
                *value *= 10;
                *value += button_labels[y][x][0] - '0';
            }
            else if(x==0)
            {
                *value /= 10;
                /* avoid negative zero */
                if(*value==-0)
                    *value=0;
            }
            else if(x==1)
            {
                *value *= 10;
            }
            else if(x==2)
            {
                if(negative)
                    *value *= -1;
                else
                    past_decimal = true;
            }
            else if(x==3)
            {
                done = true;
                break;
            }
            if (*value > max)
                *value = max;
            rb->lcd_set_drawmode(DRMODE_BG|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, NUM_MARGIN_Y+4*NUM_BOX_HEIGHT+10,
                             LCD_WIDTH, 30);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_putsxyf(NUM_MARGIN_X+10,NUM_MARGIN_Y+4*NUM_BOX_HEIGHT+10,
                            "%.3f %s", *value, abbr);
            break;
        case PLA_CANCEL:
            *value = 0;
            done = true;
            ret = RET_VAL_QUIT_ERR;
            break;
#if CONFIG_KEYPAD != IRIVER_H10_PAD
        case PLA_LEFT:
            if(x==0)
            {
                x=3;
            }
            else
            {
                x--;
            }
            break;
        case PLA_RIGHT:
            if(x==3)
            {
                x=0;
            }
            else
            {
                x++;
            }
            break;
#endif
        case PLA_UP:
            if(x!=3)
            {
                if(y==0)
                {
#if CONFIG_KEYPAD == IRIVER_H10_PAD
                    if(x > 0)
                        x--;
                    else
                        x=3;
#endif
                    y=3;
                }
                else
                {
                    y--;
                }
            }
            break;
        case PLA_DOWN:
            if(x!=3)
            {
                if(y==3)
                {
#if CONFIG_KEYPAD == IRIVER_H10_PAD
                    if(x < 3)
                        x++;
                    else
                        x=0;
#endif /* CONFIG_KEYPAD == IRIVER_H10_PAD */
                    y=0;
                }
                else
                {
                    y++;
                }
            }
            break;
        default:
            exit_on_usb(button);
            break;
        }
        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
        if(x==3)
        {
            rb->lcd_fillrect(NUM_MARGIN_X+(NUM_BOX_WIDTH*3),
                             NUM_MARGIN_Y,
                             NUM_BOX_WIDTH+1,
                             NUM_BOX_HEIGHT*4+1);
        }
        else
        {
            rb->lcd_fillrect(NUM_MARGIN_X+(NUM_BOX_WIDTH*x),
                             NUM_MARGIN_Y+(NUM_BOX_HEIGHT*y),
                             NUM_BOX_WIDTH+1, NUM_BOX_HEIGHT+1);
        }
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_update();
    }
    rb->lcd_setfont(FONT_UI);
    rb->lcd_scroll_stop();
    if (ret == RET_VAL_QUIT_ERR)
        rb->splash(HZ, "Cancelled");
    return ret;
}


static void convert_units(struct unit_t *convtab, const size_t len, size_t idx)
{
    data_t base_value = convtab[idx].value * convtab[idx].ratio + convtab[idx].offset;
    for(size_t i=0;i<len;++i)
    {
        if(i != idx)
        {
            convtab[i].value = (base_value - convtab[i].offset) / convtab[i].ratio;
        }
    }
}

static void draw_everything(struct unit_t *convtab, const size_t len, size_t idx)
{
    convert_units(convtab, len, idx);
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_clear_display();
    for(size_t i=0;i<len;++i)
    {
        if(i == idx)
        {
            rb->lcd_set_foreground(LCD_BLACK);
            rb->lcd_set_background(LCD_WHITE);
        }
        else
        {
            rb->lcd_set_foreground(LCD_WHITE);
            rb->lcd_set_background(LCD_BLACK);
        }
        rb->lcd_putsf(0, i, "%.3f %s",
                      convtab[i].value,
                      convtab[i].abbr);
    }
    rb->lcd_update();
}

static void do_unitconv(struct unit_t *convtab, const size_t len)
{
    size_t idx = 0;
    bool quit = false;
    while(!quit)
    {
        draw_everything(convtab, len, idx);
        int btn;
        switch(btn = pluginlib_getaction(-1, plugin_contexts, ARRAYLEN(plugin_contexts)))
        {
        case PLA_DOWN:
        case PLA_DOWN_REPEAT:
            if(idx++ >= len-1)
                idx=0;
            break;
        case PLA_UP:
        case PLA_UP_REPEAT:
            if((signed int)idx-- <= 0)
                idx = len-1;
            break;
        case PLA_RIGHT:
        case PLA_RIGHT_REPEAT:
            convtab[idx].value += convtab[idx].step;
            break;
        case PLA_LEFT:
        case PLA_LEFT_REPEAT:
            convtab[idx].value -= convtab[idx].step;
            if(!convtab[idx].negative && convtab[idx].value < 0)
                convtab[idx].value += convtab[idx].step;
            break;
        case PLA_SELECT:
            get_number(convtab[idx].name, convtab[idx].abbr, &convtab[idx].value, convtab[idx].negative, 1000000000);
            break;
        case PLA_CANCEL:
            return;
        default:
            exit_on_usb(btn);
            break;
        }
        rb->yield();
    }
}

enum plugin_status plugin_start(const void* parameter)
{
    (void) parameter;
    int sel = 0;
    MENUITEM_STRINGLIST(menu, "Unit Converter", NULL,
                        "Length",
                        "Mass",
                        "Temperature",
                        "Volume",
                        "Quit");
    bool quit = false;
    while(!quit)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            /* length */
            do_unitconv(length_table, ARRAYLEN(length_table));
            break;
        case 1:
            /* mass */
            do_unitconv(mass_table, ARRAYLEN(mass_table));
            break;
        case 2:
            /* temperature */
            do_unitconv(temperature_table, ARRAYLEN(temperature_table));
            break;
        case 3:
            /* volume */
            do_unitconv(volume_table, ARRAYLEN(volume_table));
            break;
        case 4:
            quit = true;
            break;
        default:
            break;
        }
    }
    return PLUGIN_OK;
}
