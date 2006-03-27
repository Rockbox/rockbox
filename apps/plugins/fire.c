/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2005 Kevin Ferrare
*
* Fire demo plugin
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "plugin.h"

#ifdef HAVE_LCD_BITMAP /* and also not for the Player */
#ifndef HAVE_LCD_COLOR
#include "gray.h"
#endif

PLUGIN_HEADER

/******************************* Globals ***********************************/

static struct plugin_api* rb; /* global api struct pointer */
static unsigned char *gbuf;
static unsigned int gbuf_size = 0;

static unsigned char fire[LCD_HEIGHT+3][LCD_WIDTH];
static unsigned char cooling_map[LCD_HEIGHT][LCD_WIDTH];
#ifndef HAVE_LCD_COLOR
static unsigned char draw_buffer[8*LCD_WIDTH];
#endif

/* Key assignement */

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define FIRE_QUIT BUTTON_OFF
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_MODE
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_REC
#define FIRE_INCREASE_MULT BUTTON_UP
#define FIRE_DECREASE_MULT BUTTON_DOWN

#elif CONFIG_KEYPAD == RECORDER_PAD
#define FIRE_QUIT BUTTON_OFF
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_ON
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_PLAY
#define FIRE_INCREASE_MULT BUTTON_UP
#define FIRE_DECREASE_MULT BUTTON_DOWN

#elif CONFIG_KEYPAD == ONDIO_PAD
#define FIRE_QUIT BUTTON_OFF
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_MENU
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_RIGHT
#define FIRE_INCREASE_MULT BUTTON_UP
#define FIRE_DECREASE_MULT BUTTON_DOWN

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
#define FIRE_QUIT BUTTON_MENU
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_SELECT
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_RIGHT
#define FIRE_INCREASE_MULT BUTTON_SCROLL_FWD
#define FIRE_DECREASE_MULT BUTTON_SCROLL_BACK

#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#define FIRE_QUIT BUTTON_POWER
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_LEFT
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_RIGHT
#define FIRE_INCREASE_MULT BUTTON_UP
#define FIRE_DECREASE_MULT BUTTON_DOWN

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define FIRE_QUIT BUTTON_A
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_LEFT
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_RIGHT
#define FIRE_INCREASE_MULT BUTTON_UP
#define FIRE_DECREASE_MULT BUTTON_DOWN

#endif

#define MIN_FLAME_VALUE 0
#if LCD_HEIGHT > 64
#define COOL_MAX 5
#else
#define COOL_MAX 8
#endif

/* unsigned 16 bit multiplication (a single instruction on the SH) */
#define MULU16(a, b) ((unsigned long) \
                     (((unsigned short) (a)) * ((unsigned short) (b))))

#ifndef HAVE_LCD_COLOR
static const unsigned char palette[255]=
{/* logarithmic histogram equalisation */

      0,  15,  31,  50,  63,  74,  82,  89,  95, 101, 105, 110, 114, 118, 121,
    124, 127, 130, 133, 135, 137, 140, 142, 144, 146, 148, 149, 151, 153, 154,
    156, 158, 159, 160, 162, 163, 164, 166, 167, 168, 169, 170, 172, 173, 174,
    175, 176, 177, 178, 179, 180, 180, 181, 182, 183, 184, 185, 186, 186, 187,
    188, 189, 189, 190, 191, 192, 192, 193, 194, 194, 195, 196, 196, 197, 198,
    198, 199, 199, 200, 201, 201, 202, 202, 203, 203, 204, 204, 205, 206, 206,
    207, 207, 208, 208, 209, 209, 210, 210, 210, 211, 211, 212, 212, 213, 213,
    214, 214, 215, 215, 215, 216, 216, 217, 217, 217, 218, 218, 219, 219, 219,
    220, 220, 221, 221, 221, 222, 222, 222, 223, 223, 223, 224, 224, 225, 225,
    225, 226, 226, 226, 227, 227, 227, 228, 228, 228, 229, 229, 229, 229, 230,
    230, 230, 231, 231, 231, 232, 232, 232, 232, 233, 233, 233, 234, 234, 234,
    234, 235, 235, 235, 236, 236, 236, 236, 237, 237, 237, 237, 238, 238, 238,
    238, 239, 239, 239, 239, 240, 240, 240, 240, 241, 241, 241, 241, 242, 242,
    242, 242, 243, 243, 243, 243, 244, 244, 244, 244, 244, 245, 245, 245, 245,
    246, 246, 246, 246, 246, 247, 247, 247, 247, 247, 248, 248, 248, 248, 249,
    249, 249, 249, 249, 250, 250, 250, 250, 250, 251, 251, 251, 251, 251, 252,
    252, 252, 252, 252, 252, 253, 253, 253, 253, 253, 254, 254, 254, 254, 254
};
#else
#define L(r,g,b) LCD_RGBPACK(r,g,b)

static const fb_data colorpalette[256] = {
L(248,246,211), L(248,246,203), L(248,245,196), L(248,245,189), L(247,245,182),
L(248,245,174), L(248,244,167), L(248,244,160), L(248,243,153), L(247,243,145),
L(248,243,138), L(247,243,131), L(247,243,125), L(248,242,117), L(248,242,110),
L(247,242,102), L(248,241, 94), L(247,240, 85), L(247,241, 78), L(248,240, 70),
L(247,239, 63), L(247,240, 55), L(247,239, 47), L(247,239, 39), L(247,238, 31),
L(247,238, 23), L(247,236, 19), L(248,232, 21), L(248,228, 22), L(248,224, 23),
L(248,219, 25), L(249,215, 26), L(249,211, 27), L(249,206, 29), L(249,203, 30),
L(249,199, 31), L(250,194, 33), L(250,190, 34), L(251,186, 35), L(251,181, 37),
L(252,177, 38), L(251,172, 40), L(252,168, 41), L(252,163, 42), L(253,158, 44),
L(253,154, 45), L(253,149, 46), L(254,145, 48), L(253,141, 50), L(253,137, 49),
L(253,135, 49), L(252,133, 48), L(251,130, 47), L(249,129, 47), L(248,128, 46),
L(247,125, 46), L(247,124, 44), L(246,122, 44), L(244,119, 43), L(243,118, 43),
L(242,115, 42), L(242,113, 41), L(241,112, 40), L(240,109, 39), L(238,107, 39),
L(237,105, 38), L(236,103, 37), L(235,101, 37), L(234, 99, 36), L(233, 97, 35),
L(232, 95, 34), L(231, 93, 33), L(230, 91, 32), L(229, 89, 32), L(228, 86, 31),
L(227, 85, 30), L(226, 82, 29), L(225, 80, 29), L(224, 78, 28), L(223, 76, 28),
L(222, 74, 26), L(221, 72, 26), L(220, 69, 25), L(218, 67, 24), L(217, 64, 23),
L(215, 60, 22), L(213, 57, 21), L(212, 54, 19), L(210, 50, 18), L(208, 47, 17),
L(207, 44, 16), L(205, 41, 14), L(203, 37, 13), L(202, 34, 12), L(201, 30, 11),
L(199, 27, 10), L(197, 24,  9), L(196, 21,  8), L(194, 18,  7), L(192, 14,  5),
L(191, 11,  4), L(189,  8,  3), L(187,  5,  1), L(186,  2,  0), L(184,  0,  0),
L(181,  0,  0), L(178,  0,  0), L(175,  0,  0), L(172,  0,  0), L(170,  0,  0),
L(167,  0,  0), L(165,  0,  0), L(162,  0,  0), L(159,  0,  0), L(156,  0,  0),
L(153,  0,  0), L(152,  0,  0), L(149,  0,  0), L(146,  0,  0), L(143,  0,  0),
L(141,  0,  0), L(138,  0,  0), L(135,  0,  0), L(132,  0,  0), L(129,  0,  0),
L(127,  0,  0), L(126,  0,  0), L(123,  0,  0), L(120,  0,  0), L(117,  0,  0),
L(115,  0,  0), L(112,  0,  0), L(109,  0,  0), L(106,  0,  0), L(104,  0,  0),
L(101,  0,  0), L( 98,  0,  0), L( 95,  0,  0), L( 93,  0,  0), L( 92,  0,  0),
L( 91,  0,  0), L( 91,  0,  0), L( 90,  0,  0), L( 89,  0,  0), L( 88,  0,  0),
L( 88,  0,  0), L( 87,  0,  0), L( 86,  0,  0), L( 85,  0,  0), L( 84,  0,  0),
L( 84,  0,  0), L( 82,  0,  0), L( 82,  0,  0), L( 81,  0,  0), L( 80,  0,  0),
L( 80,  0,  0), L( 79,  0,  0), L( 78,  0,  0), L( 77,  0,  0), L( 76,  0,  0),
L( 76,  0,  0), L( 75,  0,  0), L( 74,  0,  0), L( 73,  0,  0), L( 72,  0,  0),
L( 72,  0,  0), L( 71,  0,  0), L( 70,  0,  0), L( 69,  0,  0), L( 68,  0,  0),
L( 68,  0,  0), L( 67,  0,  0), L( 66,  0,  0), L( 65,  0,  0), L( 65,  0,  0),
L( 63,  0,  0), L( 62,  0,  0), L( 62,  0,  0), L( 61,  0,  0), L( 60,  0,  0),
L( 59,  0,  0), L( 58,  0,  0), L( 58,  0,  0), L( 58,  0,  0), L( 57,  0,  0),
L( 56,  0,  0), L( 54,  0,  0), L( 54,  0,  0), L( 53,  0,  0), L( 52,  0,  0),
L( 52,  0,  0), L( 51,  0,  0), L( 50,  0,  0), L( 49,  0,  0), L( 48,  0,  0),
L( 47,  0,  0), L( 47,  0,  0), L( 46,  0,  0), L( 45,  0,  0), L( 44,  0,  0),
L( 44,  0,  0), L( 43,  0,  0), L( 42,  0,  0), L( 42,  0,  0), L( 40,  0,  0),
L( 40,  0,  0), L( 39,  0,  0), L( 38,  0,  0), L( 38,  0,  0), L( 37,  0,  0),
L( 36,  0,  0), L( 35,  0,  0), L( 34,  0,  0), L( 33,  0,  0), L( 32,  0,  0),
L( 32,  0,  0), L( 31,  0,  0), L( 30,  0,  0), L( 30,  0,  0), L( 28,  0,  0),
L( 28,  0,  0), L( 27,  0,  0), L( 26,  0,  0), L( 25,  0,  0), L( 25,  0,  0),
L( 23,  0,  0), L( 23,  0,  0), L( 22,  0,  0), L( 21,  0,  0), L( 21,  0,  0),
L( 20,  0,  0), L( 19,  0,  0), L( 18,  0,  0), L( 18,  0,  0), L( 16,  0,  0),
L( 16,  0,  0), L( 15,  0,  0), L( 14,  0,  0), L( 14,  0,  0), L( 13,  0,  0),
L( 12,  0,  0), L( 11,  0,  0), L( 10,  0,  0), L(  9,  0,  0), L(  9,  0,  0),
L(  8,  0,  0), L(  7,  0,  0), L(  6,  0,  0), L(  5,  0,  0), L(  5,  0,  0),
L(  4,  0,  0), L(  3,  0,  0), L(  3,  0,  0), L(  2,  0,  0), L(  1,  0,  0),
L(  0,  0,  0)
};
#endif

static inline void tab_init_rand(unsigned char *tab, unsigned int tab_size,
                                 int rand_max)
{
    unsigned char *end = tab + tab_size;

    while(tab < end)
        *tab++ = (unsigned char)rb->rand() % rand_max;
}

static inline void fire_generate(int mult, int flames_type, bool moving)
{
    unsigned int pixel_value = 0; /* stop the compiler complaining */
    unsigned int cooling_value;
    unsigned char *ptr, *end, *cool;

    /* Randomize the bottom line */
    if(moving)
    {/* moving must be true the first time the function is called */
        ptr = &fire[LCD_HEIGHT][0];
        end = ptr + LCD_WIDTH;

        do
        {
            *ptr++ = (MIN_FLAME_VALUE + rb->rand() % (256-MIN_FLAME_VALUE));
        }
        while (ptr < end);
    }
    rb->yield();

    /* Convolve the pixels and handle cooling (to add nice shapes effects later) */
    cool = &cooling_map[0][0];
    ptr = &fire[0][0];
    end = ptr + LCD_HEIGHT*LCD_WIDTH;
    
    switch (flames_type)
    {
      case 0:
        do
        {
            pixel_value = ptr[LCD_WIDTH-1]  /* fire[y+1][x-1] */
                        + ptr[2*LCD_WIDTH]  /* fire[y+2][x] */
                        + ptr[LCD_WIDTH+1]  /* fire[y+1][x+1] */
                        + ptr[3*LCD_WIDTH]; /* fire[y+3][x] */
            pixel_value =  MULU16(pixel_value, mult) >> 10;

            cooling_value = *cool++;
            if (cooling_value <= pixel_value)
                pixel_value -= cooling_value;
            /* else it's too cold, don't frost the pixels !!! */

            if (pixel_value > 255)
                pixel_value = 255;

            *ptr++ = pixel_value;
        }
        while (ptr < end);
        break;

      case 1:
        do
        {
            pixel_value = ptr[LCD_WIDTH-1]  /* fire[y+1][x-1] */
                        + ptr[LCD_WIDTH]    /* fire[y+1][x] */
                        + ptr[LCD_WIDTH+1]  /* fire[y+1][x+1] */
                        + ptr[2*LCD_WIDTH]; /* fire[y+2][x] */
            pixel_value =  MULU16(pixel_value, mult) >> 10;

            cooling_value = *cool++;
            if (cooling_value <= pixel_value)
                pixel_value -= cooling_value;
            /* else it's too cold, don't frost the pixels !!! */

            if (pixel_value > 255)
                pixel_value = 255;

            *ptr++ = pixel_value;
        }
        while (ptr < end);
        break;

      default: /* We should never reach this */
        break;
    }
    rb->yield();
}

