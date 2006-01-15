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

#ifndef SIMULATOR /* not for simulator (grayscale) */
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP /* and also not for the Player */
#include "gray.h"

PLUGIN_HEADER

/******************************* Globals ***********************************/

static struct plugin_api* rb; /* global api struct pointer */
static unsigned char *gbuf;
static unsigned int gbuf_size = 0;

static unsigned char fire[LCD_HEIGHT+3][LCD_WIDTH];
static unsigned char cooling_map[LCD_HEIGHT][LCD_WIDTH];
static unsigned char draw_buffer[8*LCD_WIDTH];

/* Key assignement */
#define FIRE_QUIT BUTTON_OFF
#define FIRE_INCREASE_MULT BUTTON_UP
#define FIRE_DECREASE_MULT BUTTON_DOWN

#if CONFIG_KEYPAD == IRIVER_H100_PAD
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_MODE
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_REC

#elif CONFIG_KEYPAD == RECORDER_PAD
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_ON
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_PLAY

#elif CONFIG_KEYPAD == ONDIO_PAD
#define FIRE_SWITCH_FLAMES_TYPE BUTTON_MENU
#define FIRE_SWITCH_FLAMES_MOVING BUTTON_RIGHT

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

static unsigned char palette[255]=
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
}

void cleanup(void *parameter)
{
    (void)parameter;
    
#if !defined(SIMULATOR) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(false);
#endif
    gray_release();
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
}

/*
 * Main function that also contain the main plasma
 * algorithm.
 */

int main(void)
{
    int shades, button;
    int mult = 261;
    int flames_type=0;
    bool moving=true;
    /* get the remainder of the plugin buffer */
    gbuf = (unsigned char *) rb->plugin_get_buffer(&gbuf_size);

    shades = gray_init(rb, gbuf, gbuf_size, false, LCD_WIDTH, LCD_HEIGHT/8,
                       32, NULL) + 1;
    if(shades <= 1)
    {
        rb->splash(HZ, true, "not enougth memory");
        return PLUGIN_ERROR;
    }

#if !defined(SIMULATOR) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(true);
#endif
    /* switch on grayscale overlay */
    gray_show(true);
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

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR
