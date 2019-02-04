/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Zakk Roberts
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
#include "lib/playback_control.h"
#include "lib/pluginlib_actions.h"

/* this set the context to use with PLA */
static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

/***
 * FIREWORKS.C by ZAKK ROBERTS
 * Rockbox plugin simulating a fireworks display.
 * Supports all bitmap LCDs, fully scalable.
 * Currently disabled for Archos Recorder - runs too slow.
 ***/

/* We use PLA */
#define BTN_EXIT           PLA_EXIT
#define BTN_MENU           PLA_CANCEL
#define BTN_FIRE           PLA_SELECT
#define BTN_FIRE_REPEAT    PLA_SELECT_REPEAT

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
int firework_xpoints[MAX_ROCKETS][MAX_FIREWORKS];
int firework_ypoints[MAX_ROCKETS][MAX_FIREWORKS];
int firework_xspeed[MAX_ROCKETS][MAX_FIREWORKS];
int firework_yspeed[MAX_ROCKETS][MAX_FIREWORKS];
int firework_phase[MAX_ROCKETS];
#ifdef HAVE_LCD_COLOR
int firework_color[MAX_ROCKETS][MAX_FIREWORKS];
#endif

/* position, speed, "phase" (age) of all rockets */
int rocket_xpos[MAX_ROCKETS];
int rocket_ypos[MAX_ROCKETS];
int rocket_xspeed[MAX_ROCKETS];
int rocket_yspeed[MAX_ROCKETS];
int rocket_phase[MAX_ROCKETS];
int rocket_targetphase[MAX_ROCKETS];

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
    { STR(LANG_OFF) },
    { "50ms",  TALK_ID(50, UNIT_MS) },
    { "100ms", TALK_ID(100, UNIT_MS) },
    { "200ms", TALK_ID(200, UNIT_MS) },
    { "300ms", TALK_ID(300, UNIT_MS) },
    { "400ms", TALK_ID(400, UNIT_MS) },
    { "500ms", TALK_ID(500, UNIT_MS) },
    { "600ms", TALK_ID(600, UNIT_MS) },
    { "700ms", TALK_ID(700, UNIT_MS) },
    { "800ms", TALK_ID(800, UNIT_MS) },
    { "900ms", TALK_ID(900, UNIT_MS) },
    { "1s",    TALK_ID(1, UNIT_SEC) },
    { "2s",    TALK_ID(2, UNIT_SEC) },
    { "3s",    TALK_ID(3, UNIT_SEC) },
    { "4s",    TALK_ID(4, UNIT_SEC) }
};

int autofire_delay_values[15] = {
    0, 5, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 200, 300, 400 };

static const struct opt_items particle_settings[8] = {
    { "5",  TALK_ID(5, UNIT_INT) },
    { "10", TALK_ID(10, UNIT_INT) },
    { "15", TALK_ID(15, UNIT_INT) },
    { "20", TALK_ID(20, UNIT_INT) },
    { "25", TALK_ID(25, UNIT_INT) },
    { "30", TALK_ID(30, UNIT_INT) },
    { "35", TALK_ID(35, UNIT_INT) },
    { "40", TALK_ID(40, UNIT_INT) },
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
    { STR(LANG_OFF)    },
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
    { STR(LANG_SET_BOOL_NO) },
    { STR(LANG_SET_BOOL_YES) },
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

MENUITEM_STRINGLIST(menu, "Fireworks Menu", NULL,
                    "Start Demo", "Auto-Fire", "Particles Per Firework",
                    "Particle Life", "Gravity", "Show Rockets",
                    "FPS (Speed)", "Playback Control", "Quit");

/* called on startup. initializes all variables, etc */
static void init_all(void)
{
    int j;

    for(j=0; j<MAX_ROCKETS; j++)
    {
        rocket_phase[j] = -1;
        firework_phase[j] = -1;
    }
}

/* called when a rocket hits its destination height.
 * prepares all associated fireworks to be expelled. */
static void init_explode(int x, int y, int firework, int points)
{
    int i;

    for(i=0; i<points; i++)
    {
        rb->srand(*rb->current_tick * i);

        firework_xpoints[firework][i] = x;
        firework_ypoints[firework][i] = y;

        firework_xspeed[firework][i] = (rb->rand() % FIREWORK_MOVEMENT_RANGE)
                                            - FIREWORK_MOVEMENT_RANGE/2;
        firework_yspeed[firework][i] = (rb->rand() % FIREWORK_MOVEMENT_RANGE)
                                            - FIREWORK_MOVEMENT_RANGE/2;

#ifdef HAVE_LCD_COLOR
        firework_color[firework][i] = rb->rand() % 7;
#endif
    }
}

/* called when a rocket is launched.
 * prepares said rocket to start moving towards its destination. */
static void init_rocket(int rocket)
{
    rb->srand(*rb->current_tick);

    rocket_xpos[rocket] = rb->rand() % LCD_WIDTH;
    rocket_ypos[rocket] = LCD_HEIGHT;

    rocket_xspeed[rocket] = (rb->rand() % ROCKET_MOVEMENT_RANGE)
                                - ROCKET_MOVEMENT_RANGE/2;
    rocket_yspeed[rocket] = 3;

    rocket_phase[rocket] = 0;
    rocket_targetphase[rocket] = (ROCKET_LIFE + (rb->rand() % ROCKET_LIFE_VAR))
                                    / rocket_yspeed[rocket];
}

/* startup/configuration menu. */
static void fireworks_menu(void)
{
    int selected = 0, result;
    bool menu_quit = false;

    rb->lcd_setfont(FONT_UI);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_foreground(LCD_WHITE);
#endif
    rb->lcd_clear_display();
    rb->lcd_update();

    rb->button_clear_queue();

    while(!menu_quit)
    {
        result = rb->do_menu(&menu, &selected, NULL, false);

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
                rb->set_option("Auto-Fire", &autofire_delay, INT,
                                autofire_delay_settings, 15, NULL);
                break;

            case 2:
                rb->set_option("Particles Per Firework", &particles_per_firework,
                                INT, particle_settings, 8, NULL);
                break;

            case 3:
                rb->set_option("Particle Life", &particle_life, INT,
                                particle_life_settings, 9, NULL);
                break;

            case 4:
                rb->set_option("Gravity", &gravity, INT,
                                gravity_settings, 4, NULL);
                break;

            case 5:
                rb->set_option("Show Rockets", &show_rockets, INT,
                                rocket_settings, 3, NULL);
                break;

            case 6:
                rb->set_option("FPS (Speed)", &frames_per_second, INT,
                                fps_settings, 9, NULL);
                break;

            case 7:
                playback_control(NULL);
                break;

            case 8:
                quit_plugin = true;
                menu_quit = true;
                break;
        }
    }
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;

    int j, i;
    int thisrocket=0;
    int start_tick, elapsed_tick;
    int button;

    /* set everything up.. no BL timeout, no backdrop,
       white-text-on-black-background. */
    backlight_ignore_timeout();
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
                    rb->lcd_fillrect(rocket_xpos[j]-rocket_xspeed[j],
                                     rocket_ypos[j]+rocket_yspeed[j],
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
                    init_explode(rocket_xpos[j], rocket_ypos[j], j,
                                particle_values[particles_per_firework]);
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
                        firework_ypoints[j][i] += firework_phase[j]
                                                    /gravity_values[gravity];

#ifdef HAVE_LCD_COLOR
                    rb->lcd_set_foreground(
                        firework_darkest_colors[firework_color[j][i]]);
                    rb->lcd_fillrect(firework_xpoints[j][i]-1,
                                     firework_ypoints[j][i]-1,
                                     FIREWORK_SIZE+2, FIREWORK_SIZE+2);

                    int phase_left = particle_life_values[particle_life]
                                        - firework_phase[j];
                    if(phase_left > 10)
                        rb->lcd_set_foreground(
                            firework_colors[firework_color[j][i]]);
                    else if(phase_left > 7)
                        rb->lcd_set_foreground(
                            firework_dark_colors[firework_color[j][i]]);
                    else if(phase_left > 3)
                        rb->lcd_set_foreground(
                            firework_darker_colors[firework_color[j][i]]);
                    else
                        rb->lcd_set_foreground(
                            firework_darkest_colors[firework_color[j][i]]);
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
                init_rocket(thisrocket);
                if(++thisrocket == MAX_ROCKETS)
                    thisrocket = 0;

                start_tick = *rb->current_tick;
            }
        }

        rb->lcd_update();

        button = pluginlib_getaction(HZ/fps_values[frames_per_second], plugin_contexts,
                               ARRAYLEN(plugin_contexts));

        switch(button)
        {
            case BTN_EXIT: /* exit directly */
                quit_plugin = true;
                break;

            case BTN_MENU: /* back to config menu */
                fireworks_menu();
                break;

            case BTN_FIRE: /* fire off rockets manually */
            case BTN_FIRE_REPEAT:
                init_rocket(thisrocket);
                if(++thisrocket == MAX_ROCKETS)
                    thisrocket=0;
                break;
        }
    }
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    return PLUGIN_OK;
}
