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
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
***************************************************************************/
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP

/* Loops that the values are displayed */
#define DISP_TIME 30

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define CUBE_QUIT (BUTTON_OFF | BUTTON_REL)
#define CUBE_X_INC BUTTON_RIGHT
#define CUBE_X_DEC BUTTON_LEFT
#define CUBE_Y_INC BUTTON_UP
#define CUBE_Y_DEC BUTTON_DOWN
#define CUBE_Z_INC BUTTON_F2
#define CUBE_Z_DEC BUTTON_F1
#define CUBE_HIGHSPEED BUTTON_PLAY

#elif CONFIG_KEYPAD == ONDIO_PAD
#define CUBE_QUIT (BUTTON_OFF | BUTTON_REL)
#define CUBE_X_INC BUTTON_RIGHT
#define CUBE_X_DEC BUTTON_LEFT
#define CUBE_Y_INC BUTTON_UP
#define CUBE_Y_DEC BUTTON_DOWN
#define CUBE_Z_INC (BUTTON_MENU | BUTTON_UP)
#define CUBE_Z_DEC (BUTTON_MENU | BUTTON_DOWN)
#define CUBE_HIGHSPEED_PRE BUTTON_MENU
#define CUBE_HIGHSPEED (BUTTON_MENU | BUTTON_REL)

#elif CONFIG_KEYPAD == IRIVER_H100_PAD
#define CUBE_QUIT (BUTTON_OFF | BUTTON_REL)
#define CUBE_X_INC BUTTON_RIGHT
#define CUBE_X_DEC BUTTON_LEFT
#define CUBE_Y_INC BUTTON_UP
#define CUBE_Y_DEC BUTTON_DOWN
#define CUBE_Z_INC (BUTTON_ON | BUTTON_UP)
#define CUBE_Z_DEC (BUTTON_ON | BUTTON_DOWN)
#define CUBE_HIGHSPEED_PRE BUTTON_SELECT
#define CUBE_HIGHSPEED (BUTTON_SELECT | BUTTON_REL)

#endif

struct point_3D {
    long x, y, z;
};

struct point_2D {
    long x, y;
};

static struct point_3D sommet[8];
static struct point_3D point3D[8];
static struct point_2D point2D[8];

static long matrice[3][3];

static int nb_points = 8;

static int x_off = LCD_WIDTH/2;
static int y_off = LCD_HEIGHT/2;
static int z_off = 600;

/* Precalculated sine and cosine * 10000 (four digit fixed point math) */
static int sin_table[91] = 
{
    0, 174, 348, 523, 697,
    871,1045,1218,1391,1564,
    1736,1908,2079,2249,2419,
    2588,2756,2923,3090,3255,
    3420,3583,3746,3907,4067,
    4226,4383,4539,4694,4848,
    5000,5150,5299,5446,5591,
    5735,5877,6018,6156,6293,
    6427,6560,6691,6819,6946,
    7071,7193,7313,7431,7547,
    7660,7771,7880,7986,8090,
    8191,8290,8386,8480,8571,
    8660,8746,8829,8910,8987,
    9063,9135,9205,9271,9335,
    9396,9455,9510,9563,9612,
    9659,9702,9743,9781,9816,
    9848,9876,9902,9925,9945,
    9961,9975,9986,9993,9998,
    10000
};

static struct plugin_api* rb;

static long sin(int val)
{
    /* Speed improvement through sukzessive lookup */
    if (val<181)
    {
        if (val<91)
        {
            /* phase 0-90 degree */
            return (long)sin_table[val];
        }
        else
        {
            /* phase 91-180 degree */
            return (long)sin_table[180-val];
        }
    }
    else
    {
        if (val<271)
        {
            /* phase 181-270 degree */
            return (-1L)*(long)sin_table[val-180];
        }
        else
        {
            /* phase 270-359 degree */
            return (-1L)*(long)sin_table[360-val];
        }
    }
    return 0;
}

static long cos(int val)
{
    /* Speed improvement through sukzessive lookup */
    if (val<181)
    {
        if (val<91)
        {
            /* phase 0-90 degree */
            return (long)sin_table[90-val];
        }
        else
        {
            /* phase 91-180 degree */
            return (-1L)*(long)sin_table[val-90];
        }
    }
    else
    {
        if (val<271)
        {
            /* phase 181-270 degree */
            return (-1L)*(long)sin_table[270-val];
        }
        else
        {
            /* phase 270-359 degree */
            return (long)sin_table[val-270];
        }
    }
    return 0;
}


static void cube_rotate(int xa, int ya, int za)
{
    int i;

    /* Just to prevent unnecessary lookups */
    long sxa,cxa,sya,cya,sza,cza;
    sxa=sin(xa);
    cxa=cos(xa);
    sya=sin(ya);
    cya=cos(ya);
    sza=sin(za);
    cza=cos(za);

    /* calculate overall translation matrix */
    matrice[0][0] = cza*cya/10000L;
    matrice[1][0] = sza*cya/10000L;
    matrice[2][0] = -sya;

    matrice[0][1] = cza*sya/10000L*sxa/10000L - sza*cxa/10000L;
    matrice[1][1] = sza*sya/10000L*sxa/10000L + cxa*cza/10000L;
    matrice[2][1] = sxa*cya/10000L;

    matrice[0][2] = cza*sya/10000L*cxa/10000L + sza*sxa/10000L;
    matrice[1][2] = sza*sya/10000L*cxa/10000L - cza*sxa/10000L;
    matrice[2][2] = cxa*cya/10000L;

    /* apply translation matrix to all points */
    for(i=0;i<nb_points;i++)
    {
        point3D[i].x = matrice[0][0]*sommet[i].x + matrice[1][0]*sommet[i].y
            + matrice[2][0]*sommet[i].z;
        
        point3D[i].y = matrice[0][1]*sommet[i].x + matrice[1][1]*sommet[i].y
            + matrice[2][1]*sommet[i].z;
        
        point3D[i].z = matrice[0][2]*sommet[i].x + matrice[1][2]*sommet[i].y
            + matrice[2][2]*sommet[i].z;
    }
}

