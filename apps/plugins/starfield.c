/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* Copyright (C) 2005 Kevin Ferrare
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

PLUGIN_HEADER

/******************************* Globals ***********************************/

static struct plugin_api* rb; /* global api struct pointer */

/* Key assignement */
#if (CONFIG_KEYPAD == IPOD_4G_PAD)
#define STARFIELD_QUIT BUTTON_MENU
#define STARFIELD_INCREASE_ZMOVE BUTTON_SCROLL_FWD
#define STARFIELD_DECREASE_ZMOVE BUTTON_SCROLL_BACK
#define STARFIELD_INCREASE_NB_STARS BUTTON_RIGHT
#define STARFIELD_DECREASE_NB_STARS BUTTON_LEFT
#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#define STARFIELD_QUIT BUTTON_POWER
#define STARFIELD_INCREASE_ZMOVE BUTTON_UP
#define STARFIELD_DECREASE_ZMOVE BUTTON_DOWN
#define STARFIELD_INCREASE_NB_STARS BUTTON_RIGHT
#define STARFIELD_DECREASE_NB_STARS BUTTON_LEFT
#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define STARFIELD_QUIT BUTTON_A
#define STARFIELD_INCREASE_ZMOVE BUTTON_UP
#define STARFIELD_DECREASE_ZMOVE BUTTON_DOWN
#define STARFIELD_INCREASE_NB_STARS BUTTON_RIGHT
#define STARFIELD_DECREASE_NB_STARS BUTTON_LEFT
#else
#define STARFIELD_QUIT BUTTON_OFF
#define STARFIELD_INCREASE_ZMOVE BUTTON_UP
#define STARFIELD_DECREASE_ZMOVE BUTTON_DOWN
#define STARFIELD_INCREASE_NB_STARS BUTTON_RIGHT
#define STARFIELD_DECREASE_NB_STARS BUTTON_LEFT
#endif

#define LCD_CENTER_X (LCD_WIDTH/2)
#define LCD_CENTER_Y (LCD_HEIGHT/2)
#define Z_MAX_DIST 100


#define MAX_STARS (LCD_WIDTH*LCD_HEIGHT*20)/100
#define INIT_STARS 600
#define STARFIELD_INCREASE_STEP 100
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

static struct plugin_api* rb; /* global api struct pointer */

/*
 * Each star's stuffs
 */
struct star
{
    int x,y,z;
    int velocity;
};

static inline void star_init(struct star * star, int z_move)
{
    star->velocity=rb->rand() % STAR_MAX_VELOCITY+1;
    /* choose x between -MAX_INIT_STAR_X and MAX_INIT_STAR_X */
    star->x=rb->rand() % (2*MAX_INIT_STAR_X)-MAX_INIT_STAR_X;
    star->y=rb->rand() % (2*MAX_INIT_STAR_Y)-MAX_INIT_STAR_Y;
    if(z_move>=0)
        star->z=Z_MAX_DIST;
    else
        star->z=rb->rand() %Z_MAX_DIST/2+1;
}

static inline void star_move(struct star * star, int z_move)
{
    star->z -= z_move*star->velocity;
    if (star->z <= 0 || star->z > Z_MAX_DIST)
        star_init(star, z_move);
}

static inline void star_draw(struct star * star, int z_move)
{
    int x_draw, y_draw;
    /*
     * 3d -> 2d : projection on the screen : x2d=x3d/z (thales theorem)
     * we put the star in the center of the screen
     */
    x_draw = star->x / star->z + LCD_CENTER_X;
    if (x_draw < 1 || x_draw >= LCD_WIDTH)
    {
        star_init(star, z_move);
        return;
    }
    y_draw  = star->y / star->z + LCD_CENTER_Y;
    if (y_draw < 1 || y_draw >= LCD_HEIGHT)
    {
        star_init(star, z_move);
        return;
    }
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
};

static inline void starfield_init(struct starfield * starfield)
{
    starfield->nb_stars=0;
    starfield->z_move=INIT_SPACE_SPEED;
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
        star_init( &(starfield->tab[i]), starfield->z_move );
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
        star_move(&(starfield->tab[i]), starfield->z_move);
        star_draw(&(starfield->tab[i]), starfield->z_move);
    }
}

static struct starfield starfield;

int plugin_main(void)
{
    char str_buffer[40];
    int button, t_disp=0;
    int font_h, font_w;
    rb->lcd_getstringsize("A", &font_w, &font_h);
    starfield_init(&starfield);
    starfield_add_stars(&starfield, INIT_STARS);

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_foreground(LCD_WHITE);
#endif

    while (true)
    {
        rb->sleep(1);
        rb->lcd_clear_display();

        starfield_move_and_draw(&starfield);

        if (t_disp > 0)
        {/* if a parameter is updated (by the user), we must print it */
            --t_disp;
            rb->snprintf(str_buffer, sizeof(str_buffer),
                         "star:%d speed:%d",
                         starfield.nb_stars,
                         starfield.z_move);
            rb->lcd_putsxy(0, LCD_HEIGHT-font_h, str_buffer);
        }
        rb->lcd_update();

        button = rb->button_get(false);
        switch(button)
        {
            case (STARFIELD_INCREASE_ZMOVE):
            case (STARFIELD_INCREASE_ZMOVE | BUTTON_REPEAT):
                ++(starfield.z_move);
                t_disp=MSG_DISP_TIME;
                break;
            case (STARFIELD_DECREASE_ZMOVE):
            case (STARFIELD_DECREASE_ZMOVE | BUTTON_REPEAT):
                --(starfield.z_move);
                t_disp=MSG_DISP_TIME;
                break;
            case(STARFIELD_INCREASE_NB_STARS):
            case(STARFIELD_INCREASE_NB_STARS | BUTTON_REPEAT):
                starfield_add_stars(&starfield, STARFIELD_INCREASE_STEP);
                t_disp=MSG_DISP_TIME;
                break;
            case(STARFIELD_DECREASE_NB_STARS):
            case(STARFIELD_DECREASE_NB_STARS | BUTTON_REPEAT):
                starfield_del_stars(&starfield, STARFIELD_INCREASE_STEP);
                t_disp=MSG_DISP_TIME;
                break;
            case(STARFIELD_QUIT):
            case(SYS_USB_CONNECTED):
                rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
                return PLUGIN_OK;
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

    ret = plugin_main();

    return ret;
}

#endif // #ifdef HAVE_LCD_BITMAP
