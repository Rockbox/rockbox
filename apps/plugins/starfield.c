/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* Copyright (C) 2005 Kevin Ferrare
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
#include "lib/pluginlib_exit.h"
#include "lib/pluginlib_actions.h"
/******************************* Globals ***********************************/
/* this set the context to use with PLA */
static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

/* Key assignement */
#define STARFIELD_QUIT                        PLA_EXIT
#define STARFIELD_QUIT2                       PLA_CANCEL
#define STARFIELD_INCREASE_ZMOVE              PLA_UP
#define STARFIELD_INCREASE_ZMOVE_REPEAT       PLA_UP_REPEAT
#define STARFIELD_DECREASE_ZMOVE              PLA_DOWN
#define STARFIELD_DECREASE_ZMOVE_REPEAT       PLA_DOWN_REPEAT
#define STARFIELD_INCREASE_NB_STARS           PLA_RIGHT
#define STARFIELD_INCREASE_NB_STARS_REPEAT    PLA_RIGHT_REPEAT
#define STARFIELD_DECREASE_NB_STARS           PLA_LEFT
#define STARFIELD_DECREASE_NB_STARS_REPEAT    PLA_LEFT_REPEAT
#define STARFIELD_TOGGLE_COLOR                PLA_SELECT

#define LCD_CENTER_X (LCD_WIDTH/2)
#define LCD_CENTER_Y (LCD_HEIGHT/2)
#define Z_MAX_DIST 100

#define MAX_STARS (LCD_WIDTH*LCD_HEIGHT*20)/100
#define INIT_STARS 200
#define STARFIELD_INCREASE_STEP 50
#define INIT_SPACE_SPEED 1
#define STAR_MAX_VELOCITY 2
/*
 * max 3d coord in the 2d screen :
 * example with x
 *  x2d=x3d/z+LCD_CENTER_X (+LCD_CENTER_X to center ...)
 * so
 *  max_x2d=max_x3d/max_z+LCD_CENTER_X
 *  max_x3d=(max_x2d-LCD_CENTER_X)*max_z
 * with
 *  max_x2d = LCD_WIDTH
 *  max_z = Z_MAX_DIST
 * we have now
 * max_x3d=(LCD_WIDTH-LCD_CENTER_X)*Z_MAX_DIST
 * max_x3d=LCD_CENTER_X*Z_MAX_DIST
 */

#define MAX_INIT_STAR_X LCD_CENTER_X*Z_MAX_DIST
#define MAX_INIT_STAR_Y LCD_CENTER_Y*Z_MAX_DIST

#define MSG_DISP_TIME 30

/*
 * Each star's stuffs
 */
struct star
{
    int x,y,z;
    int velocity;
#ifdef HAVE_LCD_COLOR
    int color;
#endif
};

static inline void star_init(struct star * star, int z_move, bool color)
{
    star->velocity=rb->rand() % STAR_MAX_VELOCITY+1;
    /* choose x between -MAX_INIT_STAR_X and MAX_INIT_STAR_X */
    star->x=rb->rand() % (2*MAX_INIT_STAR_X)-MAX_INIT_STAR_X;
    star->y=rb->rand() % (2*MAX_INIT_STAR_Y)-MAX_INIT_STAR_Y;
#ifdef HAVE_LCD_COLOR
    if(color)
        star->color=LCD_RGBPACK(rb->rand()%128+128,rb->rand()%128+128,
                                rb->rand()%128+128);
    else
        star->color=LCD_WHITE;
#else
    (void)color;
#endif
    if(z_move>=0)
        star->z=Z_MAX_DIST;
    else
        star->z=rb->rand() %Z_MAX_DIST/2+1;
}

static inline void star_move(struct star * star, int z_move, bool color)
{
    star->z -= z_move*star->velocity;
    if (star->z <= 0 || star->z > Z_MAX_DIST)
        star_init(star, z_move, color);
}

static inline void star_draw(struct star * star, int z_move, bool color)
{
    int x_draw, y_draw;
    /*
     * 3d -> 2d : projection on the screen : x2d=x3d/z (thales theorem)
     * we put the star in the center of the screen
     */
    x_draw = star->x / star->z + LCD_CENTER_X;
    if (x_draw < 1 || x_draw >= LCD_WIDTH)
    {
        star_init(star, z_move, color);
        return;
    }
    y_draw  = star->y / star->z + LCD_CENTER_Y;
    if (y_draw < 1 || y_draw >= LCD_HEIGHT)
    {
        star_init(star, z_move, color);
        return;
    }

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(star->color);
#endif

    rb->lcd_drawpixel(x_draw, y_draw);
    if(star->z<5*Z_MAX_DIST/6)
    {
        rb->lcd_drawpixel(x_draw, y_draw - 1);
        rb->lcd_drawpixel(x_draw - 1, y_draw);
        if(star->z<Z_MAX_DIST/2)
        {
            rb->lcd_drawpixel(x_draw + 1, y_draw);
            rb->lcd_drawpixel(x_draw, y_draw + 1);
        }
    }
}

/*
 * Whole starfield operations
 */
struct starfield
{
    struct star tab[MAX_STARS];
    int nb_stars;
    int z_move;
    bool color;
};

static inline void starfield_init(struct starfield * starfield)
{
    starfield->nb_stars=0;
    starfield->z_move=INIT_SPACE_SPEED;
    starfield->color=false;
}

static inline void starfield_add_stars(struct starfield * starfield,
                                       int nb_to_add)
{
    int i, old_nb_stars;
    old_nb_stars=starfield->nb_stars;
    starfield->nb_stars+=nb_to_add;

    if(starfield->nb_stars > MAX_STARS)
        starfield->nb_stars=MAX_STARS;

    for( i=old_nb_stars ; i < starfield->nb_stars ; ++i )
    {
        star_init( &(starfield->tab[i]), starfield->z_move, starfield->color );
    }
}

static inline void starfield_del_stars(struct starfield * starfield,
                                       int nb_to_add)
{
    starfield->nb_stars-=nb_to_add;
    if(starfield->nb_stars<0)
        starfield->nb_stars=0;
}

static inline void starfield_move_and_draw(struct starfield * starfield)
{
    int i;
    for(i=0;i<starfield->nb_stars;++i)
    {
        star_move(&(starfield->tab[i]), starfield->z_move, starfield->color);
        star_draw(&(starfield->tab[i]), starfield->z_move, starfield->color);
    }
}

static struct starfield starfield;

static int plugin_main(void)
{
    int button, avg_peak, t_disp=0;
    int font_h, font_w;
    bool pulse __attribute__ ((unused)) = true; /* 'unused' resolves warnings */
    rb->lcd_getstringsize("A", &font_w, &font_h);
    starfield_init(&starfield);
    starfield_add_stars(&starfield, INIT_STARS);

#if LCD_DEPTH > 1
     rb->lcd_set_backdrop(NULL);
#endif
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_foreground(LCD_WHITE);
#endif

    while (true)
    {
        rb->sleep(1);
        rb->lcd_clear_display();

#if ((CONFIG_CODEC == SWCODEC)  || !defined(SIMULATOR) && \
    ((CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)))

        /* This will make the stars pulse to the music */
        if(pulse){

            /* Get the peaks. ( Borrowed from vu_meter ) */
#if (CONFIG_CODEC == SWCODEC)
            static struct pcm_peaks peaks;
            rb->mixer_channel_calculate_peaks(PCM_MIXER_CHAN_PLAYBACK,
                                              &peaks);
            #define left_peak peaks.left
            #define right_peak peaks.right
#else
            int left_peak = rb->mas_codec_readreg(0xC);
            int right_peak = rb->mas_codec_readreg(0xD);
#endif
            /* Devide peak data by 4098 to bring the max
               value down from ~32k to 8 */
            left_peak  =    left_peak/0x1000;
            right_peak =    right_peak/0x1000;

            /* Make sure they dont stop */
            if(left_peak<0x1)
                left_peak     = 0x1;
            if(right_peak<0x1)
                right_peak    = 0x1;

            /* And make sure they dont go over 8 */
            if(left_peak>0x8)
                left_peak     = 0x8;
            if(right_peak>0x8)
                right_peak    = 0x8;

            /* We need the average of both chanels */
            avg_peak     = ( left_peak + right_peak )/2;

            /* Set the speed to the peak meter */
            starfield.z_move = avg_peak;

        } /* if pulse */
#else
        (void) avg_peak;
#endif
        starfield_move_and_draw(&starfield);

#ifdef HAVE_LCD_COLOR
        rb->lcd_set_foreground(LCD_WHITE);
#endif

        /* if a parameter is updated (by the user), we must print it */
        if (t_disp > 0)
        {
            --t_disp;
#ifdef HAVE_LCD_COLOR
            rb->lcd_set_foreground(LCD_WHITE);
#endif
            rb->lcd_putsxyf(0, LCD_HEIGHT-font_h, "star:%d speed:%d",
                           starfield.nb_stars, starfield.z_move);
        }
        rb->lcd_update();

        /*We get button from PLA this way */
        button = pluginlib_getaction(TIMEOUT_NOBLOCK, plugin_contexts,
                               ARRAYLEN(plugin_contexts));

        switch(button)
        {
            case (STARFIELD_INCREASE_ZMOVE):
            case (STARFIELD_INCREASE_ZMOVE_REPEAT):
                ++(starfield.z_move);
                pulse=false;
                t_disp=MSG_DISP_TIME;
                break;
            case (STARFIELD_DECREASE_ZMOVE):
            case (STARFIELD_DECREASE_ZMOVE_REPEAT):
                --(starfield.z_move);
                pulse=false;
                t_disp=MSG_DISP_TIME;
                break;
            case(STARFIELD_INCREASE_NB_STARS):
            case(STARFIELD_INCREASE_NB_STARS_REPEAT):
                starfield_add_stars(&starfield, STARFIELD_INCREASE_STEP);
                t_disp=MSG_DISP_TIME;
                break;
            case(STARFIELD_DECREASE_NB_STARS):
            case(STARFIELD_DECREASE_NB_STARS_REPEAT):
                starfield_del_stars(&starfield, STARFIELD_INCREASE_STEP);
                t_disp=MSG_DISP_TIME;
                break;
#ifdef HAVE_LCD_COLOR
            case(STARFIELD_TOGGLE_COLOR):
                starfield.color=!starfield.color;
                break;
#endif
            case(STARFIELD_QUIT):
            case(STARFIELD_QUIT2):
                return PLUGIN_OK;
                break;
            default:
                exit_on_usb(button);
                break;
        }
    }
}

/*************************** Plugin entry point ****************************/

enum plugin_status plugin_start(const void* parameter)
{
    int ret;

    (void)parameter;
    /* Turn off backlight timeout */
    backlight_ignore_timeout();

    ret = plugin_main();

    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings();

    return ret;
}
