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
#ifndef SIMULATOR      /* The simulator dosen't have a MAS */
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP

static struct plugin_api* rb;

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    TEST_PLUGIN_API(api);
    (void) parameter;
    rb = api;

    /*
       I hope to make (left/right)_needle_top_y change some day (because it looks
       like it is stretching) so that is why it is a int and not a #define.
    */
    #define LEFT_NEEDLE_BOTTOM_X 28
    #define LEFT_NEEDLE_BOTTOM_Y 53
    int left_needle_top_x;
    int left_needle_top_y = 18;

    #define RIGHT_NEEDLE_BOTTOM_X 84
    #define RIGHT_NEEDLE_BOTTOM_Y 53
    int right_needle_top_x;
    int right_needle_top_y = 18;

    while (!PLUGIN_OK)
    {
        /* These are to define how far the tip of the needles can go to the
           left and right. The names are a bit confusing. The LEFT/RIGHT tells
           which needle it is for, and the L/R at the end tells which side
           of the needle. */
        #define MAX_LEFT_L 2
        #define MAX_LEFT_R 55
        #define MAX_RIGHT_L 57
        #define MAX_RIGHT_R 111

        #define MAX_PEAK 0x7FFF

        left_needle_top_x =
            (rb->mas_codec_readreg(0xC) *
             (MAX_LEFT_R - MAX_LEFT_L) / MAX_PEAK) + MAX_LEFT_L;
        
        right_needle_top_x =
            (rb->mas_codec_readreg(0xD) *
             (MAX_RIGHT_R - MAX_RIGHT_L) / MAX_PEAK) + MAX_RIGHT_L;

        /* Time to draw all of the display stuff!
           Could I move some of these out of the loop so they don't have to
           be re-drawn everytime, but still be displayed, or would that
           improve performance any at all? */
        rb->lcd_clear_display();

        rb->lcd_drawline(LEFT_NEEDLE_BOTTOM_X, LEFT_NEEDLE_BOTTOM_Y,
                         left_needle_top_x, left_needle_top_y);
        rb->lcd_drawline(RIGHT_NEEDLE_BOTTOM_X, RIGHT_NEEDLE_BOTTOM_Y,
                         right_needle_top_x, right_needle_top_y);

        rb->lcd_setfont(FONT_SYSFIXED);
        rb->lcd_putsxy(30, 1, "VU Meter");

        /* The first is the line under "VU Meter" and the second is under
           the needles. */
        rb->lcd_drawline(30, 9, 77, 9);
        rb->lcd_drawline(1, 53, 112, 53);

        /* These are the needle "covers" - we're going for that
           "old fashioned" look */

        /* The left needle cover - organized from the top line to the bottom */
        rb->lcd_drawline(27, 48, 29, 48);
        rb->lcd_drawline(25, 49, 31, 49);
        rb->lcd_drawline(23, 50, 33, 50);
        rb->lcd_drawline(22, 51, 34, 51);
        rb->lcd_drawline(22, 52, 34, 52);

        /* The right needle cover - organized from the top line to
           the bottom */
        rb->lcd_drawline(83, 48, 85, 48);
        rb->lcd_drawline(81, 49, 87, 49);
        rb->lcd_drawline(79, 50, 89, 50);
        rb->lcd_drawline(78, 51, 90, 51);
        rb->lcd_drawline(78, 52, 90, 52);

        rb->lcd_update();

        /* We must yield once in a while to make sure that the MPEG thread
           isn't starved, but we use the shortest possible timeout for best
           performance */
        switch (rb->button_get_w_tmo(HZ/HZ))
        {
            case BUTTON_OFF:
                return PLUGIN_OK;
        }
    }
}
#endif /* HAVE_LCD_BITMAP */
#endif /* #ifndef SIMULATOR */
