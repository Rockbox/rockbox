/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Franklin Wei
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
#include <plugin.h>
#include <stdlib.h>
#include "lib/pluginlib_actions.h"
#include "lib/playback_control.h"
/* Key definitions */
#define KEY_QUIT PLA_CANCEL
#define KEY_UP PLA_UP
#define KEY_DOWN PLA_DOWN
#define KEY_LEFT PLA_LEFT
#define KEY_RIGHT PLA_RIGHT
#define KEY_SELECT PLA_SELECT

/* Settings */
#define NUM_BALLS 1
/* Version number is major.minor.bugfix-codefix */
#define PLUGIN_NAME_VERSION "RockPhysics v1.4"
static double gravity=.5, elasticity=.8, friction=.95;
static int grav_int=2, elas_int=8, fric_int=7, ball_size_junk=1;
static int manual_speed=1, ball_size=2;
static bool backlight_status=true;
static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

/* Utility functions */
static void pxl_on(int x, int y)
{
  rb->lcd_set_foreground(LCD_WHITE);
  rb->lcd_fillrect(x, y, ball_size,ball_size);
}
static void pxl_off(int x, int y)
{
  rb->lcd_set_foreground(LCD_BLACK);
  rb->lcd_fillrect(x,y,ball_size,ball_size);
}
static signed int rand_range(int min, int max)
{
  return rb->rand()%(max-min)+min;
}

/* Physics implementation */
struct ball {
  double x, y, dx, dy, elasticity;
};
struct ball balls[NUM_BALLS];
struct ball next_world[NUM_BALLS];

/* this function is what allows for extreme high friction/elasticity */
static void check_ball_off_screen(struct ball* b)
{
  if(b->x<0)
    b->x=0;
  else if(b->x>LCD_WIDTH-ball_size)
    b->x=LCD_WIDTH-ball_size;
  
  if(b->y<0)
    b->y=0;
  else if(b->y>LCD_HEIGHT-ball_size)
    b->y=LCD_HEIGHT-ball_size;
}
/* WIP: */

/* static void do_collision(int idx, int otheridx) */
/* { */
/*   struct ball *b=&balls[idx], *out=&next_world[idx]; */
/*   struct ball *other=&balls[otheridx], *otherout=&next_world[idx]; */
/*   if(other->x-b->x <= ball_size && other->y-b->y <= ball_size) */
/*     { */
/*       /\* The two are touching *\/ */
/*       if(b->x<other->x) */
/*         { */
/*           out->x=-out->x; */
/*           otherout->x=-other->x; */
/*         } */
/*     } */
/* } */

/* static void check_collide(int idx) */
/* { */
/* #if NUM_BALLS > 1 */
/*   struct ball *b=&balls[idx], *out=&next_world[idx]; */
/*   /\* Using two loops should be more efficient than one loop and a double-increment *\/ */
/*   for(int i=0;i<idx;++i) */
/*     { */
/*       do_collision_detect(idx, i); */
/*     } */
/*   for(int i=idx+1;i<NUM_BALLS;++i) */
/*     { */
/*       do_collision_detect(idx, i); */
/*     } */
/* #endif */
/* } */
/* Move the ball, bounce if necessary */
static void step(int idx)
{
  struct ball *b=&balls[idx], *out=&next_world[idx];
  int oldx=b->x, oldy=b->y;
  /* Find new position */
  out->x+=b->dx;
  out->y+=b->dy;
  /* Calculate collision with walls */
  if(out->x<=0 || out->x>=LCD_WIDTH-ball_size)
    {
      out->dx=-b->dx*b->elasticity;
      out->x+=2*b->dx;
      out->dy*=friction;
    }

  if(out->y<=0 || out->y>=LCD_HEIGHT-ball_size)
    {
      /* Find new velocities */
      out->dy=-b->dy*b->elasticity;
      out->y+=2*b->dy;
      /* Calculate friction on other velocity */
      out->dx*=friction;
    }

  if(gravity>=0)
    {
      if(out->y<=LCD_HEIGHT-ball_size-gravity)
        out->dy+=gravity;
    }
  else /* negative gravity! */
    {
      if(out->y>=0+gravity)
        out->dy+=gravity;
    }
  /*check_collide(idx);*/
  check_ball_off_screen(out);
  pxl_off(oldx, oldy);
  /* Drawing is handled in run_sim now */
  /*pxl_on(b->x, b->y);*/
}

static void run_sim(void)
{
  rb->lcd_update();
  for(int i=0;i<NUM_BALLS;++i)
    {
      balls[i].x=rand_range(0, LCD_WIDTH-ball_size);
      balls[i].y=rand_range(0, LCD_HEIGHT-ball_size-gravity);
      balls[i].dx=rand_range(-10, 10);
      balls[i].dy=rand_range(-10, 10);
      balls[i].elasticity=elasticity;
    }
  for(;;)
    {
      /* Process keypress */
      int button = pluginlib_getaction(0, plugin_contexts,
                                       ARRAYLEN(plugin_contexts));
      switch(button)
        {
        case 0:
          break;
        case KEY_QUIT:
          return;
        case KEY_UP:
          {
            for(int i=0;i<NUM_BALLS;++i)
              {
                balls[i].dy-=manual_speed;
              }
            break;
          }
        case KEY_LEFT:
          {
            for(int i=0;i<NUM_BALLS;++i)
              {
                balls[i].dx-=manual_speed;
              }
            break;
          }
        case KEY_RIGHT:
          {
            for(int i=0;i<NUM_BALLS;++i)
              {
                balls[i].dx+=manual_speed;
              }
            break;
          }
        case KEY_DOWN:
          {
            for(int i=0;i<NUM_BALLS;++i)
              {
                balls[i].dy+=manual_speed;
              }
            break;
          }
        default:  
          if (rb->default_event_handler(button) == SYS_USB_CONNECTED) 
            {
              exit(PLUGIN_USB_CONNECTED);
            }
          break;
        }
      memcpy(&next_world, &balls, NUM_BALLS*sizeof(struct ball));
      for(int i=0;i<NUM_BALLS;++i)
        {
          step(i);
        }
      memcpy(&balls, &next_world, NUM_BALLS*sizeof(struct ball));
      for(int i=0;i<NUM_BALLS;++i)
        {
          pxl_on(balls[i].x,balls[i].y);
        }
      rb->lcd_update();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
      rb->cpu_boost(true);
#endif
      rb->sleep(HZ/100);
      rb->yield();
    }
}

/* Settings */
static const struct opt_items gravity_settings[7]={
  {"Negative", 0},
  {"Zero-G", 1},
  {"Moon", 2},
  {"Earth", 3},
  {"Saturn", 4},
  {"Jupiter", 5},
  {"Sun", 6}
};
static int grav_values[7]={-5, 0, 5, 10, 20, 40, 80};
static void set_real_grav(int sel)
{
  gravity=(double)grav_values[sel]/10;
}
static const struct opt_items elasticity_settings[11]={
  {"0%", 0},
  {"10%", 1},
  {"20%", 2},
  {"30%", 3},
  {"40%", 4},
  {"50%", 5},
  {"60%", 6},
  {"70%", 7},
  {"80%", 8},
  {"90%", 9},
  {"100%", 10}
};
static const int elas_values[]={0, 1, 2,3,4,5,6,7,8,9,10};
static void set_real_elas(int sel)
{
  elasticity=(double)elas_values[sel]/10;
}
static const struct opt_items friction_settings[10]={
  {"0% (most)", 0},
  {"25%", 1},
  {"50%", 2},
  {"75%", 3},
  {"80%", 4},
  {"85%", 5},
  {"90%", 6},
  {"95%", 7},
  {"98%", 8},
  {"100% (none)", 9}
};
static int fric_values[]={0, 25, 50, 75, 80, 85, 90, 95, 98, 100};
static void set_real_friction(int sel)
{
  friction=(double)fric_values[sel]/100;
}
static const struct opt_items manual_speed_settings[]={
  {"Disabled", 0},
  {"1", 1},
  {"2", 2},
  {"3", 3},
  {"4", 4},
  {"5", 5},
  {"6", 6},
  {"7", 7},
  {"8", 8},
  {"9", 9},
  {"10", 10}
};
static const struct opt_items ball_size_settings[]={
  {"1 pixel", 1},
  {"2 pixels", 2},
  {"3 pixels", 3},
  {"4 pixels", 4},
  {"5 pixels", 5},
  {"6 pixels", 6},
  {"7 pixels", 7},
  {"8 pixels", 8},
  {"9 pixels", 9},
  {"10 pixels", 10}
};
static void set_real_ball_size(int opt)
{
  ball_size=opt+1;
}
static void rockphys_menu(void)
{
  MENUITEM_STRINGLIST(menu, PLUGIN_NAME_VERSION, NULL, "Start Demo", "Gravity", "Elasticity", "Friction", "Manual Speed", "Ball Size", "Playback Control", "About", "Exit");
 menu_start:
  switch(rb->do_menu(&menu, NULL, NULL, false)) /* returns index of selected option */
    {
    case 0: /* start */
      rb->lcd_clear_display();
      run_sim();
      rb->lcd_clear_display();
      goto menu_start;
    case 1: /* gravity */
      rb->set_option("Gravity", &grav_int, INT,
                     gravity_settings,7, &set_real_grav);
      goto menu_start;
    case 2: /* elasticity */
      rb->set_option("Elasticity", &elas_int, INT,
                     elasticity_settings, 11, &set_real_elas);
      goto menu_start;
    case 3: /* friction */
      rb->set_option("Friction", &fric_int, INT,
                     friction_settings,10, &set_real_friction);
      goto menu_start;
    case 4: /* manual speed */
      rb->set_option("Manual Speed", &manual_speed, INT,
                     manual_speed_settings,11, NULL);
      goto menu_start;
    case 5: /* ball size */
      rb->set_option("Ball Size", &ball_size_junk, INT, ball_size_settings, 10, &set_real_ball_size);
      goto menu_start;
    case 6: /* playback control */
      playback_control(NULL);
      goto menu_start;
    case 7: /* about */
      rb->lcd_clear_display();
      rb->lcd_putsxy(0,0,PLUGIN_NAME_VERSION);
      rb->lcd_putsxy(0,11,"(C) Franklin Wei");
      rb->lcd_putsxy(0,22,"Any key to exit...");
      rb->lcd_update();
      rb->button_get(true); /* wait for a button press */
      goto menu_start;
    case 8: /* quit */
      return;
    }
}

enum plugin_status plugin_start(const void* parameter)
{
  (void)parameter;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
  rb->cpu_boost(true);
#endif
  backlight_status=rb->is_backlight_on(true);
  rb->srand((*rb->current_tick));
  rb->lcd_clear_display();
  rockphys_menu();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
  rb->cpu_boost(false);
#endif
  return PLUGIN_OK; /* make GCC happy */
}
