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
#include "lib/helper.h"
#ifdef HAVE_LCD_BITMAP

#include "lib/pluginlib_actions.h"
#include "lib/fixedpoint.h"

#ifndef HAVE_LCD_COLOR
#include "lib/grey.h"
#endif

#if (LCD_WIDTH == 112) && (LCD_HEIGHT == 64)
/* Archos has not enough plugin RAM for full-width fire :( */
#define FIRE_WIDTH 106
#define FIRE_XPOS 3
#else
#define FIRE_WIDTH LCD_WIDTH
#define FIRE_XPOS 0
#endif

PLUGIN_HEADER

static const struct plugin_api* rb; /* global api struct pointer */

#ifndef HAVE_LCD_COLOR
GREY_INFO_STRUCT
        static unsigned char draw_buffer[FIRE_WIDTH];

#endif

/* Key assignement */
const struct button_mapping* plugin_contexts[]= {
    generic_increase_decrease,
    generic_directions,
#if defined(HAVE_REMOTE_LCD)
    remote_directions,
#endif
    generic_actions
};
#define PLA_ARRAY_COUNT sizeof(plugin_contexts)/sizeof(plugin_contexts[0])

#define FIRE_QUIT PLA_QUIT
#define FIRE_SWITCH_FLAMES_TYPE PLA_LEFT
#define FIRE_SWITCH_FLAMES_MOVING PLA_RIGHT
#define FIRE_INCREASE_MULT PLA_INC
#define FIRE_DECREASE_MULT PLA_DEC

#define MIN_FLAME_VALUE 0
#define COOL_MAX (440/LCD_HEIGHT+2)

#ifndef HAVE_LCD_COLOR
static unsigned char palette[256];

void color_palette_init(unsigned char* palette)
{
    int i;
    for(i=0;i<=160;i++)//palette[i]=(3/2)*i
        palette[i]=(i*3)/2;

    /* 'regular' fire doesn't exceed this value */
    for(;i<=255;i++)//palette[i]=(3/20)*i+216
        palette[i]=(i*3+20*217)/20;
}
#else

static fb_data palette[256];

/*
 * Color palette generation algorithm taken from
 * the "The Demo Effects Collection" GPL project
 * Copyright (C) 2002 W.P. van Paassen
 */
void color_palette_init(fb_data* palette)
{
    int i;
    for (i = 0; i < 32; i++){
        /* black to blue, 32 values*/
        palette[i]=LCD_RGBPACK(0, 0, 2*i);

        /* blue to red, 32 values*/
        palette[i +  32]=LCD_RGBPACK(8*i, 0, 64 - 2*i);

        /* red to yellow, 32 values*/
        palette[i +  64]=LCD_RGBPACK(255, 8*i, 0);

        /* yellow to white, 162 values */
        palette[i +  96]=LCD_RGBPACK(255, 255,   0 + 4*i);
        palette[i + 128]=LCD_RGBPACK(255, 255,  64 + 4*i);
        palette[i + 160]=LCD_RGBPACK(255, 255, 128 + 4*i);
        palette[i + 192]=LCD_RGBPACK(255, 255, 192 + i);
        palette[i + 224]=LCD_RGBPACK(255, 255, 224 + i);
    }
}

#endif

static void tab_init_rand(unsigned char *tab, unsigned int tab_size,
                          int rand_max)
{
    unsigned char *end = tab + tab_size;

    while(tab < end)
        *tab++ = (unsigned char)rb->rand() % rand_max;
}

struct fire {
    unsigned char fire[LCD_HEIGHT+3][FIRE_WIDTH];
    unsigned char cooling_map[LCD_HEIGHT][FIRE_WIDTH];
    int flames_type;
    bool moving;
    unsigned int mult;
};
/* makes the instance a global variable since it's too big to fit on the target's stack */
static struct fire fire;

static inline void fire_convolve(struct fire* fire)
{
    unsigned int pixel_value;
    unsigned int cooling_value;
    unsigned char *ptr, *end, *cool;
    unsigned int mult=fire->mult;

    rb->yield();
    /* Convolve the pixels and handle cooling (to add nice shapes effects later) */
    cool = &fire->cooling_map[0][0];
    ptr = &fire->fire[0][0];
    end = ptr + LCD_HEIGHT*FIRE_WIDTH;

    switch (fire->flames_type){
        case 0:
            do{
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
            }while (ptr < end);
            break;

        case 1:
            mult -= 2; 
            do{
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
            }while (ptr < end);
            break;

            default: /* We should never reach this */
                break;
    }
    rb->yield();
}

static void fire_generate_bottom_seed(struct fire* fire)
{
    unsigned char *ptr, *end;
    ptr = &fire->fire[LCD_HEIGHT][0];
    end = ptr + FIRE_WIDTH;
    do{
        *ptr++ = (MIN_FLAME_VALUE + rb->rand() % (256-MIN_FLAME_VALUE));
    }while (ptr < end);
}

static inline void fire_step(struct fire* fire)
{
    if(fire->moving){
        /* Randomize the bottom line */
        fire_generate_bottom_seed(fire);
        /* Add here further effects like fire letters, ball ... */
    }
    fire_convolve(fire);
}

static void fire_init(struct fire* fire)
{
    fire->mult = 261;
    fire->flames_type=0;
    fire->moving=true;
    rb->memset(&fire->fire[0][0], 0, sizeof(fire->fire));
    tab_init_rand(&fire->cooling_map[0][0], LCD_HEIGHT*FIRE_WIDTH, COOL_MAX);
    fire_generate_bottom_seed(fire);
}

static inline void fire_draw(struct fire* fire)
{
    int y;
    unsigned char *src = &fire->fire[0][0];
#ifndef HAVE_LCD_COLOR
    unsigned char *dest, *end;
#else
    fb_data *dest, *end;
#endif

    for (y = 0; y < LCD_HEIGHT; y++){
#ifndef HAVE_LCD_COLOR
        dest = draw_buffer;
#else
        dest = rb->lcd_framebuffer + LCD_WIDTH * y + FIRE_XPOS;
#endif
        end = dest + FIRE_WIDTH;

        do
            *dest++ = palette[*src++];
        while (dest < end);
#ifndef HAVE_LCD_COLOR
        grey_ub_gray_bitmap(draw_buffer, 0, y, FIRE_WIDTH, 1);
#endif
    }
#ifdef HAVE_LCD_COLOR
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


#ifndef HAVE_LCD_COLOR
int init_grey(void)
{
    unsigned char *gbuf;
    size_t gbuf_size = 0;

    /* get the remainder of the plugin buffer */
    gbuf = (unsigned char *) rb->plugin_get_buffer(&gbuf_size);

    if (!grey_init(rb, gbuf, gbuf_size, GREY_ON_COP,
                   FIRE_WIDTH, LCD_HEIGHT, NULL)){
        rb->splash(HZ, "not enough memory");
        return PLUGIN_ERROR;
    }
    /* switch on greyscale overlay */
    grey_set_position(FIRE_XPOS, 0);
    grey_show(true);
    return PLUGIN_OK;
}
#endif

int main(void)
{
    int action;

#ifndef HAVE_LCD_COLOR
    if(init_grey()!=PLUGIN_OK)
        return(PLUGIN_ERROR);
#endif
    color_palette_init(palette);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    fire_init(&fire);
    while (true){
        fire_step(&fire);
        fire_draw(&fire);
        rb->yield();

        action = pluginlib_getaction(rb, 0, plugin_contexts, PLA_ARRAY_COUNT);

        switch(action){
            case FIRE_QUIT:
                cleanup(NULL);
                return PLUGIN_OK;

            case FIRE_INCREASE_MULT:
                ++fire.mult;
                break;

            case FIRE_DECREASE_MULT:
                if (fire.mult > 0)
                    --fire.mult;
                break;

            case FIRE_SWITCH_FLAMES_TYPE:
                fire.flames_type = (fire.flames_type + 1) % 2;
                break;

            case FIRE_SWITCH_FLAMES_MOVING:
                fire.moving = !fire.moving;
                break;

            default:
                if (rb->default_event_handler_ex(action, cleanup, NULL)
                    == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
        }
    }
}

/*************************** Plugin entry point ****************************/

enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    int ret;

    rb = api; //copy to global api pointer
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
