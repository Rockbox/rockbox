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
#include "helper.h"

#ifdef HAVE_LCD_BITMAP /* and also not for the Player */
#ifndef HAVE_LCD_COLOR
#include "grey.h"
#endif

#if (LCD_WIDTH == 112) && (LCD_HEIGHT == 64)
/* Archos has not enough plugin RAM for full-width fire :( */
#define FIRE_WIDTH 108
#define FIRE_XPOS 2
#else
#define FIRE_WIDTH LCD_WIDTH
#define FIRE_XPOS 0
#endif

PLUGIN_HEADER

/******************************* Globals ***********************************/

static struct plugin_api* rb; /* global api struct pointer */

static unsigned char fire[LCD_HEIGHT+3][FIRE_WIDTH];
static unsigned char cooling_map[LCD_HEIGHT][FIRE_WIDTH];

#ifndef HAVE_LCD_COLOR
GREY_INFO_STRUCT
static unsigned char *gbuf;
static size_t gbuf_size = 0;
static unsigned char draw_buffer[FIRE_WIDTH];
#endif

/* Key assignement */

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define FIRE_QUIT BUTTON_OFF
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_MODE
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_REC
#define FIRE_INCREASE_MULT BUTTON_UP
#define FIRE_DECREASE_MULT BUTTON_DOWN

#define FIRE_RC_QUIT BUTTON_RC_STOP

#elif CONFIG_KEYPAD == RECORDER_PAD
#define FIRE_QUIT BUTTON_OFF
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_ON
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_PLAY
#define FIRE_INCREASE_MULT BUTTON_UP
#define FIRE_DECREASE_MULT BUTTON_DOWN

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define FIRE_QUIT BUTTON_OFF
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_ON
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_SELECT
#define FIRE_INCREASE_MULT BUTTON_UP
#define FIRE_DECREASE_MULT BUTTON_DOWN

#elif CONFIG_KEYPAD == ONDIO_PAD
#define FIRE_QUIT BUTTON_OFF
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_MENU
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_RIGHT
#define FIRE_INCREASE_MULT BUTTON_UP
#define FIRE_DECREASE_MULT BUTTON_DOWN

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define FIRE_QUIT BUTTON_MENU
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_SELECT
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_RIGHT
#define FIRE_INCREASE_MULT BUTTON_SCROLL_FWD
#define FIRE_DECREASE_MULT BUTTON_SCROLL_BACK

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define FIRE_QUIT BUTTON_POWER
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_LEFT
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_RIGHT
#define FIRE_INCREASE_MULT BUTTON_UP
#define FIRE_DECREASE_MULT BUTTON_DOWN

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define FIRE_QUIT BUTTON_POWER
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_LEFT
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_RIGHT
#define FIRE_INCREASE_MULT BUTTON_UP
#define FIRE_DECREASE_MULT BUTTON_DOWN

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
(CONFIG_KEYPAD == SANSA_C200_PAD)
#define FIRE_QUIT BUTTON_POWER
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_LEFT
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_RIGHT
#define FIRE_INCREASE_MULT BUTTON_UP
#define FIRE_DECREASE_MULT BUTTON_DOWN

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define FIRE_QUIT BUTTON_POWER
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_LEFT
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_RIGHT
#define FIRE_INCREASE_MULT BUTTON_SCROLL_UP
#define FIRE_DECREASE_MULT BUTTON_SCROLL_DOWN

#elif (CONFIG_KEYPAD == IRIVER_IFP7XX_PAD)
#define FIRE_QUIT BUTTON_PLAY
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_MODE
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_EQ
#define FIRE_INCREASE_MULT BUTTON_UP
#define FIRE_DECREASE_MULT BUTTON_DOWN

#endif

#define MIN_FLAME_VALUE 0
#define COOL_MAX (440/LCD_HEIGHT+2)

/* fast unsigned multiplication (16x16bit->32bit or 32x32bit->32bit,
 * whichever is faster for the architecture) */
#ifdef CPU_ARM
#define FMULU(a, b) ((uint32_t) (((uint32_t) (a)) * ((uint32_t) (b))))
#else /* SH1, coldfire */
#define FMULU(a, b) ((uint32_t) (((uint16_t) (a)) * ((uint16_t) (b))))
#endif

#ifndef HAVE_LCD_COLOR
static const unsigned char palette[256] = {
      0,   1,   3,   4,   6,   7,   9,  10,  12,  13,  15,  16,  18,  19,  21,  22,
     24,  25,  27,  28,  30,  31,  33,  34,  36,  37,  39,  40,  42,  43,  45,  46,
     48,  49,  51,  52,  54,  55,  57,  59,  60,  61,  63,  64,  66,  67,  69,  70,
     72,  73,  75,  76,  78,  79,  81,  82,  84,  85,  87,  88,  90,  91,  93,  94,
     96,  97,  99, 100, 102, 103, 105, 106, 108, 109, 111, 112, 114, 115, 117, 118,
    120, 121, 123, 124, 126, 127, 129, 130, 132, 133, 135, 136, 138, 139, 141, 142,
    144, 145, 147, 148, 150, 151, 153, 154, 156, 157, 159, 160, 162, 163, 165, 166,
    168, 169, 171, 172, 174, 175, 177, 178, 180, 181, 183, 184, 186, 187, 189, 190,
    192, 193, 195, 196, 198, 199, 201, 202, 204, 205, 207, 208, 210, 211, 213, 214,
    216, 217, 219, 220, 222, 223, 225, 226, 228, 229, 231, 232, 234, 235, 237, 238,
    /* 'regular' fire doesn't exceed this value */
    240, 240, 240, 240, 240, 240, 241, 241, 241, 241, 241, 241, 242, 242, 242, 242,
    242, 242, 243, 243, 243, 243, 243, 243, 244, 244, 244, 244, 244, 244, 245, 245,
    245, 245, 245, 245, 246, 246, 246, 246, 246, 246, 247, 247, 247, 247, 247, 247,
    248, 248, 248, 248, 248, 248, 249, 249, 249, 249, 249, 249, 250, 250, 250, 250,
    250, 250, 251, 251, 251, 251, 251, 251, 252, 252, 252, 252, 252, 252, 253, 253,
    253, 253, 253, 253, 254, 254, 254, 254, 254, 254, 255, 255, 255, 255, 255, 255
};
#else
#define L(r,g,b) LCD_RGBPACK(r,g,b)

