/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2003 Lee Pilgrim
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 **************************************************************************/
#include "plugin.h"

#ifndef SIMULATOR
#ifdef HAVE_LCD_BITMAP

static struct plugin_api* rb;

/* This table is used to define x positions on a logarithmic (dBfs) scale.
   The formula I used to figure these out was log(x+1)*31.89293,
   where x was the original x position. */
static unsigned char db_scale[] = {0,9.6,15.2,19.2,22.3,24.8,27,28.8,30.4,31.9,33.2,34.4,35.5,36.6,37.5,38.4,
                                   39.2,40,40.8,41.5,52.2,42.8,43.4,44,44.6,45.1,45.7,46.2,46.6,47.1,47.6,48,
                                   48.4,48.8,49.2,49.6,50,50.4,50.7,51.1,51.4,51.8,52.1,52.4,52.7,53,53.3,53.6,
                                   53.9,54.2,54.5,54.7,55,55.3,55.5,55.8,56};

/* Define's y positions, and makes it look like an arch, like a real needle would. */
static unsigned char y_values[] = {32,31,30,29,28,27,26,25,24,24,23,22,22,21,21,20,19,19,18,18,18,18,18,17,17,17,17,17,
                                   17,17,17,17,17,18,18,18,18,18,19,19,20,21,21,22,22,23,24,24,25,26,27,28,29,30,31,32,
                                   32,31,30,29,28,27,26,25,24,24,23,22,22,21,21,20,19,19,18,18,18,18,18,17,17,17,17,17,
                                   17,17,17,17,17,18,18,18,18,18,19,19,20,21,21,22,22,23,24,24,25,26,27,28,29,30,31,32};

/* Linear mode bitmap icon. */
static unsigned char mode_linear[] = {0xDF,0x10,0xD0,0x00,0xDF,0x00,0xDF,0x02,0xC4,0x08,0xDF,
                                      0x00,0xDF,0x15,0xD1,0x00,0xDE,0x05,0xDE,0x00,0xDF,0x05,
                                      0xDA};

/* Logarithmic (dBfs) mode bitmap icon. */
static unsigned char mode_dbfs[] =   {0xC8,0x14,0x14,0x1F,0x00,0xDF,0x15,0x15,0x0A,0xC0,0x1E,
                                      0x05,0x05,0xC0,0x16,0x15,0xD5,0x09,0xC0,0x00,0xF0,0x00,
                                      0xFE};


#define MAX_PEAK 0x7FFF
#define NEEDLE_BOTTOM_Y 54

#define LEFT_NEEDLE_BOTTOM_X 28
int left_needle_top_y;
int left_needle_top_x;
int old_left_needle_top_x = 0;
int left_needle_top_x_no_log;

#define RIGHT_NEEDLE_BOTTOM_X 84
int right_needle_top_y;
int right_needle_top_x;
int old_right_needle_top_x = 56;
int right_needle_top_x_no_log;

bool show_arch = true;
bool use_log_scale = true;
int needle_cover_mode = 1;

/* This draws everthing but the needles, needle covers, and the visible arch. */
void draw_status_bar(void)
{
    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_clear_display();

    rb->lcd_drawline(0, 10, 111, 10);
    rb->lcd_putsxy(1, 1, "VU Meter");

    if(use_log_scale)
        rb->lcd_bitmap(mode_dbfs, 88, 1, 23, 8, true);
    else
        rb->lcd_bitmap(mode_linear, 88, 1, 23, 8, true);

    rb->lcd_putsxy(25, 56, "L");
    rb->lcd_putsxy(81, 56, "R");

    /* The last line under the needle covers. */
    rb->lcd_drawline(0, 54, 111, 54);

    rb->lcd_update();
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    TEST_PLUGIN_API(api);
    (void) parameter;
    rb = api;

    draw_status_bar();

    while (!PLUGIN_OK)
    {
        left_needle_top_x_no_log =
            (rb->mas_codec_readreg(0xC) * 56 / MAX_PEAK);

        right_needle_top_x_no_log =
            (rb->mas_codec_readreg(0xD) * 56 / MAX_PEAK) + 56;

        rb->lcd_clear_display();

        if (use_log_scale)
        {
            left_needle_top_x = db_scale[left_needle_top_x_no_log];
            right_needle_top_x = db_scale[right_needle_top_x_no_log-56]+56;
        }

        else
        {
            left_needle_top_x = left_needle_top_x_no_log;
            right_needle_top_x = right_needle_top_x_no_log;
        }

        /* Makes a decay on the needle. Todo: Make a user custom decay. */
        left_needle_top_x = (left_needle_top_x+old_left_needle_top_x*2)/3;
        right_needle_top_x = (right_needle_top_x+old_right_needle_top_x*2)/3;

        old_left_needle_top_x = left_needle_top_x;
        old_right_needle_top_x = right_needle_top_x;

        left_needle_top_y = y_values[left_needle_top_x];
        right_needle_top_y = y_values[right_needle_top_x];

        /* Draws the needles */
        rb->lcd_drawline(LEFT_NEEDLE_BOTTOM_X, NEEDLE_BOTTOM_Y,
                        left_needle_top_x, left_needle_top_y);

        rb->lcd_drawline(RIGHT_NEEDLE_BOTTOM_X, NEEDLE_BOTTOM_Y,
                         right_needle_top_x, right_needle_top_y);

        if(needle_cover_mode == 1)  /* Rounded needle cover. */
        {
            /* Left needle cover, top to bottom */
            rb->lcd_drawline(27, 49, 29, 49);
            rb->lcd_drawline(25, 50, 31, 50);
            rb->lcd_drawline(23, 51, 33, 51);
            rb->lcd_drawline(22, 52, 34, 52);
            rb->lcd_drawline(22, 53, 34, 53);

            /* Right needle cover, top to bottom */
            rb->lcd_drawline(83, 49, 85, 49);
            rb->lcd_drawline(81, 50, 87, 50);
            rb->lcd_drawline(79, 51, 89, 51);
            rb->lcd_drawline(78, 52, 90, 52);
            rb->lcd_drawline(78, 53, 90, 53);
        }

        else  /* Pyramid needle cover. */
        {
            /* Left needle cover, top to bottom */
            rb->lcd_drawpixel(28, 49);
            rb->lcd_drawline(27, 50, 29, 50);
            rb->lcd_drawline(26, 51, 30, 51);
            rb->lcd_drawline(25, 52, 31, 52);
            rb->lcd_drawline(24, 53, 32, 53);

            /* Right needle cover, top to bottom */
            rb->lcd_drawpixel(84, 49);
            rb->lcd_drawline(83, 50, 85, 50);
            rb->lcd_drawline(82, 51, 86, 51);
            rb->lcd_drawline(81, 52, 87, 52);
            rb->lcd_drawline(80, 53, 88, 53);
        }

        if(show_arch)
        {
            int i;
            for(i=0;i<=112;i++)
                rb->lcd_drawpixel(i, (y_values[i])-2 );
        }

        rb->lcd_update_rect(0,15,112,39);

        switch (rb->button_get_w_tmo(1))
        {
            case BUTTON_OFF:
                return PLUGIN_OK;
                break;

            case BUTTON_ON:
                rb->lcd_clear_display();
                rb->lcd_putsxy(1, 1, "Information:");
                rb->lcd_putsxy(2, 15, "OFF: Exit");
                rb->lcd_putsxy(2, 25, "ON: This Info");
                rb->lcd_putsxy(2, 35, "PLAY: Change Scale");
                rb->lcd_putsxy(2, 45, "F1: Needle Covers");
                rb->lcd_putsxy(2, 55, "F2: Visible Arch");
                rb->lcd_update();
                rb->sleep(HZ*3);
                draw_status_bar();
                break;

            case BUTTON_PLAY:
                use_log_scale = !use_log_scale;
                draw_status_bar();
                break;

            case BUTTON_F1:
                needle_cover_mode==2 ? needle_cover_mode=1 : needle_cover_mode++;
                break;

            case BUTTON_F2:
                show_arch = !show_arch;
                break;

            case SYS_USB_CONNECTED:
                rb->usb_screen();
                return PLUGIN_USB_CONNECTED;
                break;
        }
    }
}
#endif /* #ifdef HAVE_LCD_BITMAP */
#endif /* #ifndef SIMULATOR */
