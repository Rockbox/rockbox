/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:
 *
 * Copyright (C) 2007 Zakk Roberts
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "oldmenuapi.h"
#include "helper.h"

PLUGIN_HEADER

static struct plugin_api* rb;

/***
 * FIREWORKS.C by ZAKK ROBERTS
 * Rockbox plugin simulating a fireworks display.
 * Supports all bitmap LCDs, fully scalable.
 * Currently disabled for Archos Recorder - runs too slow.
 ***/

/* All sorts of keymappings.. */
#if (CONFIG_KEYPAD == IRIVER_H300_PAD) || (CONFIG_KEYPAD == IRIVER_H100_PAD)
#define BTN_MENU BUTTON_OFF
#define BTN_FIRE BUTTON_SELECT
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define BTN_MENU BUTTON_MENU
#define BTN_FIRE BUTTON_SELECT
#elif (CONFIG_KEYPAD == RECORDER_PAD)
#define BTN_MENU BUTTON_OFF
#define BTN_FIRE BUTTON_PLAY
#elif (CONFIG_KEYPAD == ARCHOS_AV300_PAD)
#define BTN_MENU BUTTON_OFF
#define BTN_FIRE BUTTON_SELECT
#elif (CONFIG_KEYPAD == ONDIO_PAD)
#define BTN_MENU BUTTON_MENU
#define BTN_FIRE BUTTON_UP
#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define BTN_MENU BUTTON_POWER
#define BTN_FIRE BUTTON_SELECT
#elif (CONFIG_KEYPAD == IRIVER_IFP7XX_PAD)
#define BTN_MENU BUTTON_MODE
#define BTN_FIRE BUTTON_SELECT
#elif (CONFIG_KEYPAD == GIGABEAT_PAD) || \
(CONFIG_KEYPAD == GIGABEAT_S_PAD) || \
(CONFIG_KEYPAD == MROBE100_PAD)
#define BTN_MENU BUTTON_MENU
#define BTN_FIRE BUTTON_SELECT
#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
(CONFIG_KEYPAD == SANSA_C200_PAD)
#define BTN_MENU BUTTON_POWER
#define BTN_FIRE BUTTON_SELECT
#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define BTN_MENU BUTTON_POWER
#define BTN_FIRE BUTTON_PLAY
#endif

/* The lowdown on source terminology:
 * a ROCKET is launched from the LCD bottom.
 * FIREWORKs are ejected from the rocket when it explodes. */

#define MAX_ROCKETS 40
#define ROCKET_LIFE (LCD_HEIGHT/2)
#define ROCKET_LIFE_VAR (LCD_HEIGHT/4)
#define ROCKET_SIZE 2
#define ROCKET_MOVEMENT_RANGE 4
#define ROCKET_TRAIL_PARTICLES 50

#define MAX_FIREWORKS 40
#define FIREWORK_MOVEMENT_RANGE 6
#define FIREWORK_SIZE  2

/* position, speed, "phase" (age), color of all fireworks */
int firework_xpoints[MAX_ROCKETS+1][MAX_FIREWORKS];
int firework_ypoints[MAX_ROCKETS+1][MAX_FIREWORKS];
int firework_xspeed[MAX_ROCKETS+1][MAX_FIREWORKS];
int firework_yspeed[MAX_ROCKETS+1][MAX_FIREWORKS];
int firework_phase[MAX_ROCKETS+1];
#ifdef HAVE_LCD_COLOR
int firework_color[MAX_ROCKETS+1][MAX_FIREWORKS];
#endif

/* position, speed, "phase" (age) of all rockets */
int rocket_xpos[MAX_ROCKETS+1];
int rocket_ypos[MAX_ROCKETS+1];
int rocket_xspeed[MAX_ROCKETS+1];
int rocket_yspeed[MAX_ROCKETS+1];
int rocket_phase[MAX_ROCKETS+1];
int rocket_targetphase[MAX_ROCKETS+1];

/* settings values. these should eventually be saved to
 * disk. maybe a preset loading/saving system? */
int autofire_delay = 0;
int particles_per_firework = 2;
int particle_life = 1;
int gravity = 1;
int show_rockets = 1;
int frames_per_second = 4;
bool quit_plugin = false;

/* firework colors:
 * firework_colors = brightest firework color, used most of the time.
 * DARK colors = fireworks are nearly burnt out.
 * DARKER colors = fireworks are several frames away from burning out.
 * DARKEST colors = fireworks are a couple frames from burning out. */
#ifdef HAVE_LCD_COLOR
static const unsigned firework_colors[] = {
LCD_RGBPACK(0,255,64), LCD_RGBPACK(61,255,249), LCD_RGBPACK(255,200,61),
LCD_RGBPACK(217,22,217), LCD_RGBPACK(22,217,132), LCD_RGBPACK(67,95,254),
LCD_RGBPACK(151,84,213) };

static const unsigned firework_dark_colors[] = {
LCD_RGBPACK(0,128,32), LCD_RGBPACK(30,128,128), LCD_RGBPACK(128,100,30),
LCD_RGBPACK(109,11,109), LCD_RGBPACK(11,109,66), LCD_RGBPACK(33,47,128),
LCD_RGBPACK(75,42,105) };

static const unsigned firework_darker_colors[] = {
LCD_RGBPACK(0,64,16), LCD_RGBPACK(15,64,64), LCD_RGBPACK(64,50,15),
LCD_RGBPACK(55,5,55), LCD_RGBPACK(5,55,33), LCD_RGBPACK(16,24,64),
LCD_RGBPACK(38,21,52) };

static const unsigned firework_darkest_colors[] = {
LCD_RGBPACK(0,32,8), LCD_RGBPACK(7,32,32), LCD_RGBPACK(32,25,7),
LCD_RGBPACK(27,2,27), LCD_RGBPACK(2,27,16), LCD_RGBPACK(8,12,32),
LCD_RGBPACK(19,10,26) };

#define EXPLOSION_COLOR LCD_RGBPACK(255,240,0)

#endif

static const struct opt_items autofire_delay_settings[15] = {
    { "Off",   -1 },
    { "50ms",  -1 },
    { "100ms", -1 },
    { "200ms", -1 },
    { "300ms", -1 },
    { "400ms", -1 },
    { "500ms", -1 },
    { "600ms", -1 },
    { "700ms", -1 },
    { "800ms", -1 },
    { "900ms", -1 },
    { "1s",    -1 },
    { "2s",    -1 },
    { "3s",    -1 },
    { "4s",    -1 }
};

int autofire_delay_values[15] = {
    0, 5, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 200, 300, 400 };

static const struct opt_items particle_settings[8] = {
    { "5",  -1 },
    { "10", -1 },
    { "15", -1 },
    { "20", -1 },
    { "25", -1 },
    { "30", -1 },
    { "35", -1 },
    { "40", -1 },
};

int particle_values[8] = {
    5, 10, 15, 20, 25, 30, 35, 40 };

static const struct opt_items particle_life_settings[9] = {
    { "20 cycles",  -1 },
    { "30 cycles",  -1 },
    { "40 cycles",  -1 },
    { "50 cycles",  -1 },
    { "60 cycles",  -1 },
    { "70 cycles",  -1 },
    { "80 cycles",  -1 },
    { "90 cycles",  -1 },
    { "100 cycles", -1 }
};

int particle_life_values[9] = {
    20, 30, 40, 50, 60, 70, 80, 90, 100 };

static const struct opt_items gravity_settings[4] = {
    { "Off",      -1 },
    { "Weak",     -1 },
    { "Moderate", -1 },
    { "Strong",   -1 },
};

int gravity_values[4] = {
    0, 30, 20, 10 };

#ifdef HAVE_LCD_COLOR

static const struct opt_items rocket_settings[3] = {
    { "No",                -1 },
    { "Yes (no trails)",   -1 },
    { "Yes (with trails)", -1 },
};
int rocket_values[4] = {
    2, 1, 0 };

#else

static const struct opt_items rocket_settings[2] = {
    { "No",  -1 },
    { "Yes", -1 },
};
int rocket_values[4] = {
    1, 0 };

#endif

static const struct opt_items fps_settings[9] = {
    { "20 FPS", -1 },
    { "25 FPS", -1 },
    { "30 FPS", -1 },
    { "35 FPS", -1 },
    { "40 FPS", -1 },
    { "45 FPS", -1 },
    { "50 FPS", -1 },
    { "55 FPS", -1 },
    { "60 FPS", -1 }
};

int fps_values[9] = {
    20, 25, 30, 35, 40, 45, 50, 55, 60 };

static const struct menu_item items[] = {
    { "Start Demo", NULL },
    { "Auto-Fire", NULL },
    { "Particles Per Firework", NULL },
    { "Particle Life", NULL },
    { "Gravity", NULL },
    { "Show Rockets", NULL },
    { "FPS (Speed)", NULL },
    { "Quit", NULL }
};

/* called on startup. initializes all variables, etc */
void init_all(void)
{
    int j;

    for(j=0; j<MAX_ROCKETS; j++)
        firework_phase[j] = -1;
}

/* called when a rocket hits its destination height.
 * prepares all associated fireworks to be expelled. */
void init_explode(int x, int y, int firework, int points)
{
    int i;

    for(i=0; i<points; i++)
    {
        rb->srand(*rb->current_tick * i);

        firework_xpoints[firework][i] = x;
        firework_ypoints[firework][i] = y;

        firework_xspeed[firework][i] = (rb->rand() % FIREWORK_MOVEMENT_RANGE) - FIREWORK_MOVEMENT_RANGE/2;
        firework_yspeed[firework][i] = (rb->rand() % FIREWORK_MOVEMENT_RANGE) - FIREWORK_MOVEMENT_RANGE/2;

#ifdef HAVE_LCD_COLOR
        firework_color[firework][i] = rb->rand() % 7;
#endif
    }
}

/* called when a rocket is launched.
 * prepares said rocket to start moving towards its destination. */
void init_rocket(int rocket)
{
    rb->srand(*rb->current_tick);

    rocket_xpos[rocket] = rb->rand() % LCD_WIDTH;
    rocket_ypos[rocket] = LCD_HEIGHT;

    rocket_xspeed[rocket] = (rb->rand() % ROCKET_MOVEMENT_RANGE) - ROCKET_MOVEMENT_RANGE/2;
    rocket_yspeed[rocket] = 3;

    rocket_targetphase[rocket] = (ROCKET_LIFE + (rb->rand() % ROCKET_LIFE_VAR)) / rocket_yspeed[rocket];
}

/* startup/configuration menu. */
void fireworks_menu(void)
{
    int m, result;
    bool menu_quit = false;

    rb->lcd_setfont(FONT_UI);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_foreground(LCD_WHITE);
#endif
    rb->lcd_clear_display();
    rb->lcd_update();

    m = menu_init(rb, items, sizeof(items) / sizeof(*items),
                      NULL, NULL, NULL, NULL);

    rb->button_clear_queue();

    while(!menu_quit)
    {
        result = menu_show(m);

        switch(result)
        {
            case 0:
                rb->lcd_setfont(FONT_SYSFIXED);

#ifdef HAVE_LCD_COLOR
                rb->lcd_set_background(LCD_BLACK);
                rb->lcd_set_foreground(LCD_WHITE);
#endif

                rb->lcd_clear_display();
                rb->lcd_update();

                init_all();
                menu_quit = true;
                break;

            case 1:
                rb->set_option("Auto-Fire", &autofire_delay, INT, autofire_delay_settings, 15, NULL);
                break;

            case 2:
                rb->set_option("Particles Per Firework", &particles_per_firework, INT, particle_settings, 8, NULL);
                break;

            case 3:
                rb->set_option("Particle Life", &particle_life, INT, particle_life_settings, 9, NULL);
                break;

            case 4:
                rb->set_option("Gravity", &gravity, INT, gravity_settings, 4, NULL);
                break;

            case 5:
                rb->set_option("Show Rockets", &show_rockets, INT, rocket_settings, 3, NULL);
                break;

            case 6:
                rb->set_option("FPS (Speed)", &frames_per_second, INT, fps_settings, 9, NULL);
                break;

            case 7:
                quit_plugin = true;
                menu_quit = true;
                break;
        }
    }

    menu_exit(m);
}

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    (void)parameter;

    rb = api;

    int j, i, autofire=0;
    int thisrocket=0;
    int start_tick, elapsed_tick;
    int button;

    /* set everything up.. no BL timeout, no backdrop,
       white-text-on-black-background. */
    backlight_force_on(rb); /* backlight control in lib/helper.c */
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_foreground(LCD_WHITE);
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    fireworks_menu();

    start_tick = *rb->current_tick;

    while(!quit_plugin)
    {
        rb->lcd_clear_display();

        /* loop through every possible rocket */
        for(j=0; j<MAX_ROCKETS; j++)
        {
            /* if the current rocket is actually moving/"alive" then go on and
             * move/update/explode it */
            if(rocket_phase[j] > -1)
            {
#ifdef HAVE_LCD_COLOR /* draw trail, if requested */
                if(show_rockets==2)
                {
                    rb->lcd_set_foreground(LCD_RGBPACK(128,128,128));
                    rb->lcd_fillrect(rocket_xpos[j], rocket_ypos[j],
                                     ROCKET_SIZE, ROCKET_SIZE);
                    rb->lcd_set_foreground(LCD_RGBPACK(64,64,64));
                    rb->lcd_fillrect(rocket_xpos[j]-rocket_xspeed[j], rocket_ypos[j]+rocket_yspeed[j],
                                     ROCKET_SIZE, ROCKET_SIZE);
                }
#endif

                /* move rocket */
                rocket_xpos[j] += rocket_xspeed[j];
                rocket_ypos[j] -= rocket_yspeed[j];

#ifdef HAVE_LCD_COLOR
                rb->lcd_set_foreground(LCD_WHITE);
#endif
                if(show_rockets==2 || show_rockets==1)
                    rb->lcd_fillrect(rocket_xpos[j], rocket_ypos[j],
                                    ROCKET_SIZE, ROCKET_SIZE);

                /* if(rocket isn't "there" yet) keep moving
                 * if(rocket IS there) explode it. */
                if(rocket_phase[j] < rocket_targetphase[j])
                    rocket_phase[j]++;
                else
                {
                    rocket_phase[j] = -1;

                    firework_phase[j] = 0;
                    init_explode(rocket_xpos[j], rocket_ypos[j], j, particle_values[particles_per_firework]);
                }
            }

            /* and now onto the fireworks for this particular rocket... */
            if(firework_phase[j] > -1)
            {
                for(i=0; i<particle_values[particles_per_firework]; i++)
                {
                    firework_xpoints[j][i] += firework_xspeed[j][i];
                    firework_ypoints[j][i] += firework_yspeed[j][i];

                    if(gravity != 0)
                        firework_ypoints[j][i] += firework_phase[j]/gravity_values[gravity];

#ifdef HAVE_LCD_COLOR
                    rb->lcd_set_foreground(firework_darkest_colors[firework_color[j][i]]);
                    rb->lcd_fillrect(firework_xpoints[j][i]-1,
                                     firework_ypoints[j][i]-1,
                                     FIREWORK_SIZE+2, FIREWORK_SIZE+2);

                    if(firework_phase[j] < particle_life_values[particle_life]-10)
                        rb->lcd_set_foreground(firework_colors[firework_color[j][i]]);
                    else if(firework_phase[j] < particle_life_values[particle_life]-7)
                        rb->lcd_set_foreground(firework_dark_colors[firework_color[j][i]]);
                    else if(firework_phase[j] < particle_life_values[particle_life]-3)
                        rb->lcd_set_foreground(firework_darker_colors[firework_color[j][i]]);
                    else
                        rb->lcd_set_foreground(firework_darkest_colors[firework_color[j][i]]);
#endif
                    rb->lcd_fillrect(firework_xpoints[j][i],
                                     firework_ypoints[j][i],
                                     FIREWORK_SIZE, FIREWORK_SIZE);
/* WIP - currently ugly explosion effect
#ifdef HAVE_LCD_COLOR
                    if(firework_phase[j] < 10)
                    {
                        rb->lcd_set_foreground(EXPLOSION_COLOR);
                        rb->lcd_fillrect(rocket_xpos[j]-firework_phase[j],
                                         rocket_ypos[j]-firework_phase[j],
                                         firework_phase[j]*2, firework_phase[j]*2);
                    }
#endif */
                }

#ifdef HAVE_LCD_COLOR
                    rb->lcd_set_foreground(LCD_WHITE);
#endif

                /* firework at its destination age?
                 * no = keep aging; yes = delete it. */
                if(firework_phase[j] < particle_life_values[particle_life])
                    firework_phase[j]++;
                else
                    firework_phase[j] = -1;
            }
        }

        /* is autofire on? */
        if(autofire_delay != 0)
        {
            elapsed_tick = *rb->current_tick - start_tick;

            if(elapsed_tick > autofire_delay_values[autofire_delay])
            {
                rocket_phase[autofire] = 0;
                init_rocket(autofire);

                start_tick = *rb->current_tick;

                if(autofire < MAX_ROCKETS)
                    autofire++;
                else
                    autofire = 0;
            }
        }

        rb->lcd_update();

        button = rb->button_get_w_tmo(HZ/fps_values[frames_per_second]);
        switch(button)
        {
            case BTN_MENU: /* back to config menu */
                fireworks_menu();
                break;

            case BTN_FIRE: /* fire off rockets manually */
            case BTN_FIRE|BUTTON_REPEAT:
                if(thisrocket < MAX_ROCKETS)
                    thisrocket++;
                else
                    thisrocket=0;

                rocket_phase[thisrocket] = 0;
                init_rocket(thisrocket);
                break;
        }
    }
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(rb); /* backlight control in lib/helper.c */

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    return PLUGIN_OK;
}
