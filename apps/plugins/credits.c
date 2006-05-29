/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Robert Hak <rhak at ramapo.edu>
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"

PLUGIN_HEADER

void roll_credits(void);
const char* const credits[] = {
#include "credits.raw" /* generated list of names from docs/CREDITS */
};

static struct plugin_api* rb;

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int j = 0;
    int btn;

    (void)parameter;
    rb = api;

    rb->backlight_set_timeout(1);

    rb->show_logo();
#ifdef HAVE_LCD_CHARCELLS
    rb->lcd_double_height(false);
#endif

    for (j = 0; j < 15; j++) {
        rb->sleep((HZ*2)/10);

        btn = rb->button_get(false);
        if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
            return PLUGIN_OK;
    }

    roll_credits();

    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);

    return PLUGIN_OK;
}

#ifdef HAVE_LCD_CHARCELLS

void roll_credits(void)
{
    int numnames = sizeof(credits)/sizeof(char*);
    int curr_name = 0;
    int curr_len = utf8length(credits[0]);
    int curr_index = 0;
    int curr_line = 0;
    int name, len, new_len, line, x;

    while (1)
    {
        rb->lcd_clear_display();

        name = curr_name;
        x = -curr_index;
        len = curr_len;
        line = curr_line;

        while (x < 11)
        {
            int x2;

            if (x < 0)
                rb->lcd_puts(0, line, credits[name] + utf8seek(credits[name], -x));
            else
                rb->lcd_puts(x, line, credits[name]);

            if (++name >= numnames)
                break;

            line ^= 1;

            x2 = x + len/2;
            if ((unsigned)x2 < 11)
                rb->lcd_putc(x2, line, '*');

            new_len = utf8length(credits[name]);
            x += MAX(len/2 + 2, len - new_len/2 + 1);
            len = new_len;
        }
        /* abort on keypress */
        if (rb->button_get_w_tmo(HZ/8) & BUTTON_REL)
            return;

        if (++curr_index >= curr_len)
        {
            if (++curr_name >= numnames)
                break;
            new_len = utf8length(credits[curr_name]);
            curr_index -= MAX(curr_len/2 + 2, curr_len - new_len/2 + 1);
            curr_len = new_len;
            curr_line ^= 1;
        }
    }
}

#else

void roll_credits(void)
{
#if (CONFIG_KEYPAD == RECORDER_PAD)
    #define PAUSE_TIME 1.2
    #define ANIM_SPEED 35
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD)
    #define PAUSE_TIME 0
    #define ANIM_SPEED 100
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
    #define PAUSE_TIME 0
    #define ANIM_SPEED 35
#else
    #define PAUSE_TIME 1
    #define ANIM_SPEED 40