static inline void fire_draw(void)
{
#ifndef HAVE_LCD_COLOR
    int block;
    unsigned char *dest, *end;
    unsigned char *src = &fire[0][0];

    for (block = 0; block < LCD_HEIGHT; block += 8)
    {
        dest = draw_buffer;
        end = dest + 8*LCD_WIDTH;

        do
            *dest++ = palette[*src++];
        while(dest < end);

        gray_ub_gray_bitmap(draw_buffer, 0, block, LCD_WIDTH, 8);
    }
#else
    fb_data* dest = rb->lcd_framebuffer;
    fb_data* end = rb->lcd_framebuffer+(LCD_WIDTH*LCD_HEIGHT);
    unsigned char* src = &fire[0][0];

    while (dest < end)
    {
        *(dest++) = colorpalette[*(src++)];
    }
    rb->lcd_update();
#endif
}

void cleanup(void *parameter)
{
    (void)parameter;
    
#if !defined(SIMULATOR) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(false);
#endif
#ifndef HAVE_LCD_COLOR
    gray_release();
#endif
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
}

/*
 * Main function that also contain the main plasma
 * algorithm.
 */

int main(void)
{
    int shades, button;
#ifdef HAVE_LCD_COLOR
    int mult = 267;
#else
    int mult = 261;
#endif
    int flames_type=0;
    bool moving=true;
    /* get the remainder of the plugin buffer */
    gbuf = (unsigned char *) rb->plugin_get_buffer(&gbuf_size);

#ifdef HAVE_LCD_COLOR
    shades = 256;
#else
    shades = gray_init(rb, gbuf, gbuf_size, false, LCD_WIDTH, LCD_HEIGHT/8,
                       32, NULL) + 1;
    if(shades <= 1)
    {
        rb->splash(HZ, true, "not enough memory");
        return PLUGIN_ERROR;
    }
#endif

#if !defined(SIMULATOR) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(true);
#endif
#ifndef HAVE_LCD_COLOR
    /* switch on grayscale overlay */
    gray_show(true);
#endif
    rb->memset(&fire[0][0], 0, sizeof(fire));
    tab_init_rand(&cooling_map[0][0], LCD_HEIGHT*LCD_WIDTH, COOL_MAX);
    while (true)
    {
        fire_generate(mult, flames_type, moving);
        fire_draw();
        rb->yield();

        button = rb->button_get(false);

        switch(button)
        {
            case(FIRE_QUIT):
                cleanup(NULL);
                return PLUGIN_OK;
                break;

            case (FIRE_INCREASE_MULT):
                ++mult;
                break;

            case (FIRE_DECREASE_MULT):
                if (mult > 0)
                    --mult;
                break;

            case (FIRE_SWITCH_FLAMES_TYPE):
                flames_type = (flames_type + 1) % 2;
                break;

            case (FIRE_SWITCH_FLAMES_MOVING):
                moving = !moving;
                break;

            default:
                if (rb->default_event_handler_ex(button, cleanup, NULL)
                    == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
    }
}

/*************************** Plugin entry point ****************************/

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int ret;

    rb = api; // copy to global api pointer
    (void)parameter;
    if (rb->global_settings->backlight_timeout > 0)
        rb->backlight_set_timeout(1);/* keep the light on */

    ret = main();

    return ret;
}

#endif /* #ifdef HAVE_LCD_BITMAP */
