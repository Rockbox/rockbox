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
****************************************************************************/

#include "config.h"
#include "options.h"

#ifdef USE_DEMOS

#include <stdlib.h>
#include "lcd.h"
#include "config.h"
#include "kernel.h"
#include "menu.h"
#include "button.h"
#include "sprintf.h"
#include "screens.h"

typedef struct
{long x,y,z;} point3D;

typedef struct
{long x,y;}   point2D;

static point3D Sommet[8];
static point3D Point3D[8];
static point2D Point2D[8];

static int Nb_points = 8;

static int Xoff = 56;
static int Yoff = 95;
static int Zoff = 600;

/* Precalculated sine and cosine * 10000 (four digit fixed point math) */
static int SinTable[91] = 
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

static long Sin(int val)
{
    /* Speed improvement through sukzessive lookup */
    if (val<181)
    {
        if (val<91)/* phase 0-90 degree */
        {
            return (long)SinTable[val];
        }
        else/* phase 91-180 degree */
        {
            return (long)SinTable[180-val];
        }
    }
    else
    {
        if (val<271)/* phase 181-270 degree */
        {
            return (-1L)*(long)SinTable[val-180];
        }
        else/* phase 270-359 degree */
        {
            return (-1L)*(long)SinTable[360-val];
        }
    }
    return 0;
}

static long Cos(int val)
{
    /* Speed improvement through sukzessive lookup */
    if (val<181)
    {
        if (val<91)/* phase 0-90 degree */
        {
            return (long)SinTable[90-val];
        }
        else/* phase 91-180 degree */
        {
            return (-1L)*(long)SinTable[val-90];
        }
    }
    else
    {
        if (val<271)/* phase 181-270 degree */
        {
            return (-1L)*(long)SinTable[270-val];
        }
        else/* phase 270-359 degree */
        {
            return (long)SinTable[val-270];
        }
    }
    return 0;
}

static long matrice[3][3];

static void cube_rotate(int Xa, int Ya, int Za)
{
    int i;

    /* Just to prevent unnecessary lookups */
    long sxa,cxa,sya,cya,sza,cza;
    sxa=Sin(Xa);
    cxa=Cos(Xa);
    sya=Sin(Ya);
    cya=Cos(Ya);
    sza=Sin(Za);
    cza=Cos(Za);

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
    for(i=0;i<Nb_points;i++)
    {
        Point3D[i].x = matrice[0][0]*Sommet[i].x
		     + matrice[1][0]*Sommet[i].y
		     + matrice[2][0]*Sommet[i].z;

        Point3D[i].y = matrice[0][1]*Sommet[i].x
		     + matrice[1][1]*Sommet[i].y
		     + matrice[2][1]*Sommet[i].z;

        Point3D[i].z = matrice[0][2]*Sommet[i].x
		     + matrice[1][2]*Sommet[i].y
		     + matrice[2][2]*Sommet[i].z;
    }
}

static void cube_viewport(void)
{
    int i;

    /* Do viewport transformation for all points */
    for(i=0;i<Nb_points;i++)
    {
        Point2D[i].x=(((Point3D[i].x)<<8)/10000L)/
          (Point3D[i].z/10000L+Zoff)+Xoff;
        Point2D[i].y=(((Point3D[i].y)<<8)/10000L)/
          (Point3D[i].z/10000L+Zoff)+Yoff;
    }
}

static void cube_init(void)
{
    /* Original 3D-position of cube's corners */
    Sommet[0].x = -40;  Sommet[0].y = -40;  Sommet[0].z = -40;
    Sommet[1].x =  40;  Sommet[1].y = -40;  Sommet[1].z = -40;
    Sommet[2].x =  40;  Sommet[2].y =  40;  Sommet[2].z = -40;
    Sommet[3].x = -40;  Sommet[3].y =  40;  Sommet[3].z = -40;
    Sommet[4].x =  40;  Sommet[4].y = -40;  Sommet[4].z =  40;
    Sommet[5].x = -40;  Sommet[5].y = -40;  Sommet[5].z =  40;
    Sommet[6].x = -40;  Sommet[6].y =  40;  Sommet[6].z =  40;
    Sommet[7].x =  40;  Sommet[7].y =  40;  Sommet[7].z =  40;
}

static void line(int a, int b)
{
    lcd_drawline(Point2D[a].x,Point2D[a].y,Point2D[b].x,Point2D[b].y);
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


/* Loops that the values are displayed */
#define DISP_TIME 30

bool cube(void)
{
    int t_disp=0;
    char buffer[30];

    int xa=0;
    int ya=0;
    int za=0;
    int xs=1;
    int ys=3;
    int zs=1;
    bool highspeed=0;
    bool exit=0;
    
    cube_init();

    while(!exit)
    {
        if (!highspeed) sleep(4);
        
        lcd_clear_display();
        cube_rotate(xa,ya,za);
        cube_viewport();
        cube_draw();
        if (t_disp>0)
        {
            t_disp--;
            snprintf(buffer, 30, "x:%d y:%d z:%d h:%d",xs,ys,zs,highspeed);
            lcd_putsxy(0, 56, buffer);
         }
        lcd_update();
        
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

        switch(button_get(false)) 
        {
            case BUTTON_RIGHT:
                xs+=1;
                if (xs>10)
                    xs=10;
                t_disp=DISP_TIME;
                break;
            case BUTTON_LEFT:
                xs-=1;
                if (xs<-10)
                    xs=-10;
                t_disp=DISP_TIME;
                break;
            case BUTTON_UP:
                ys+=1;
                if (ys>10)
                    ys=10;
                t_disp=DISP_TIME;
                break;
            case BUTTON_DOWN:
                ys-=1;
                if (ys<-10)
                    ys=-10;
                t_disp=DISP_TIME;
                break;
            case BUTTON_F2:
                zs+=1;
                if (zs>10)
                    zs=10;
                t_disp=DISP_TIME;
                break;
            case BUTTON_F1:
                zs-=1;
                if (zs<-10)
                    zs=-10;
                t_disp=DISP_TIME;
                break;
            case BUTTON_PLAY:
                highspeed=!highspeed;
                t_disp=DISP_TIME;
                break;
            case BUTTON_OFF|BUTTON_REL:
                exit=1;
                break;

            case SYS_USB_CONNECTED:
                usb_screen();
                return true;

        }
    }
    return false;
}

#endif /* USE_DEMOS */

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "../../firmware/rockbox-mode.el")
 * end:
 * vim: et sw=4 ts=8 sts=4 tw=78
 */