static const fb_data colorpalette[256] = {
    L(  0,  0,  0), L(  5,  0,  0), L( 10,  0,  0), L( 15,  0,  0), L( 20,  0,  0),
    L( 25,  0,  0), L( 30,  0,  0), L( 35,  0,  0), L( 40,  0,  0), L( 45,  0,  0),
    L( 50,  0,  0), L( 55,  0,  0), L( 60,  0,  0), L( 65,  0,  0), L( 70,  0,  0),
    L( 75,  0,  0), L( 80,  0,  0), L( 85,  0,  0), L( 90,  0,  0), L( 95,  0,  0),
    L(100,  0,  0), L(105,  0,  0), L(110,  0,  0), L(115,  0,  0), L(120,  0,  0),
    L(125,  0,  0), L(130,  0,  0), L(135,  0,  0), L(140,  0,  0), L(145,  0,  0),
    L(150,  0,  0), L(155,  0,  0), L(160,  0,  0), L(165,  0,  0), L(170,  0,  0),
    L(175,  0,  0), L(180,  0,  0), L(185,  0,  0), L(186,  2,  0), L(187,  5,  1),
    L(188,  6,  2), L(189,  8,  3), L(190, 10,  4), L(191, 12,  4), L(192, 14,  5),
    L(193, 15,  6), L(194, 17,  7), L(195, 19,  8), L(196, 21,  8), L(197, 22,  9),
    L(198, 24, 10), L(198, 26, 10), L(199, 28, 10), L(200, 29, 11), L(201, 32, 11),
    L(202, 34, 12), L(203, 36, 13), L(204, 39, 13), L(205, 40, 14), L(206, 42, 14),
    L(207, 43, 15), L(208, 46, 16), L(208, 48, 17), L(209, 50, 18), L(210, 51, 18),
    L(212, 53, 19), L(213, 55, 20), L(214, 57, 21), L(215, 59, 22), L(216, 61, 22),
    L(217, 63, 23), L(217, 65, 24), L(218, 67, 24), L(219, 69, 25), L(220, 70, 25),
    L(222, 72, 26), L(223, 74, 27), L(224, 76, 28), L(225, 78, 28), L(225, 80, 29),
    L(226, 82, 29), L(227, 84, 30), L(228, 86, 31), L(228, 88, 32), L(229, 90, 32),
    L(230, 92, 32), L(231, 94, 33), L(232, 96, 34), L(233, 98, 35), L(234, 99, 36),
    L(235,101, 37), L(236,103, 37), L(237,105, 38), L(238,107, 39), L(240,109, 39),
    L(240,111, 40), L(241,113, 40), L(242,114, 41), L(242,116, 42), L(243,119, 43),
    L(245,120, 43), L(246,122, 44), L(247,124, 44), L(248,126, 46), L(248,128, 47),
    L(249,130, 47), L(251,131, 47), L(252,133, 48), L(253,135, 49), L(253,138, 49),
    L(253,140, 50), L(254,143, 49), L(254,145, 48), L(253,148, 47), L(253,151, 46),
    L(253,154, 45), L(253,156, 44), L(253,158, 44), L(252,162, 42), L(252,165, 41),
    L(252,167, 41), L(252,170, 40), L(251,173, 40), L(252,175, 38), L(252,178, 37),
    L(251,181, 37), L(251,183, 36), L(251,186, 35), L(250,189, 34), L(250,192, 33),
    L(250,194, 33), L(249,198, 31), L(249,200, 31), L(249,203, 30), L(249,206, 29),
    L(249,209, 28), L(249,211, 27), L(249,214, 26), L(248,216, 25), L(248,219, 25),
    L(248,222, 24), L(248,224, 23), L(248,227, 22), L(248,230, 21), L(248,232, 21),
    L(247,236, 19), L(247,238, 23), L(247,239, 31), L(247,239, 45), L(247,240, 55),
    L(248,240, 68), L(247,241, 78), L(248,241, 90), L(247,242,102), L(248,242,114),
    L(247,243,125), L(248,243,138), L(248,243,153), L(248,244,162), L(248,245,174),
    /* 'regular' fire doesn't exceed this value */
    L(247,245,182), L(247,245,182), L(247,245,183), L(247,245,183), L(247,245,184),
    L(247,245,184), L(247,245,185), L(247,245,185), L(247,245,186), L(247,245,186),
    L(247,245,187), L(247,245,187), L(247,245,188), L(247,245,188), L(247,245,189),
    L(247,245,189), L(248,245,190), L(248,245,190), L(248,245,191), L(248,245,191),
    L(248,245,192), L(248,245,192), L(248,245,193), L(248,245,193), L(248,245,194),
    L(248,245,194), L(248,245,195), L(248,245,195), L(248,245,196), L(248,245,196),
    L(248,245,197), L(248,245,197), L(248,245,198), L(248,245,198), L(248,245,199),
    L(248,245,199), L(248,245,200), L(248,245,200), L(248,245,201), L(248,245,201),
    L(248,245,202), L(248,245,202), L(248,245,203), L(248,245,203), L(248,245,204),
    L(248,245,204), L(248,245,205), L(248,245,205), L(248,246,206), L(248,246,206),
    L(248,246,207), L(248,246,207), L(248,246,208), L(248,246,208), L(248,246,209),
    L(248,246,209), L(248,246,210), L(248,246,210), L(248,246,211), L(248,246,211),
    L(248,246,212), L(248,246,212), L(248,246,213), L(248,246,213), L(248,246,214),
    L(248,246,214), L(248,246,215), L(248,246,215), L(248,246,216), L(248,246,216),
    L(248,246,217), L(248,246,217), L(248,246,218), L(248,246,218), L(248,246,219),
    L(248,246,219), L(248,246,220), L(248,246,220), L(248,246,221), L(248,246,221),
    L(248,246,222), L(248,246,222), L(248,246,223), L(248,246,223), L(248,246,224),
    L(248,246,224), L(248,246,225), L(248,246,225), L(248,246,226), L(248,246,226),
    L(248,246,227), L(248,246,227), L(248,246,228), L(248,246,228), L(248,246,229),
    L(248,246,229)
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
        end = ptr + FIRE_WIDTH;

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
    end = ptr + LCD_HEIGHT*FIRE_WIDTH;

    switch (flames_type)
    {
      case 0:
        do
        {
            pixel_value = ptr[FIRE_WIDTH-1]  /* fire[y+1][x-1] */
                        + ptr[2*FIRE_WIDTH]  /* fire[y+2][x] */
                        + ptr[FIRE_WIDTH+1]  /* fire[y+1][x+1] */
                        + ptr[3*FIRE_WIDTH]; /* fire[y+3][x] */
            pixel_value = FMULU(pixel_value, mult) >> 10;

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
        mult -= 2; 
        do
        {
            pixel_value = ptr[FIRE_WIDTH-1]  /* fire[y+1][x-1] */
                        + ptr[FIRE_WIDTH]    /* fire[y+1][x] */
                        + ptr[FIRE_WIDTH+1]  /* fire[y+1][x+1] */
                        + ptr[2*FIRE_WIDTH]; /* fire[y+2][x] */
            pixel_value = FMULU(pixel_value, mult) >> 10;

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
    int y;
    unsigned char *src = &fire[0][0];
#ifndef HAVE_LCD_COLOR
    unsigned char *dest, *end;

    for (y = 0; y < LCD_HEIGHT; y++)
    {
        dest = draw_buffer;
        end = dest + FIRE_WIDTH;

        do
            *dest++ = palette[*src++];
        while (dest < end);

        grey_ub_gray_bitmap(draw_buffer, 0, y, FIRE_WIDTH, 1);
    }
#else
    fb_data *dest, *end;

    for (y = 0; y < LCD_HEIGHT; y++)
    {
        dest = rb->lcd_framebuffer + LCD_WIDTH * y + FIRE_XPOS;
        end = dest + FIRE_WIDTH;

        do
            *dest++ = colorpalette[*src++];
        while (dest < end);
    }
    rb->lcd_update();
#endif
}

void cleanup(void *parameter)
{
    (void)parameter;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
#ifndef HAVE_LCD_COLOR
    grey_release();
#endif
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(rb); /* backlight control in lib/helper.c */
}

/*
 * Main function that also contain the main plasma
 * algorithm.
 */

int main(void)
{
    int button;
    int mult = 261;
    int flames_type=0;
    bool moving=true;

#ifndef HAVE_LCD_COLOR
    /* get the remainder of the plugin buffer */
    gbuf = (unsigned char *) rb->plugin_get_buffer(&gbuf_size);

    if (!grey_init(rb, gbuf, gbuf_size, false, FIRE_WIDTH, LCD_HEIGHT, NULL))
    {
        rb->splash(HZ, "not enough memory");
        return PLUGIN_ERROR;
    }
    /* switch on greyscale overlay */
    grey_set_position(FIRE_XPOS, 0);
    grey_show(true);
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    rb->memset(&fire[0][0], 0, sizeof(fire));
    tab_init_rand(&cooling_map[0][0], LCD_HEIGHT*FIRE_WIDTH, COOL_MAX);
    while (true)
    {
        fire_generate(mult, flames_type, moving);
        fire_draw();
        rb->yield();

        button = rb->button_get(false);

        switch(button)
        {
#ifdef FIRE_RC_QUIT
            case FIRE_RC_QUIT :
#endif
            case (FIRE_QUIT):
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
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    /* Turn off backlight timeout */
    backlight_force_on(rb); /* backlight control in lib/helper.c */

    ret = main();

    return ret;
}

#endif /* #ifdef HAVE_LCD_BITMAP */
