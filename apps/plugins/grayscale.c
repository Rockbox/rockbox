/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Grayscale demo plugin
*
* Copyright (C) 2004 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#ifndef SIMULATOR /* not for simulator by now */
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP /* and also not for the Player */
#include "gray.h"

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define GRAYSCALE_SHIFT BUTTON_ON
#elif CONFIG_KEYPAD == ONDIO_PAD
#define GRAYSCALE_SHIFT BUTTON_MENU
#endif
/******************************* Globals ***********************************/

static struct plugin_api* rb; /* global api struct pointer */
static char pbuf[32];         /* global printf buffer */
static unsigned char *gbuf;
static unsigned int gbuf_size = 0;

/**************************** main function ********************************/

void cleanup(void *parameter)
{
    (void)parameter;

    gray_release_buffer(); /* switch off overlay and deinitialize */
    /* restore normal backlight setting */
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
}

/* this is only a demo of what the framework can do */
int main(void)
{
    int shades, time;
    int x, y, i;
    int button, scroll_amount;
    bool black_border;

    static const unsigned char rockbox[] = {
    /* ...........................................
     * .####...###...###..#...#.####...###..#...#.
     * .#...#.#...#.#...#.#..#..#...#.#...#..#.#..
     * .####..#...#.#.....###...####..#...#...#...
     * .#..#..#...#.#...#.#..#..#...#.#...#..#.#..
     * .#...#..###...###..#...#.####...###..#...#.
     * ...........................................
     * 43 x 7 pixel, 1 bpp
     */
       0x00, 0x3E, 0x0A, 0x0A, 0x1A, 0x24, 0x00, 0x1C, 0x22, 0x22,
       0x22, 0x1C, 0x00, 0x1C, 0x22, 0x22, 0x22, 0x14, 0x00, 0x3E,
       0x08, 0x08, 0x14, 0x22, 0x00, 0x3E, 0x2A, 0x2A, 0x2A, 0x14,
       0x00, 0x1C, 0x22, 0x22, 0x22, 0x1C, 0x00, 0x22, 0x14, 0x08,
       0x14, 0x22, 0x00
    };
    
    static const unsigned char showing[] = {
    /* .......................................
     * ..####.#...#..###..#...#.#.#...#..####.
     * .#.....#...#.#...#.#...#.#.##..#.#.....
     * ..###..#####.#...#.#.#.#.#.#.#.#.#..##.
     * .....#.#...#.#...#.#.#.#.#.#..##.#...#.
     * .####..#...#..###...#.#..#.#...#..####.
     * .......................................
     * 39 x 7 pixel, 1 bpp
     */
       0x00, 0x24, 0x2A, 0x2A, 0x2A, 0x12, 0x00, 0x3E, 0x08, 0x08,
       0x08, 0x3E, 0x00, 0x1C, 0x22, 0x22, 0x22, 0x1C, 0x00, 0x1E,
       0x20, 0x18, 0x20, 0x1E, 0x00, 0x3E, 0x00, 0x3E, 0x04, 0x08,
       0x10, 0x3E, 0x00, 0x1C, 0x22, 0x22, 0x2A, 0x3A, 0x00
    };
    
    static const unsigned char grayscale_gray[] = {
    /* .......................................................
     * ..####.####...###..#...#..####..###...###..#.....#####.
     * .#.....#...#.#...#.#...#.#.....#...#.#...#.#.....#.....
     * .#..##.####..#####..#.#...###..#.....#####.#.....####..
     * .#...#.#..#..#...#...#.......#.#...#.#...#.#.....#.....
     * ..####.#...#.#...#...#...####...###..#...#.#####.#####.
     * .......................................................
     * 55 x 7 pixel, 8 bpp
     */
       110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,
       110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,
       110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,
       110,110,110,110,110,110,110,
       120,120, 20, 20, 20, 20,120,222,222,222,222,120,120,120, 24, 24,
        24,120,120,226,120,120,120,226,120,120, 28, 28, 28, 28,120,120,
       230,230,230,120,120,120, 32, 32, 32,120,120,234,120,120,120,120,
       120, 36, 36, 36, 36, 36,120,
       130, 20,130,130,130,130,130,222,130,130,130,222,130, 24,130,130,
       130, 24,130,226,130,130,130,226,130, 28,130,130,130,130,130,230,
       130,130,130,230,130, 32,130,130,130, 32,130,234,130,130,130,130,
       130, 36,130,130,130,130,130,
       140, 20,140,140, 20, 20,140,222,222,222,222,140,140, 24, 24, 24,
        24, 24,140,140,226,140,226,140,140,140, 28, 28, 28,140,140,230,
       140,140,140,140,140, 32, 32, 32, 32, 32,140,234,140,140,140,140,
       140, 36, 36, 36, 36,140,140,
       130, 20,130,130,130, 20,130,222,130,130,222,130,130, 24,130,130,
       130, 24,130,130,130,226,130,130,130,130,130,130,130, 28,130,230,
       130,130,130,230,130, 32,130,130,130, 32,130,234,130,130,130,130,
       130, 36,130,130,130,130,130,
       120,120, 20, 20, 20, 20,120,222,120,120,120,222,120, 24,120,120,
       120, 24,120,120,120,226,120,120,120, 28, 28, 28, 28,120,120,120,
       230,230,230,120,120, 32,120,120,120, 32,120,234,234,234,234,234,
       120, 36, 36, 36, 36, 36,120,
       110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,
       110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,
       110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,
       110,110,110,110,110,110,110
    };

    if (rb->global_settings->backlight_timeout > 0)
        rb->backlight_set_timeout(1); /* keep the light on */

    rb->lcd_setfont(FONT_SYSFIXED);   /* select default font */

    /* get the remainder of the plugin buffer */
    gbuf = (unsigned char *) rb->plugin_get_buffer(&gbuf_size);

    /* initialize the grayscale buffer:
     * 112 pixels wide, 7 rows (56 pixels) high, (try to) reserve
     * 32 bitplanes for 33 shades of gray. (uses 25268 bytes)*/
    shades = gray_init_buffer(gbuf, gbuf_size, 112, 7, 32, NULL) + 1;

    /* place grayscale overlay 1 row down */
    gray_position_display(0, 1);

    rb->snprintf(pbuf, sizeof(pbuf), "Shades: %d", shades);
    rb->lcd_puts(0, 0, pbuf);
    rb->lcd_update();

    gray_show_display(true);  /* switch on grayscale overlay */

    time = *rb->current_tick; /* start time measurement */

    gray_set_foreground(150);
    gray_fillrect(0, 0, 112, 56); /* fill everything with gray 150 */

    /* draw a dark gray line star background */
    gray_set_foreground(80);
    for (y = 0; y < 56; y += 8)        /* horizontal part */
    {
        gray_drawline(0, y, 111, 55 - y); /* gray lines */
    }
    for (x = 10; x < 112; x += 10)     /* vertical part */
    {
        gray_drawline(x, 0, 111 - x, 55); /* gray lines */
    }

    gray_set_foreground(0);
    gray_drawrect(0, 0, 112, 56);   /* black border */

    /* draw gray tones */
    for (i = 0; i < 86; i++)           
    {
        x = 13 + i;
        gray_set_foreground(3 * i);
        gray_verline(x, 6, 49);     /* vertical lines */
    }

    gray_set_drawmode(GRAY_DRAW_INVERSE);
    gray_fillrect(13, 29, 86, 21);   /* invert rectangle (lower half) */
    gray_drawline(13, 27, 98, 27);   /* invert a line */

    /* show bitmaps (1 bit and 8 bit) */
    gray_set_drawinfo(GRAY_DRAW_SOLID, 255, 100);
    gray_drawbitmap(rockbox, 14, 13, 43, 7, 43);   /* opaque */
    gray_set_drawinfo(GRAY_DRAW_FG, 0, -1);
    gray_drawbitmap(showing, 58, 13, 39, 7, 39); /* transparent */

    gray_drawgraymap(grayscale_gray, 28, 35, 55, 7, 55);

    time = *rb->current_tick - time;  /* end time measurement */

    rb->snprintf(pbuf, sizeof(pbuf), "Shades: %d, %d.%02ds", shades,
                 time / 100, time % 100);
    rb->lcd_puts(0, 0, pbuf);
    gray_deferred_update();             /* schedule an lcd_update() */

    /* drawing is now finished, play around with scrolling 
     * until you press OFF or connect USB
     */
    while (true)
    {
        scroll_amount = 1;
        black_border = false;

        button = rb->button_get(true);

        if (rb->default_event_handler_ex(button, cleanup, NULL) 
            == SYS_USB_CONNECTED)
            return PLUGIN_USB_CONNECTED;

        if (button & GRAYSCALE_SHIFT)
            black_border = true;
            
        if (button & BUTTON_REPEAT)
            scroll_amount = 4;

        switch(button & ~(GRAYSCALE_SHIFT | BUTTON_REPEAT))
        {
            case BUTTON_LEFT:

                gray_scroll_left(scroll_amount, black_border); /* scroll left */
                break;

            case BUTTON_RIGHT:

                gray_scroll_right(scroll_amount, black_border); /* scroll right */
                break;

            case BUTTON_UP:

                gray_scroll_up(scroll_amount, black_border); /* scroll up */
                break;

            case BUTTON_DOWN:

                gray_scroll_down(scroll_amount, black_border); /* scroll down */
                break;

            case BUTTON_OFF:

                cleanup(NULL);
                return PLUGIN_OK;
        }
    }
}

/*************************** Plugin entry point ****************************/

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    /* this macro should be called as the first thing you do in the plugin.
    it test that the api version and model the plugin was compiled for
    matches the machine it is running on */
    TEST_PLUGIN_API(api);

    rb = api; // copy to global api pointer
    (void)parameter;

    /* This plugin uses the grayscale framework, so initialize */
    gray_init(api);

    return main();
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR

