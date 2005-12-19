/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Itai Shaked
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 **************************************************************************/
#include "plugin.h"
#include "playergfx.h"

#ifdef HAVE_LCD_BITMAP
#define NUM_PARTICLES (LCD_WIDTH * LCD_HEIGHT / 72)
#define SNOW_HEIGHT LCD_HEIGHT
#define SNOW_WIDTH LCD_WIDTH
#define MYLCD(fn) rb->lcd_ ## fn
#else
#define NUM_PARTICLES 10
#define SNOW_HEIGHT 14
#define SNOW_WIDTH 20
#define MYLCD(fn) pgfx_ ## fn
#endif

/* variable button definitions */
#if CONFIG_KEYPAD == PLAYER_PAD
#define SNOW_QUIT BUTTON_STOP
#elif (CONFIG_KEYPAD == IPOD_4G_PAD)
#define SNOW_QUIT BUTTON_MENU
#else
#define SNOW_QUIT BUTTON_OFF
#endif

static short particles[NUM_PARTICLES][2];
static struct plugin_api* rb;

#ifdef HAVE_LCD_BITMAP
#if LCD_WIDTH >= 160
#define FLAKE_WIDTH 5
static const unsigned char flake[] = {0x0a,0x04,0x1f,0x04,0x0a};
#else
#define FLAKE_WIDTH 3
static const unsigned char flake[] = {0x02,0x07,0x02};
#endif
#endif

static bool particle_exists(int particle)
{
    if (particles[particle][0]>=0 && particles[particle][1]>=0 &&
        particles[particle][0]<SNOW_WIDTH && particles[particle][1]<SNOW_HEIGHT)
        return true;
    else
        return false;
}

static int create_particle(void)
{
    int i;

    for (i=0; i<NUM_PARTICLES; i++) {
        if (!particle_exists(i)) {
            particles[i][0]=(rb->rand()%SNOW_WIDTH);
            particles[i][1]=0;
            return i;
        }
    }
    return -1;
}

static void snow_move(void)
{
    int i;

    if (!(rb->rand()%2))
        create_particle();

    for (i=0; i<NUM_PARTICLES; i++) {
        if (particle_exists(i)) {
            MYLCD(set_drawmode)(DRMODE_SOLID|DRMODE_INVERSEVID);
#ifdef HAVE_LCD_BITMAP
            rb->lcd_fillrect(particles[i][0],particles[i][1],
                             FLAKE_WIDTH,FLAKE_WIDTH);
#else
            pgfx_drawpixel(particles[i][0],particles[i][1]);
#endif
            MYLCD(set_drawmode)(DRMODE_SOLID);
#ifdef HAVE_REMOTE_LCD
            if (particles[i][0] <= LCD_REMOTE_WIDTH 
                    && particles[i][1] <= LCD_REMOTE_HEIGHT) {
                rb->lcd_remote_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                rb->lcd_remote_fillrect(particles[i][0],particles[i][1],
                                        FLAKE_WIDTH,FLAKE_WIDTH);
                rb->lcd_remote_set_drawmode(DRMODE_SOLID);
            }
#endif
            switch ((rb->rand()%7)) {
                case 0:
                    particles[i][0]++;
                    break;

                case 1:
                    particles[i][0]--;
                    break;

                case 2:
                    break;

                default:
                    particles[i][1]++;
                    break;
            }
            if (particle_exists(i))
#ifdef HAVE_LCD_BITMAP
                rb->lcd_mono_bitmap(flake,particles[i][0],particles[i][1],
                                    FLAKE_WIDTH,FLAKE_WIDTH);
#else
                pgfx_drawpixel(particles[i][0],particles[i][1]);
#endif
#ifdef HAVE_REMOTE_LCD
            if (particles[i][0] <= LCD_REMOTE_WIDTH 
                    && particles[i][1] <= LCD_REMOTE_HEIGHT) {
                rb->lcd_remote_bitmap(flake,particles[i][0],particles[i][1],
                                      FLAKE_WIDTH,FLAKE_WIDTH);
            }
#endif
        }
    }
}


static void snow_init(void)
{
    int i;

    for (i=0; i<NUM_PARTICLES; i++) {
        particles[i][0]=-1;
        particles[i][1]=-1;
    }        
#ifdef HAVE_LCD_CHARCELLS
    pgfx_display(0, 0); /* display three times */
    pgfx_display(4, 0);
    pgfx_display(8, 0);
#endif
    MYLCD(clear_display)();
#ifdef HAVE_REMOTE_LCD
    rb->lcd_remote_clear_display();
#endif
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int button;
    TEST_PLUGIN_API(api);
    (void)(parameter);
    rb = api;

#ifdef HAVE_LCD_CHARCELLS
    if (!pgfx_init(rb, 4, 2))
    {
        rb->splash(HZ*2, true, "Old LCD :(");
        return PLUGIN_OK;
    }
#endif
    snow_init();
    while (1) {
        snow_move();
        MYLCD(update)();
#ifdef HAVE_REMOTE_LCD
        rb->lcd_remote_update();
#endif
        rb->sleep(HZ/20);
        
        button = rb->button_get(false);

        if (button == SNOW_QUIT)
        {
#ifdef HAVE_LCD_CHARCELLS
            pgfx_release();
#endif
            return PLUGIN_OK;
        }
        else
            if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
            {
#ifdef HAVE_LCD_CHARCELLS
                pgfx_release();
#endif
                return PLUGIN_USB_CONNECTED;
            }
    }
}