static void cube_viewport(void)
{
    int i;

    /* Do viewport transformation for all points */
    for(i=0;i<nb_points;i++)
    {
        point2D[i].x=(((point3D[i].x)<<8)/10000L)/
          (point3D[i].z/10000L+z_off)+x_off;
        point2D[i].y=(((point3D[i].y)<<8)/10000L)/
          (point3D[i].z/10000L+z_off)+y_off;
    }
}

#if LCD_HEIGHT > 100
#define DIST 80
#else
#define DIST 40
#endif

static void cube_init(void)
{
    /* Original 3D-position of cube's corners */
    sommet[0].x = -DIST;  sommet[0].y = -DIST;  sommet[0].z = -DIST;
    sommet[1].x =  DIST;  sommet[1].y = -DIST;  sommet[1].z = -DIST;
    sommet[2].x =  DIST;  sommet[2].y =  DIST;  sommet[2].z = -DIST;
    sommet[3].x = -DIST;  sommet[3].y =  DIST;  sommet[3].z = -DIST;
    sommet[4].x =  DIST;  sommet[4].y = -DIST;  sommet[4].z =  DIST;
    sommet[5].x = -DIST;  sommet[5].y = -DIST;  sommet[5].z =  DIST;
    sommet[6].x = -DIST;  sommet[6].y =  DIST;  sommet[6].z =  DIST;
    sommet[7].x =  DIST;  sommet[7].y =  DIST;  sommet[7].z =  DIST;
}

static void line(int a, int b)
{
    rb->lcd_drawline(point2D[a].x, point2D[a].y, point2D[b].x, point2D[b].y);
}

static void cube_draw(void)
{
    /* Draws front face */
    line(0,1); line(1,2);
    line(2,3); line(3,0);

    /* Draws rear face */
    line(4,5); line(5,6);
    line(6,7); line(7,4);

    /* Draws  the other edges */
    line(0,5);
    line(1,4);
    line(2,7);
    line(3,6);
}


enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int t_disp=0;
    char buffer[30];

    int button;
    int lastbutton=0;
    int xa=0;
    int ya=0;
    int za=0;
    int xs=1;
    int ys=3;
    int zs=1;
    bool highspeed=0;
    bool exit=0;

    TEST_PLUGIN_API(api);
    (void)(parameter);
    rb = api;

    rb->lcd_setfont(FONT_SYSFIXED);

    cube_init();

    while(!exit)
    {
        if (highspeed) 
            rb->yield();
        else
            rb->sleep(4);
        
        rb->lcd_clear_display();
        cube_rotate(xa,ya,za);
        cube_viewport();
        cube_draw();
        if (t_disp>0)
        {
            t_disp--;
            rb->snprintf(buffer, 30, "x:%d y:%d z:%d h:%d",xs,ys,zs,highspeed);
            rb->lcd_putsxy(0, LCD_HEIGHT-8, buffer);
        }
        rb->lcd_update();

        xa+=xs;
        if (xa>359)
            xa-=360;
        if (xa<0)
            xa+=360;
        ya+=ys;
        if (ya>359)
            ya-=360;
        if (ya<0)
            ya+=360;
        za+=zs;
        if (za>359)
            za-=360;
        if (za<0)
            za+=360;

        button = rb->button_get(false);
        switch(button)
        {
            case CUBE_X_INC:
                xs+=1;
                if (xs>10)
                    xs=10;
                t_disp=DISP_TIME;
                break;
            case CUBE_X_DEC:
                xs-=1;
                if (xs<-10)
                    xs=-10;
                t_disp=DISP_TIME;
                break;
            case CUBE_Y_INC:
                ys+=1;
                if (ys>10)
                    ys=10;
                t_disp=DISP_TIME;
                break;
            case CUBE_Y_DEC:
                ys-=1;
                if (ys<-10)
                    ys=-10;
                t_disp=DISP_TIME;
                break;
            case CUBE_Z_INC:
                zs+=1;
                if (zs>10)
                    zs=10;
                t_disp=DISP_TIME;
                break;
            case CUBE_Z_DEC:
                zs-=1;
                if (zs<-10)
                    zs=-10;
                t_disp=DISP_TIME;
                break;
            case CUBE_HIGHSPEED:
#ifdef CUBE_HIGHSPEED_PRE
                if (lastbutton!=CUBE_HIGHSPEED_PRE)
                    break;
#endif
                highspeed=!highspeed;
                t_disp=DISP_TIME;
                break;
            case CUBE_QUIT:
                exit=1;
                break;

            default:
                if(rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
        if (button!=BUTTON_NONE)
            lastbutton=button;
    }

    return PLUGIN_OK;
}

#endif