#endif

    #define NUM_VISIBLE_LINES (LCD_HEIGHT/font_h - 1)
    #define CREDITS_TARGETPOS ((LCD_WIDTH/2)-(credits_w/2))

    int i=0, j=0, namepos=0, offset_dummy, btn;
    int name_w, name_h, name_targetpos=1, font_h;
    int credits_w, credits_pos;
    int numnames = (sizeof(credits)/sizeof(char*));
    char name[40], elapsednames[20];

    rb->lcd_setfont(FONT_UI);
    rb->lcd_clear_display();
    rb->lcd_update();

    rb->lcd_getstringsize("A", NULL, &font_h);

    /* snprintf "credits" text, and save the width and height */
    rb->snprintf(elapsednames, sizeof(elapsednames), "[Credits] %d/%d",
                 j+1, numnames);
    rb->lcd_getstringsize(elapsednames, &credits_w, NULL);

    /* fly in "credits" text from the left */
    for(credits_pos = 0 - credits_w; credits_pos <= CREDITS_TARGETPOS;
        credits_pos += (CREDITS_TARGETPOS-credits_pos + 14) / 7)
    {
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_fillrect(0, 0, LCD_WIDTH, font_h);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_putsxy(credits_pos, 0, elapsednames);
        rb->lcd_update_rect(0, 0, LCD_WIDTH, font_h);
        rb->sleep(HZ/ANIM_SPEED);
    }

    /* first screen's worth of lines fly in */
    for(i=0; i<NUM_VISIBLE_LINES; i++)
    {
        rb->snprintf(name, sizeof(name), "%s", credits[i]);
        rb->lcd_getstringsize(name, &name_w, &name_h);

        rb->snprintf(elapsednames, sizeof(elapsednames), "[Credits] %d/%d",
                     i+1, numnames);
        rb->lcd_getstringsize(elapsednames, &credits_w, NULL);
        rb->lcd_putsxy(CREDITS_TARGETPOS, 0, elapsednames);

        for(namepos = 0-name_w; namepos <= name_targetpos;
            namepos += (name_targetpos - namepos + 14) / 7)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, font_h*(i+1), LCD_WIDTH, font_h); /* clear any trails left behind */
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_putsxy(namepos, font_h*(i+1), name);
            rb->lcd_update_rect(0, font_h*(i+1), LCD_WIDTH, font_h);
            rb->lcd_update_rect(CREDITS_TARGETPOS, 0, credits_w, font_h);

            /* exit on keypress */
            btn = rb->button_get_w_tmo(HZ/ANIM_SPEED);
            if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
                return;
        }
    }
    j+=i;

    /* pause for a bit if needed */
    btn = rb->button_get_w_tmo(HZ*PAUSE_TIME); /* exit on keypress */
    if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
        return;

    /* now begin looping the in-out animation */
    while(j < numnames)
    {
        /* just a screen's worth at a time */
        for(i=0; i<NUM_VISIBLE_LINES; i++)
        {
            if(j+i >= numnames)
                break;

            offset_dummy=1;

            rb->snprintf(name, sizeof(name), "%s", credits[j+i-NUM_VISIBLE_LINES]);
            rb->lcd_getstringsize(name, &name_w, &name_h);

            /* fly out an existing line.. */
            while(namepos<LCD_WIDTH+offset_dummy)
            {
                rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                rb->lcd_fillrect(0, font_h*(i+1), LCD_WIDTH, font_h); /* clear trails */
                rb->lcd_set_drawmode(DRMODE_SOLID);
                rb->lcd_putsxy(namepos, font_h*(i+1), name);
                rb->lcd_update_rect(0, font_h*(i+1), LCD_WIDTH, font_h);

                /* exit on keypress */
                btn = rb->button_get_w_tmo(HZ/ANIM_SPEED);
                if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
                    return;

                namepos += offset_dummy;
                offset_dummy++;
            }

            rb->snprintf(name, sizeof(name), "%s", credits[j+i]);
            rb->lcd_getstringsize(name, &name_w, &name_h);

            rb->snprintf(elapsednames, sizeof(elapsednames), "[Credits] %d/%d",
                         j+i+1, numnames);
            rb->lcd_getstringsize(elapsednames, &credits_w, NULL);
            rb->lcd_putsxy(CREDITS_TARGETPOS, 0, elapsednames);

            for(namepos = 0-name_w; namepos <= name_targetpos;
                namepos += (name_targetpos - namepos + 14) / 7)
            {
                rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                rb->lcd_fillrect(0, font_h*(i+1), LCD_WIDTH, font_h);
                rb->lcd_set_drawmode(DRMODE_SOLID);
                rb->lcd_putsxy(namepos, font_h*(i+1), name);
                rb->lcd_update_rect(0, font_h*(i+1), LCD_WIDTH, font_h);
                rb->lcd_update_rect(CREDITS_TARGETPOS, 0, credits_w, font_h);

                /* exit on keypress */
                btn = rb->button_get_w_tmo(HZ/ANIM_SPEED);
                if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
                    return;
            }

            namepos = name_targetpos;

            /* ..and repeat. */
        }
        j+=i;

        btn = rb->button_get_w_tmo(HZ*PAUSE_TIME); /* exit on keypress */
        if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
            return;
    }

    btn = rb->button_get_w_tmo(HZ); /* exit on keypress */
    if (btn !=  BUTTON_NONE && !(btn & BUTTON_REL))
        return;

    offset_dummy = 1;

    /* now make the text exit to the right */
    for(credits_pos = (LCD_WIDTH/2)-(credits_w/2); credits_pos <= LCD_WIDTH+offset_dummy;
        credits_pos += offset_dummy, offset_dummy++)
    {
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_fillrect(0, 0, LCD_WIDTH, font_h);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_putsxy(credits_pos, 0, elapsednames);
        rb->lcd_update();
    }
}

#endif
