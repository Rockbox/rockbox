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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "grey.h"
#include "helper.h"

PLUGIN_HEADER

#if (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) \
 || (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define GREY_QUIT BUTTON_MENU
#define GREY_OK   BUTTON_SELECT
#define GREY_PREV BUTTON_LEFT
#define GREY_NEXT BUTTON_RIGHT
#define GREY_UP   BUTTON_SCROLL_FWD
#define GREY_DOWN BUTTON_SCROLL_BACK

#elif CONFIG_KEYPAD == IRIVER_H100_PAD
#define GREY_QUIT BUTTON_OFF
#define GREY_OK   BUTTON_SELECT
#define GREY_PREV BUTTON_LEFT
#define GREY_NEXT BUTTON_RIGHT
#define GREY_UP   BUTTON_UP
#define GREY_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == RECORDER_PAD
#define GREY_QUIT BUTTON_OFF
#define GREY_OK   BUTTON_PLAY
#define GREY_PREV BUTTON_LEFT
#define GREY_NEXT BUTTON_RIGHT
#define GREY_UP   BUTTON_UP
#define GREY_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == ONDIO_PAD
#define GREY_QUIT BUTTON_OFF
#define GREY_OK   BUTTON_MENU
#define GREY_PREV BUTTON_LEFT
#define GREY_NEXT BUTTON_RIGHT
#define GREY_UP   BUTTON_UP
#define GREY_DOWN BUTTON_DOWN

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD) \
   || (CONFIG_KEYPAD == MROBE100_PAD)
#define GREY_QUIT BUTTON_POWER
#define GREY_OK   BUTTON_SELECT
#define GREY_PREV BUTTON_LEFT
#define GREY_NEXT BUTTON_RIGHT
#define GREY_UP   BUTTON_UP
#define GREY_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define GREY_QUIT BUTTON_RC_REC
#define GREY_OK   BUTTON_RC_PLAY
#define GREY_PREV BUTTON_RC_REW
#define GREY_NEXT BUTTON_RC_FF
#define GREY_UP   BUTTON_RC_VOL_UP
#define GREY_DOWN BUTTON_RC_VOL_DOWN

#else
#error unsupported keypad
#endif

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

static const struct plugin_api* rb;
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
enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    bool done = false;
    int cur_step = 1;
    int button, i, l, fd;
    unsigned char filename[MAX_PATH];

    /* standard stuff */
    (void)parameter;
    rb = api;
    
    gbuf = (unsigned char *) rb->plugin_get_buffer(&gbuf_size);

    if (!grey_init(rb, gbuf, gbuf_size,
                   GREY_BUFFERED|GREY_RAWMAPPED|GREY_ON_COP,
                   LCD_WIDTH, LCD_HEIGHT, NULL))
    {
        rb->splash(HZ, "Not enough memory.");
        return PLUGIN_ERROR;
    }
    for (i = 0; i <= STEPS; i++)
        input_levels[i] = lcd_levels[i] = (255 * i + (STEPS/2)) / STEPS;

    backlight_force_on(rb); /* backlight control in lib/helper.c */

    grey_set_background(0); /* set background to black */
    grey_clear_display();
    grey_show(true);

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

        button = rb->button_get(true);
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
            case GREY_UP|BUTTON_REPEAT:
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
            case GREY_DOWN|BUTTON_REPEAT:
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
                rb->create_numbered_filename(filename, "/", "test_grey_",
                                             ".txt", 2 IF_CNFN_NUM_(, NULL));
                fd = rb->open(filename, O_RDWR|O_CREAT|O_TRUNC);
                if (fd >= 0)
                {
                    for (i = 0; i <= STEPS; i++)
                         rb->fdprintf(fd, "%3d: %3d\n", input_levels[i],
                                      lcd_levels[i]);
                    rb->close(fd);
                }
                /* fall through */

            case GREY_QUIT:
                done = true;
                break;
        }
    }

    grey_release();
    backlight_use_settings(rb); /* backlight control in lib/helper.c */
    return PLUGIN_OK;
}
