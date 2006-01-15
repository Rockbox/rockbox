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

#if defined(HAVE_LCD_BITMAP) && (LCD_DEPTH < 4)
#include "gray.h"

PLUGIN_HEADER

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define GRAYSCALE_SHIFT BUTTON_ON
#elif CONFIG_KEYPAD == ONDIO_PAD
#define GRAYSCALE_SHIFT BUTTON_MENU
#elif CONFIG_KEYPAD == IRIVER_H100_PAD
#define GRAYSCALE_SHIFT BUTTON_ON
#endif

#define GFX_HEIGHT (LCD_HEIGHT-8)
#if LCD_WIDTH < 160
#define GFX_GRAYTONE_WIDTH 86
#define GFX_GRAYTONE_STEP 3
#else
#define GFX_GRAYTONE_WIDTH 128
#define GFX_GRAYTONE_STEP 2
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

    gray_release(); /* switch off overlay and deinitialize */
    /* restore normal backlight setting */
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
}

/* this is only a demo of what the framework can do */
int main(void)
{
    int shades, time;
    int x, y, i;
    int button, scroll_amount;
    bool black_border = false;

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

    /* initialize the greyscale buffer:
       Archos: 112 pixels wide, 7 rows (56 pixels) high, (try to) reserve
       32 bitplanes for 33 shades of grey.
       H1x0: 160 pixels wide, 30 rows (120 pixels) high, (try to) reserve
       32 bitplanes for 33 shades of grey. */
    shades = gray_init(rb, gbuf, gbuf_size, true, LCD_WIDTH, GFX_HEIGHT/8,
                       32, NULL) + 1;

    /* place greyscale overlay 1 row down */
    gray_set_position(0, 1);

    rb->snprintf(pbuf, sizeof(pbuf), "Shades: %d", shades);
    rb->lcd_puts(0, 0, pbuf);
    rb->lcd_update();

#if !defined(SIMULATOR) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(true);
#endif
    gray_show(true);          /* switch on greyscale overlay */

    time = *rb->current_tick; /* start time measurement */

    gray_set_background(150);
    gray_clear_display();     /* fill everything with grey 150 */

    /* draw a dark grey line star background */
    gray_set_foreground(80);
    for (y = 0; y < GFX_HEIGHT; y += 8)                   /* horizontal part */
    {
        gray_drawline(0, y, (LCD_WIDTH-1), (GFX_HEIGHT-1) - y); /*grey lines */
    }
    for (x = 10; x <= LCD_WIDTH; x += 10)                    /* vertical part */
    {
        gray_drawline(x, 0, (LCD_WIDTH-1) - x, (GFX_HEIGHT-1)); /*grey lines */
    }

    gray_set_foreground(0);
    gray_drawrect(0, 0, LCD_WIDTH, GFX_HEIGHT);   /* black border */

    /* draw grey tones */
    for (i = 0; i < GFX_GRAYTONE_WIDTH; i++)
    {
        x = ((LCD_WIDTH-GFX_GRAYTONE_WIDTH)/2) + i;
        gray_set_foreground(GFX_GRAYTONE_STEP * i);
        /* vertical lines */
        gray_vline(x, (GFX_HEIGHT/8), (GFX_HEIGHT-GFX_HEIGHT/8-1));
    }

    gray_set_drawmode(DRMODE_COMPLEMENT);
    /* invert rectangle (lower half) */
    gray_fillrect((LCD_WIDTH-GFX_GRAYTONE_WIDTH)/2, (GFX_HEIGHT/2+1),
                  GFX_GRAYTONE_WIDTH, (GFX_HEIGHT/2-GFX_HEIGHT/8-1));
    /* invert a line */
    gray_hline((LCD_WIDTH-GFX_GRAYTONE_WIDTH)/2,
               (LCD_WIDTH+GFX_GRAYTONE_WIDTH)/2, (GFX_HEIGHT/2-1));

    /* show bitmaps (1 bit and 8 bit) */
    /* opaque */
    gray_set_drawinfo(DRMODE_SOLID, 255, 100);
    gray_mono_bitmap(rockbox,
                     MAX((LCD_WIDTH/2-47), ((LCD_WIDTH-GFX_GRAYTONE_WIDTH)/2)),
                     (5*GFX_HEIGHT/16-4), 43, 7);
    /* transparent */
    gray_set_drawinfo(DRMODE_FG, 0, 100);
    gray_mono_bitmap(showing, (LCD_WIDTH/2+4) , (5*GFX_HEIGHT/16-4), 39, 7);
    /* greyscale */
    gray_gray_bitmap(grayscale_gray, ((LCD_WIDTH-55)/2), (11*GFX_HEIGHT/16-4),
                     55, 7);

    gray_update();

    time = *rb->current_tick - time;  /* end time measurement */

    rb->snprintf(pbuf, sizeof(pbuf), "Shades: %d, %d.%02ds", shades,
                 time / 100, time % 100);
    rb->lcd_puts(0, 0, pbuf);
    gray_deferred_lcd_update();       /* schedule an lcd_update() */
#if !defined(SIMULATOR) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(false);
#endif

    /* drawing is now finished, play around with scrolling 
     * until you press OFF or connect USB
     */
    gray_set_background(255);
    while (true)
    {
        scroll_amount = 1;

        button = rb->button_get(true);

        if (rb->default_event_handler_ex(button, cleanup, NULL) 
            == SYS_USB_CONNECTED)
            return PLUGIN_USB_CONNECTED;

        if (button & GRAYSCALE_SHIFT)
        {
            if (!black_border)
            {
                gray_set_background(0);
                black_border = true;
            }
        }
        else
        {
            if (black_border)
            {
                gray_set_background(255);
                black_border = false;
            }
        }

        if (button & BUTTON_REPEAT)
            scroll_amount = 4;

        switch (button & ~(GRAYSCALE_SHIFT | BUTTON_REPEAT))
        {
            case BUTTON_LEFT:

                gray_scroll_left(scroll_amount);  /* scroll left */
                gray_update();
                break;

            case BUTTON_RIGHT:

                gray_scroll_right(scroll_amount); /* scroll right */
                gray_update();
                break;

            case BUTTON_UP:

                gray_scroll_up(scroll_amount);    /* scroll up */
                gray_update();
                break;

            case BUTTON_DOWN:

                gray_scroll_down(scroll_amount);  /* scroll down */
                gray_update();
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
    rb = api; // copy to global api pointer
    (void)parameter;

    return main();
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR

