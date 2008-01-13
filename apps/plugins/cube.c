/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2002 Damien Teney
* modified to use int instead of float math by Andreas Zwirtes
* heavily extended by Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
***************************************************************************/
#include "plugin.h"
#include "grey.h"
#include "playergfx.h"
#include "xlcd.h"
#include "fixedpoint.h"

PLUGIN_HEADER

/* Loops that the values are displayed */
#define DISP_TIME 30

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define CUBE_QUIT          BUTTON_OFF
#define CUBE_X_INC         BUTTON_RIGHT
#define CUBE_X_DEC         BUTTON_LEFT
#define CUBE_Y_INC         BUTTON_UP
#define CUBE_Y_DEC         BUTTON_DOWN
#define CUBE_Z_INC         BUTTON_F2
#define CUBE_Z_DEC         BUTTON_F1
#define CUBE_MODE          BUTTON_F3
#define CUBE_PAUSE         BUTTON_PLAY
#define CUBE_HIGHSPEED     BUTTON_ON

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define CUBE_QUIT          BUTTON_OFF
#define CUBE_X_INC         BUTTON_RIGHT
#define CUBE_X_DEC         BUTTON_LEFT
#define CUBE_Y_INC         BUTTON_UP
#define CUBE_Y_DEC         BUTTON_DOWN
#define CUBE_Z_INC         BUTTON_F2
#define CUBE_Z_DEC         BUTTON_F1
#define CUBE_MODE          BUTTON_F3
#define CUBE_PAUSE         BUTTON_SELECT
#define CUBE_HIGHSPEED     BUTTON_ON

#elif CONFIG_KEYPAD == PLAYER_PAD
#define CUBE_QUIT          BUTTON_STOP
#define CUBE_X_INC         BUTTON_RIGHT
#define CUBE_X_DEC         BUTTON_LEFT
#define CUBE_Y_INC         (BUTTON_ON | BUTTON_RIGHT)
#define CUBE_Y_DEC         (BUTTON_ON | BUTTON_LEFT)
#define CUBE_Z_INC         (BUTTON_MENU | BUTTON_RIGHT)
#define CUBE_Z_DEC         (BUTTON_MENU | BUTTON_LEFT)
#define CUBE_MODE_PRE      BUTTON_MENU
#define CUBE_MODE          (BUTTON_MENU | BUTTON_REL)
#define CUBE_PAUSE         BUTTON_PLAY
#define CUBE_HIGHSPEED_PRE BUTTON_ON
#define CUBE_HIGHSPEED     (BUTTON_ON | BUTTON_REL)

#elif CONFIG_KEYPAD == ONDIO_PAD
#define CUBE_QUIT          BUTTON_OFF
#define CUBE_X_INC         BUTTON_RIGHT
#define CUBE_X_DEC         BUTTON_LEFT
#define CUBE_Y_INC         BUTTON_UP
#define CUBE_Y_DEC         BUTTON_DOWN
#define CUBE_Z_INC         (BUTTON_MENU | BUTTON_UP)
#define CUBE_Z_DEC         (BUTTON_MENU | BUTTON_DOWN)
#define CUBE_MODE_PRE      BUTTON_MENU
#define CUBE_MODE          (BUTTON_MENU | BUTTON_REL)
#define CUBE_PAUSE         (BUTTON_MENU | BUTTON_LEFT)
#define CUBE_HIGHSPEED     (BUTTON_MENU | BUTTON_RIGHT)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define CUBE_QUIT          BUTTON_OFF
#define CUBE_X_INC         BUTTON_RIGHT
#define CUBE_X_DEC         BUTTON_LEFT
#define CUBE_Y_INC         BUTTON_UP
#define CUBE_Y_DEC         BUTTON_DOWN
#define CUBE_Z_INC         (BUTTON_ON | BUTTON_UP)
#define CUBE_Z_DEC         (BUTTON_ON | BUTTON_DOWN)
#define CUBE_MODE          BUTTON_MODE
#define CUBE_PAUSE_PRE     BUTTON_ON
#define CUBE_PAUSE         (BUTTON_ON | BUTTON_REL)
#define CUBE_HIGHSPEED     BUTTON_SELECT

#define CUBE_RC_QUIT       BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define CUBE_QUIT          BUTTON_MENU
#define CUBE_X_INC         BUTTON_RIGHT
#define CUBE_X_DEC         BUTTON_LEFT
#define CUBE_Y_INC         BUTTON_SCROLL_FWD
#define CUBE_Y_DEC         BUTTON_SCROLL_BACK
#define CUBE_Z_INC         (BUTTON_SELECT | BUTTON_RIGHT)
#define CUBE_Z_DEC         (BUTTON_SELECT | BUTTON_LEFT)
#define CUBE_MODE          (BUTTON_SELECT | BUTTON_MENU)
#define CUBE_PAUSE_PRE     BUTTON_PLAY
#define CUBE_PAUSE         (BUTTON_PLAY | BUTTON_REL)
#define CUBE_HIGHSPEED     (BUTTON_SELECT | BUTTON_PLAY)

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define CUBE_QUIT          BUTTON_PLAY
#define CUBE_X_INC         BUTTON_RIGHT
#define CUBE_X_DEC         BUTTON_LEFT
#define CUBE_Y_INC         BUTTON_UP
#define CUBE_Y_DEC         BUTTON_DOWN
#define CUBE_Z_INC         BUTTON_MODE
#define CUBE_Z_DEC         BUTTON_EQ
#define CUBE_MODE          (BUTTON_SELECT | BUTTON_REPEAT)
#define CUBE_PAUSE         (BUTTON_SELECT | BUTTON_REL)
#define CUBE_HIGHSPEED     (BUTTON_MODE | BUTTON_EQ) /* TODO: this is impossible */

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define CUBE_QUIT          BUTTON_POWER
#define CUBE_X_INC         BUTTON_LEFT
#define CUBE_X_DEC         BUTTON_RIGHT
#define CUBE_Y_INC         BUTTON_UP
#define CUBE_Y_DEC         BUTTON_DOWN
#define CUBE_Z_INC         (BUTTON_PLAY | BUTTON_UP)
#define CUBE_Z_DEC         (BUTTON_PLAY | BUTTON_DOWN)
#define CUBE_MODE          BUTTON_SELECT
#define CUBE_PAUSE_PRE     BUTTON_PLAY
#define CUBE_PAUSE         (BUTTON_PLAY | BUTTON_REL)
#define CUBE_HIGHSPEED     (BUTTON_REC | BUTTON_REL)

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define CUBE_QUIT          BUTTON_POWER
#define CUBE_X_INC         BUTTON_LEFT
#define CUBE_X_DEC         BUTTON_RIGHT
#define CUBE_Y_INC         BUTTON_UP
#define CUBE_Y_DEC         BUTTON_DOWN
#define CUBE_Z_INC         BUTTON_VOL_UP
#define CUBE_Z_DEC         BUTTON_VOL_DOWN
#define CUBE_MODE          BUTTON_MENU
#define CUBE_PAUSE         BUTTON_SELECT
#define CUBE_HIGHSPEED     BUTTON_A

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define CUBE_QUIT          BUTTON_POWER
#define CUBE_X_INC         BUTTON_LEFT
#define CUBE_X_DEC         BUTTON_RIGHT
#define CUBE_Y_INC         BUTTON_SCROLL_FWD
#define CUBE_Y_DEC         BUTTON_SCROLL_BACK
#define CUBE_Z_INC         BUTTON_UP
#define CUBE_Z_DEC         BUTTON_DOWN
#define CUBE_MODE_PRE      BUTTON_SELECT
#define CUBE_MODE          (BUTTON_SELECT|BUTTON_REPEAT)
#define CUBE_PAUSE_PRE     BUTTON_SELECT
#define CUBE_PAUSE         (BUTTON_SELECT|BUTTON_REL)
#define CUBE_HIGHSPEED     BUTTON_REC

#elif (CONFIG_KEYPAD == SANSA_C200_PAD)
#define CUBE_QUIT          BUTTON_POWER
#define CUBE_X_INC         BUTTON_LEFT
#define CUBE_X_DEC         BUTTON_RIGHT
#define CUBE_Y_INC         BUTTON_VOL_UP
#define CUBE_Y_DEC         BUTTON_VOL_DOWN
#define CUBE_Z_INC         BUTTON_UP
#define CUBE_Z_DEC         BUTTON_DOWN
#define CUBE_MODE_PRE      BUTTON_SELECT
#define CUBE_MODE          (BUTTON_SELECT|BUTTON_REPEAT)
#define CUBE_PAUSE_PRE     BUTTON_SELECT
#define CUBE_PAUSE         (BUTTON_SELECT|BUTTON_REL)
#define CUBE_HIGHSPEED     BUTTON_REC


#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define CUBE_QUIT          BUTTON_POWER
#define CUBE_X_INC         BUTTON_LEFT
#define CUBE_X_DEC         BUTTON_RIGHT
#define CUBE_Y_INC         BUTTON_SCROLL_UP
#define CUBE_Y_DEC         BUTTON_SCROLL_DOWN
#define CUBE_Z_INC         (BUTTON_PLAY | BUTTON_SCROLL_UP)
#define CUBE_Z_DEC         (BUTTON_PLAY | BUTTON_SCROLL_DOWN)
#define CUBE_MODE          BUTTON_REW
#define CUBE_PAUSE_PRE     BUTTON_PLAY
#define CUBE_PAUSE         (BUTTON_PLAY | BUTTON_REL)
#define CUBE_HIGHSPEED     (BUTTON_FF | BUTTON_REL)

#elif CONFIG_KEYPAD == MROBE500_PAD
#define CUBE_QUIT          BUTTON_POWER
#define CUBE_X_INC         BUTTON_LEFT
#define CUBE_X_DEC         BUTTON_RIGHT
#define CUBE_Y_INC         BUTTON_RC_PLAY
#define CUBE_Y_DEC         BUTTON_RC_DOWN
#define CUBE_Z_INC         BUTTON_RC_VOL_UP
#define CUBE_Z_DEC         BUTTON_RC_VOL_DOWN
#define CUBE_MODE          BUTTON_RC_MODE
#define CUBE_PAUSE_PRE     BUTTON_RC_HEART
#define CUBE_PAUSE         (BUTTON_RC_HEART | BUTTON_REL)
#define CUBE_HIGHSPEED     BUTTON_RC_HEART

#endif

#ifdef HAVE_LCD_BITMAP

#define DIST (10 * MIN(LCD_HEIGHT, LCD_WIDTH) / 16)
static int x_off = LCD_WIDTH/2;
static int y_off = LCD_HEIGHT/2;

#if LCD_DEPTH == 1
#define USE_GSLIB
GREY_INFO_STRUCT
struct my_lcd {
    void (*update)(void);
    void (*clear_display)(void);
    void (*drawline)(int x1, int y1, int x2, int y2);
    void (*putsxy)(int x, int y, const unsigned char *string);
};

static struct my_lcd greyfuncs = {
    grey_update, grey_clear_display, grey_drawline, grey_putsxy
};
static struct my_lcd lcdfuncs; /* initialised at runtime */
static struct my_lcd *mylcd = &greyfuncs;

#define MYLCD(fn) mylcd->fn
#define MY_FILLTRIANGLE(x1, y1, x2, y2, x3, y3) grey_filltriangle(x1, y1, x2, y2, x3, y3)
#define MY_SET_FOREGROUND(fg) grey_set_foreground(fg)
#define MY_GET_FOREGROUND() grey_get_foreground()

#else
#define MYLCD(fn) rb->lcd_ ## fn
#define MY_FILLTRIANGLE(x1, y1, x2, y2, x3, y3) xlcd_filltriangle(x1, y1, x2, y2, x3, y3)
#define MY_SET_FOREGROUND(fg) rb->lcd_set_foreground(fg)
#define MY_GET_FOREGROUND() rb->lcd_get_foreground()
#endif

#if CONFIG_LCD == LCD_SSD1815
#define ASPECT 320 /* = 1.25 (fixed point 24.8) */
#else
#define ASPECT 256 /* = 1.00 */
#endif

#else /* !LCD_BITMAP */

#define MYLCD(fn) pgfx_ ## fn
#define DIST 9
static int x_off = 10;
static int y_off = 7;
#define ASPECT 300 /* = 1.175 */

#endif /* !LCD_BITMAP */

struct point_3D {
    long x, y, z;
};

struct point_2D {
    long x, y;
};

struct line {
    int start, end;
};

struct face {
    int corner[4];
    int line[4];
};

/* initial, unrotated cube corners */
static const struct point_3D sommet[8] =
{
    {-DIST, -DIST, -DIST},
    { DIST, -DIST, -DIST},
    { DIST,  DIST, -DIST},
    {-DIST,  DIST, -DIST},
    {-DIST, -DIST,  DIST},
    { DIST, -DIST,  DIST},
    { DIST,  DIST,  DIST},
    {-DIST,  DIST,  DIST}
};

/* The 12 lines forming the edges */
static const struct line lines[12] =
{
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 7}, {7, 6}, {6, 5}, {5, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
};

static bool lines_drawn[12];

/* The 6 faces of the cube; points are in clockwise order when viewed
   from the outside */
static const struct face faces[6] =
{
    {{0, 1, 2, 3}, {0, 1, 2, 3}},
    {{4, 7, 6, 5}, {4, 5, 6, 7}},
    {{0, 4, 5, 1}, {8, 7, 9, 0}},
    {{2, 6, 7, 3}, {10, 5, 11, 2}},
    {{0, 3, 7, 4}, {3, 11, 4, 8}},
    {{1, 5, 6, 2}, {9, 6, 10, 1}}
};

#if LCD_DEPTH > 1 || defined(USE_GSLIB)
static const unsigned face_colors[6] =
{
#ifdef HAVE_LCD_COLOR
    LCD_RGBPACK(255, 0, 0), LCD_RGBPACK(255, 0, 0), LCD_RGBPACK(0, 255, 0),
    LCD_RGBPACK(0, 255, 0), LCD_RGBPACK(0, 0, 255), LCD_RGBPACK(0, 0, 255)
#elif defined(USE_GSLIB)
    GREY_LIGHTGRAY, GREY_LIGHTGRAY, GREY_DARKGRAY,
    GREY_DARKGRAY,  GREY_BLACK,     GREY_BLACK
#else
    LCD_LIGHTGRAY, LCD_LIGHTGRAY, LCD_DARKGRAY,
    LCD_DARKGRAY,  LCD_BLACK,     LCD_BLACK
#endif
};
#endif

enum {
#if LCD_DEPTH > 1 || defined(USE_GSLIB)
    SOLID,
#endif
    HIDDEN_LINES,
    WIREFRAME,
    NUM_MODES
};

static int mode = 0;

static struct point_3D point3D[8];
static struct point_2D point2D[8];
static long matrice[3][3];

static const int nb_points = 8;
static long z_off = 600;

static struct plugin_api* rb;

static void cube_rotate(int xa, int ya, int za)
{
    int i;
    /* Just to prevent unnecessary lookups */
    long sxa, cxa, sya, cya, sza, cza;

    sxa = sin_int(xa);
    cxa = cos_int(xa);
    sya = sin_int(ya);
    cya = cos_int(ya);
    sza = sin_int(za);
    cza = cos_int(za);

    /* calculate overall translation matrix */
    matrice[0][0] = (cza * cya) >> 14;
    matrice[1][0] = (sza * cya) >> 14;
    matrice[2][0] = -sya;

    matrice[0][1] = (((cza * sya) >> 14) * sxa - sza * cxa) >> 14;
    matrice[1][1] = (((sza * sya) >> 14) * sxa + cxa * cza) >> 14;
    matrice[2][1] = (sxa * cya) >> 14;

    matrice[0][2] = (((cza * sya) >> 14) * cxa + sza * sxa) >> 14;
    matrice[1][2] = (((sza * sya) >> 14) * cxa - cza * sxa) >> 14;
    matrice[2][2] = (cxa * cya) >> 14;

    /* apply translation matrix to all points */
    for (i = 0; i < nb_points; i++)
    {
        point3D[i].x = matrice[0][0] * sommet[i].x + matrice[1][0] * sommet[i].y
                     + matrice[2][0] * sommet[i].z;

        point3D[i].y = matrice[0][1] * sommet[i].x + matrice[1][1] * sommet[i].y
                     + matrice[2][1] * sommet[i].z;

        point3D[i].z = matrice[0][2] * sommet[i].x + matrice[1][2] * sommet[i].y
                     + matrice[2][2] * sommet[i].z;
    }
}

static void cube_viewport(void)
{
    int i;

    /* Do viewport transformation for all points */
    for (i = 0; i < nb_points; i++)
    {
#if ASPECT != 256
        point2D[i].x = (point3D[i].x * ASPECT) / (point3D[i].z + (z_off << 14))
                     + x_off;
#else
        point2D[i].x = (point3D[i].x << 8) / (point3D[i].z + (z_off << 14))
                     + x_off;
#endif
        point2D[i].y = (point3D[i].y << 8) / (point3D[i].z + (z_off << 14))
                     + y_off;
    }
}

static void cube_draw(void)
{
    int i, j, line;
#if LCD_DEPTH > 1 || defined(USE_GSLIB)
    unsigned old_foreground;
#endif

    switch (mode)
    {
#if LCD_DEPTH > 1 || defined(USE_GSLIB)
      case SOLID:
      
        old_foreground = MY_GET_FOREGROUND();
        for (i = 0; i < 6; i++)
        {
            /* backface culling; if the shape winds counter-clockwise, we are
             * looking at the backface, and the (simplified) cross product
             * is < 0. Do not draw it. */
            if (0 >= (point2D[faces[i].corner[1]].x - point2D[faces[i].corner[0]].x)
                   * (point2D[faces[i].corner[2]].y - point2D[faces[i].corner[1]].y)
                   - (point2D[faces[i].corner[1]].y - point2D[faces[i].corner[0]].y)
                   * (point2D[faces[i].corner[2]].x - point2D[faces[i].corner[1]].x))
                continue;

            MY_SET_FOREGROUND(face_colors[i]);
            MY_FILLTRIANGLE(point2D[faces[i].corner[0]].x,
                            point2D[faces[i].corner[0]].y,
                            point2D[faces[i].corner[1]].x,
                            point2D[faces[i].corner[1]].y,
                            point2D[faces[i].corner[2]].x,
                            point2D[faces[i].corner[2]].y);
            MY_FILLTRIANGLE(point2D[faces[i].corner[0]].x,
                            point2D[faces[i].corner[0]].y,
                            point2D[faces[i].corner[2]].x,
                            point2D[faces[i].corner[2]].y,
                            point2D[faces[i].corner[3]].x,
                            point2D[faces[i].corner[3]].y);

        }
        MY_SET_FOREGROUND(old_foreground);
        break;
#endif /* (LCD_DEPTH > 1) || GSLIB */

      case HIDDEN_LINES:

        rb->memset(lines_drawn, 0, sizeof(lines_drawn));
        for (i = 0; i < 6; i++)
        {
            /* backface culling; if the shape winds counter-clockwise, we are
             * looking at the backface, and the (simplified) cross product
             * is < 0. Do not draw it. */
            if (0 >= (point2D[faces[i].corner[1]].x - point2D[faces[i].corner[0]].x)
                   * (point2D[faces[i].corner[2]].y - point2D[faces[i].corner[1]].y)
                   - (point2D[faces[i].corner[1]].y - point2D[faces[i].corner[0]].y)
                   * (point2D[faces[i].corner[2]].x - point2D[faces[i].corner[1]].x))
                continue;

            for (j = 0; j < 4; j++)
            {
                line = faces[i].line[j];
                if (!lines_drawn[line])
                {
                    lines_drawn[line] = true;
                    MYLCD(drawline)(point2D[lines[line].start].x,
                                    point2D[lines[line].start].y,
                                    point2D[lines[line].end].x,
                                    point2D[lines[line].end].y);
                }
            }
        }
        break;

      case WIREFRAME:

        for (i = 0; i < 12; i++)
            MYLCD(drawline)(point2D[lines[i].start].x,
                            point2D[lines[i].start].y,
                            point2D[lines[i].end].x,
                            point2D[lines[i].end].y);
        break;
    }
}

void cleanup(void *parameter)
{
    (void)parameter;

#ifdef USE_GSLIB
    grey_release();
#elif defined HAVE_LCD_CHARCELLS
    pgfx_release();
#endif
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    char buffer[30];
    int t_disp = 0;
#ifdef USE_GSLIB
    unsigned char *gbuf;
    size_t gbuf_size = 0;
    bool mode_switch = true;
#endif

    int button;
    int lastbutton = BUTTON_NONE;
    int xa = 0;
    int ya = 0;
    int za = 0;
    int xs = 1;
    int ys = 3;
    int zs = 1;
    bool highspeed = false;
    bool paused = false;
    bool redraw = true;
    bool exit = false;

    (void)(parameter);
    rb = api;

#ifdef HAVE_LCD_BITMAP
#if LCD_DEPTH > 1
    xlcd_init(rb);
#elif defined(USE_GSLIB)
    gbuf = (unsigned char *)rb->plugin_get_buffer(&gbuf_size);
    if (!grey_init(rb, gbuf, gbuf_size, true, LCD_WIDTH, LCD_HEIGHT, NULL))
    {
        rb->splash(HZ, "Couldn't init greyscale display");
        return PLUGIN_ERROR;
    }
    /* init lcd_ function pointers */
    lcdfuncs.update =        rb->lcd_update;
    lcdfuncs.clear_display = rb->lcd_clear_display;
    lcdfuncs.drawline =      rb->lcd_drawline;
    lcdfuncs.putsxy =        rb->lcd_putsxy;

    grey_setfont(FONT_SYSFIXED);
#endif
    rb->lcd_setfont(FONT_SYSFIXED);
#else /* LCD_CHARCELLS */
    if (!pgfx_init(rb, 4, 2))
    {
        rb->splash(HZ*2, "Old LCD :(");
        return PLUGIN_OK;
    }
    pgfx_display(3, 0);
#endif

    while(!exit)
    {
        if (highspeed)
            rb->yield();
        else
            rb->sleep(4);

        if (redraw)
        {
            MYLCD(clear_display)();
            cube_rotate(xa, ya, za);
            cube_viewport();
            cube_draw();
            redraw = false;
        }

#ifdef HAVE_LCD_BITMAP
        if (t_disp > 0)
        {
            t_disp--;
            rb->snprintf(buffer, sizeof(buffer), "x:%d y:%d z:%d h:%d",
                         xs, ys, zs, highspeed);
            MYLCD(putsxy)(0, LCD_HEIGHT-8, buffer);
            if (t_disp == 0)
                redraw = true;
        }
#else
        if (t_disp > 0)
        {
            if (t_disp == DISP_TIME)
            {
                rb->snprintf(buffer, sizeof(buffer), "x%d", xs);
                rb->lcd_puts(0, 0, buffer);
                rb->snprintf(buffer, sizeof(buffer), "y%d", ys);
                rb->lcd_puts(0, 1, buffer);
                pgfx_display(3, 0);
                rb->snprintf(buffer, sizeof(buffer), "z%d", zs);
                rb->lcd_puts(8, 0, buffer);
                rb->snprintf(buffer, sizeof(buffer), "h%d", highspeed);
                rb->lcd_puts(8, 1, buffer);
            }
            t_disp--;
            if (t_disp == 0)
            {
                rb->lcd_clear_display();
                pgfx_display(3, 0);
            }
        }
#endif
#ifdef USE_GSLIB
        if (mode_switch)
        {
            grey_show(mode == SOLID);
            mode_switch = false;
        }
#endif
        MYLCD(update)();

        if (!paused)
        {
            xa += xs;
            if (xa > 359)
                xa -= 360;
            else if (xa < 0)
                xa += 360;

            ya += ys;
            if (ya > 359)
                ya -= 360;
            else if (ya < 0)
                ya += 360;

            za += zs;
            if (za > 359)
                za -= 360;
            else if (za < 0)
                za += 360;
            redraw = true;
        }

        button = rb->button_get(false);
        switch (button)
        {
            case CUBE_X_INC:
            case (CUBE_X_INC|BUTTON_REPEAT):
                if( !paused )
                {
                    if( xs < 10)
                    xs++;
                }
                else
                {
                    xa++;
                    if( xa > 359 )
                        xa -= 360;
                }
                t_disp = DISP_TIME;
                redraw = true;
                break;

            case CUBE_X_DEC:
            case (CUBE_X_DEC|BUTTON_REPEAT):
                if( !paused )
                {
                if (xs > -10)
                    xs--;
                }
                else
                {
                    xa--;
                    if( xa < 0 )
                        xa += 360;
                }
                t_disp = DISP_TIME;
                redraw = true;
                break;

            case CUBE_Y_INC:
            case (CUBE_Y_INC|BUTTON_REPEAT):
                if( !paused )
                {
                if (ys < 10)
                    ys++;
                }
                else
                {
                    ya++;
                    if( ya > 359 )
                        ya -= 360;
                }
                t_disp = DISP_TIME;
                redraw = true;
                break;

            case CUBE_Y_DEC:
            case (CUBE_Y_DEC|BUTTON_REPEAT):
                if( !paused )
                {
                if (ys > -10)
                    ys--;
                }
                else
                {
                    ya--;
                    if( ya < 0 )
                        ya += 360;
                }
                t_disp = DISP_TIME;
                redraw = true;
                break;

            case CUBE_Z_INC:
            case (CUBE_Z_INC|BUTTON_REPEAT):
                if( !paused )
                {
                if (zs < 10)
                    zs++;
                }
                else
                {
                    za++;
                    if( za > 359 )
                        za -= 360;
                }
                t_disp = DISP_TIME;
                redraw = true;
                break;

            case CUBE_Z_DEC:
            case (CUBE_Z_DEC|BUTTON_REPEAT):
                if( !paused )
                {
                if (zs > -10)
                    zs--;
                }
                else
                {
                    za--;
                    if( za < 0 )
                        za += 360;
                }
                t_disp = DISP_TIME;
                redraw = true;
                break;

            case CUBE_MODE:
#ifdef CUBE_MODE_PRE
                if (lastbutton != CUBE_MODE_PRE)
                    break;
#endif
                if (++mode >= NUM_MODES)
                    mode = 0;
#ifdef USE_GSLIB
                mylcd = (mode == SOLID) ? &greyfuncs : &lcdfuncs;
                mode_switch = true;
#endif
                redraw = true;
                break;

            case CUBE_PAUSE:
#ifdef CUBE_PAUSE_PRE
                if (lastbutton != CUBE_PAUSE_PRE)
                    break;
#endif
                paused = !paused;
                break;

            case CUBE_HIGHSPEED:
#ifdef CUBE_HIGHSPEED_PRE
                if (lastbutton != CUBE_HIGHSPEED_PRE)
                    break;
#endif
                highspeed = !highspeed;
                t_disp = DISP_TIME;
                break;

#ifdef CUBE_RC_QUIT
            case CUBE_RC_QUIT:
#endif
            case CUBE_QUIT:
                exit = true;
                break;

            default:
                if (rb->default_event_handler_ex(button, cleanup, NULL)
                    == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
        if (button != BUTTON_NONE)
            lastbutton = button;
    }

#ifdef USE_GSLIB
    grey_release();
#elif defined(HAVE_LCD_CHARCELLS)
    pgfx_release();
#endif
    return PLUGIN_OK;
}

