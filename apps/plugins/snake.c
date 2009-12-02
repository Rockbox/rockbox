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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*
Snake!

by Itai Shaked

ok, a little explanation - 
board holds the snake and apple position - 1+ - snake body (the number
represents the age [1 is the snake's head]).
-1 is an apple, and 0 is a clear spot.
dir is the current direction of the snake - 0=up, 1=right, 2=down, 3=left;

*/

#include "plugin.h"
#ifdef HAVE_LCD_BITMAP
#include "lib/configfile.h"
#include "lib/playback_control.h"

PLUGIN_HEADER

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define SNAKE_QUIT BUTTON_OFF
#define SNAKE_LEFT BUTTON_LEFT
#define SNAKE_RIGHT BUTTON_RIGHT
#define SNAKE_UP   BUTTON_UP
#define SNAKE_DOWN BUTTON_DOWN
#define SNAKE_PLAYPAUSE BUTTON_PLAY

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define SNAKE_QUIT BUTTON_OFF
#define SNAKE_LEFT BUTTON_LEFT
#define SNAKE_RIGHT BUTTON_RIGHT
#define SNAKE_UP   BUTTON_UP
#define SNAKE_DOWN BUTTON_DOWN
#define SNAKE_PLAYPAUSE BUTTON_SELECT

#elif CONFIG_KEYPAD == ONDIO_PAD
#define SNAKE_QUIT BUTTON_OFF
#define SNAKE_LEFT BUTTON_LEFT
#define SNAKE_RIGHT BUTTON_RIGHT
#define SNAKE_UP   BUTTON_UP
#define SNAKE_DOWN BUTTON_DOWN
#define SNAKE_PLAYPAUSE BUTTON_MENU

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define SNAKE_QUIT BUTTON_OFF
#define SNAKE_LEFT BUTTON_LEFT
#define SNAKE_RIGHT BUTTON_RIGHT
#define SNAKE_UP   BUTTON_UP
#define SNAKE_DOWN BUTTON_DOWN
#define SNAKE_PLAYPAUSE BUTTON_ON

#define SNAKE_RC_QUIT BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define SNAKE_QUIT (BUTTON_SELECT|BUTTON_MENU)
#define SNAKE_LEFT BUTTON_LEFT
#define SNAKE_RIGHT BUTTON_RIGHT
#define SNAKE_UP   BUTTON_MENU
#define SNAKE_DOWN BUTTON_PLAY
#define SNAKE_PLAYPAUSE BUTTON_SELECT

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define SNAKE_QUIT BUTTON_POWER
#define SNAKE_LEFT BUTTON_LEFT
#define SNAKE_RIGHT BUTTON_RIGHT
#define SNAKE_UP   BUTTON_UP
#define SNAKE_DOWN BUTTON_DOWN
#define SNAKE_PLAYPAUSE BUTTON_PLAY

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define SNAKE_QUIT BUTTON_POWER
#define SNAKE_LEFT BUTTON_LEFT
#define SNAKE_RIGHT BUTTON_RIGHT
#define SNAKE_UP   BUTTON_UP
#define SNAKE_DOWN BUTTON_DOWN
#define SNAKE_PLAYPAUSE BUTTON_SELECT

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
    (CONFIG_KEYPAD == SANSA_C200_PAD) || \
    (CONFIG_KEYPAD == SANSA_CLIP_PAD) || \
    (CONFIG_KEYPAD == SANSA_M200_PAD)
#define SNAKE_QUIT BUTTON_POWER
#define SNAKE_LEFT BUTTON_LEFT
#define SNAKE_RIGHT BUTTON_RIGHT
#define SNAKE_UP   BUTTON_UP
#define SNAKE_DOWN BUTTON_DOWN
#define SNAKE_PLAYPAUSE BUTTON_SELECT

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#define SNAKE_QUIT (BUTTON_HOME|BUTTON_REPEAT)
#define SNAKE_LEFT BUTTON_LEFT
#define SNAKE_RIGHT BUTTON_RIGHT
#define SNAKE_UP   BUTTON_UP
#define SNAKE_DOWN BUTTON_DOWN
#define SNAKE_PLAYPAUSE BUTTON_SELECT

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define SNAKE_QUIT BUTTON_POWER
#define SNAKE_LEFT BUTTON_LEFT
#define SNAKE_RIGHT BUTTON_RIGHT
#define SNAKE_UP   BUTTON_SCROLL_UP
#define SNAKE_DOWN BUTTON_SCROLL_DOWN
#define SNAKE_PLAYPAUSE BUTTON_PLAY

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#define SNAKE_QUIT BUTTON_BACK
#define SNAKE_LEFT BUTTON_LEFT
#define SNAKE_RIGHT BUTTON_RIGHT
#define SNAKE_UP   BUTTON_UP
#define SNAKE_DOWN BUTTON_DOWN
#define SNAKE_PLAYPAUSE BUTTON_SELECT

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#define SNAKE_QUIT BUTTON_POWER
#define SNAKE_LEFT BUTTON_LEFT
#define SNAKE_RIGHT BUTTON_RIGHT
#define SNAKE_UP   BUTTON_UP
#define SNAKE_DOWN BUTTON_DOWN
#define SNAKE_PLAYPAUSE BUTTON_SELECT

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define SNAKE_QUIT BUTTON_RC_REC
#define SNAKE_LEFT BUTTON_RC_REW
#define SNAKE_RIGHT BUTTON_RC_FF
#define SNAKE_UP   BUTTON_RC_VOL_UP
#define SNAKE_DOWN BUTTON_RC_VOL_DOWN
#define SNAKE_PLAYPAUSE BUTTON_RC_PLAY

#define SNAKE_RC_QUIT BUTTON_REC

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define SNAKE_QUIT BUTTON_BACK
#define SNAKE_LEFT BUTTON_LEFT
#define SNAKE_RIGHT BUTTON_RIGHT
#define SNAKE_UP   BUTTON_UP
#define SNAKE_DOWN BUTTON_DOWN
#define SNAKE_PLAYPAUSE BUTTON_PLAY

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define SNAKE_QUIT BUTTON_POWER
#define SNAKE_LEFT BUTTON_LEFT
#define SNAKE_RIGHT BUTTON_RIGHT
#define SNAKE_UP   BUTTON_UP
#define SNAKE_DOWN BUTTON_DOWN
#define SNAKE_PLAYPAUSE BUTTON_MENU

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define SNAKE_QUIT BUTTON_POWER
#define SNAKE_LEFT BUTTON_PREV
#define SNAKE_RIGHT BUTTON_NEXT
#define SNAKE_UP   BUTTON_UP
#define SNAKE_DOWN BUTTON_DOWN
#define SNAKE_PLAYPAUSE BUTTON_MENU

#elif CONFIG_KEYPAD == SAMSUNG_YH_PAD
#define SNAKE_QUIT      BUTTON_REC
#define SNAKE_LEFT      BUTTON_LEFT
#define SNAKE_RIGHT     BUTTON_RIGHT
#define SNAKE_UP        BUTTON_UP
#define SNAKE_DOWN      BUTTON_DOWN
#define SNAKE_PLAYPAUSE BUTTON_PLAY

#elif CONFIG_KEYPAD == MROBE500_PAD
#define SNAKE_QUIT BUTTON_POWER
#define SNAKE_RC_QUIT BUTTON_RC_DOWN

#elif (CONFIG_KEYPAD == ONDAVX747_PAD)
#define SNAKE_QUIT BUTTON_POWER

#elif (CONFIG_KEYPAD == ONDAVX777_PAD)
#define SNAKE_QUIT BUTTON_POWER

#elif CONFIG_KEYPAD == COWOND2_PAD
#define SNAKE_QUIT BUTTON_POWER

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef SNAKE_QUIT
#define SNAKE_QUIT      BUTTON_TOPLEFT
#endif
#ifndef SNAKE_LEFT
#define SNAKE_LEFT      BUTTON_MIDLEFT
#endif
#ifndef SNAKE_RIGHT
#define SNAKE_RIGHT     BUTTON_MIDRIGHT
#endif
#ifndef SNAKE_UP
#define SNAKE_UP        BUTTON_TOPMIDDLE
#endif
#ifndef SNAKE_DOWN
#define SNAKE_DOWN      BUTTON_BOTTOMMIDDLE
#endif
#ifndef SNAKE_PLAYPAUSE
#define SNAKE_PLAYPAUSE BUTTON_CENTER
#endif
#endif

#define BOARD_WIDTH (LCD_WIDTH/4)
#define BOARD_HEIGHT (LCD_HEIGHT/4)

static int board[BOARD_WIDTH][BOARD_HEIGHT],snakelength;
static unsigned int score,hiscore=0,level=1;
static int dir;
static bool apple, dead;

#define CONFIG_FILE_NAME "snake.cfg"
static struct configdata config[] = {
   {TYPE_INT, 0, 10, { .int_p = &level }, "level", NULL},
   {TYPE_INT, 0, 10000, { .int_p = &hiscore }, "hiscore", NULL},
};

static void snake_die (void)
{
    char pscore[17];
    rb->lcd_clear_display();
    rb->snprintf(pscore,sizeof(pscore),"Your score: %d",score);
    rb->lcd_puts(0,0,"Oops...");
    rb->lcd_puts(0,1, pscore);
    if (score>hiscore) {
        hiscore=score;
        rb->lcd_puts(0,2,"New High Score!");
    }
    else {
        rb->snprintf(pscore,sizeof(pscore),"High Score: %d",hiscore);
        rb->lcd_puts(0,2,pscore);
    }
    rb->lcd_update();
    rb->sleep(3*HZ);
    dead=true;
}

static void snake_colission (short x, short y)
{
    if (x==BOARD_WIDTH || x<0 || y==BOARD_HEIGHT || y<0) {
        snake_die();
        return;
    }
    switch (board[x][y]) {
        case 0:
            break;
        case -1:
            snakelength+=2;
            score+=level;
            apple=false;
            break;
        default:
            snake_die();
            break;
    }
}

static void snake_move_head (short x, short y)
{
    switch (dir) {
        case 0:
            y-=1;
            break;
        case 1:
            x+=1;
            break;
        case 2:
            y+=1;
            break;
        case 3:
            x-=1;
            break;
    }
    snake_colission (x,y);
    if (dead)
        return;
    board[x][y]=1;
    rb->lcd_fillrect(x*4,y*4,4,4);
}

static void snake_redraw (void)
{
    short x,y;
    rb->lcd_clear_display();
    for (x=0; x<BOARD_WIDTH; x++) {
        for (y=0; y<BOARD_HEIGHT; y++) {
            switch (board[x][y]) {
                case -1:
                    rb->lcd_fillrect((x*4)+1,y*4,2,4);
                    rb->lcd_fillrect(x*4,(y*4)+1,4,2);
                    break;
                case 0:
                    break;
                default:
                    rb->lcd_fillrect(x*4,y*4,4,4);
                    break;
            }
        }
    }
    rb->lcd_update();
}

static void snake_frame (void)
{
    short x,y,head=0;
    for (x=0; x<BOARD_WIDTH; x++) {
        for (y=0; y<BOARD_HEIGHT; y++) {
            switch (board[x][y]) {
                case 1:
                    if (!head) {
                        snake_move_head(x,y);
                        if (dead)
                            return;
                        board[x][y]++;
                        head=1;
                    }
                    break;
                case 0:
                    break;
                case -1:
                    break;
                default:
                    if (board[x][y]==snakelength) {
                        board[x][y]=0;
                        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                        rb->lcd_fillrect(x*4,y*4,4,4);
                        rb->lcd_set_drawmode(DRMODE_SOLID);
                    }
                    else
                        board[x][y]++;
                    break;
            }
        }
    }
    rb->lcd_update();
}

static void snake_game_init(void) {
    short x,y;
    rb->lcd_clear_display();

    for (x=0; x<BOARD_WIDTH; x++) {
        for (y=0; y<BOARD_HEIGHT; y++) {
            board[x][y]=0;
        }
    }
    apple=false;
    dead=false;
    snakelength=4;
    score=0;
    board[11][7]=1;   
}

static void snake_choose_level(void)
{
    rb->set_int("Snake Speed", "", UNIT_INT, &level, NULL, 1, 1, 9, NULL);
}

static bool _ingame;
static int snake_menu_cb(int action, const struct menu_item_ex *this_item)
{
    if(action == ACTION_REQUEST_MENUITEM
       && !_ingame && ((intptr_t)this_item)==0)
        return ACTION_EXIT_MENUITEM;
    return action;
}

static int snake_game_menu(bool ingame)
{
    rb->button_clear_queue();
    int choice = 0;
    
    _ingame = ingame;

    MENUITEM_STRINGLIST(main_menu,"Snake Menu",snake_menu_cb,
                        "Resume Game",
                        "Start New Game",
                        "Snake Speed",
                        "High Score",
                        "Playback Control",
                        "Quit");
                             
    while (true) {
        choice = rb->do_menu(&main_menu, &choice, NULL, false);
        switch (choice) {
            case 0:
                snake_redraw();
                return 0;
            case 1:
                snake_game_init();
                return 0;
            case 2:
                snake_choose_level();
                break;
            case 3:
                rb->splashf(HZ*2, "High Score: %d", hiscore);
                break;
            case 4:
                playback_control(NULL);
                break;
            case 5:
                return 1;
            case MENU_ATTACHED_USB:
                return 1;
            default:
                break;
        }
    }
}

static int snake_game_loop (void) {
    int button;
    short x,y;
    bool pause = false; 
    
    if (snake_game_menu(false)==1)
        return 1;

    while (true) {
        if (!pause) {
            snake_frame();
            if (dead) {
                if (snake_game_menu(false)==1)
                    return 1;
            }
            if (!apple) {
                do {
                    x=rb->rand() % BOARD_WIDTH;
                    y=rb->rand() % BOARD_HEIGHT;
                } while (board[x][y]);
                apple=true;
                board[x][y]=-1;
                rb->lcd_fillrect((x*4)+1,y*4,2,4);
                rb->lcd_fillrect(x*4,(y*4)+1,4,2);
            }

            rb->sleep(HZ/level);
        }

        button=rb->button_get(false);
        
#ifdef HAS_BUTTON_HOLD
        if (rb->button_hold()) {
            pause = true;
            rb->splash (HZ, "Paused");
        }
#endif
        switch (button) {
             case SNAKE_UP:
                 if (dir!=2) dir=0;
                 break;
             case SNAKE_RIGHT:
                 if (dir!=3) dir=1;
                 break;
             case SNAKE_DOWN:
                 if (dir!=0) dir=2;
                 break;
             case SNAKE_LEFT:
                 if (dir!=1) dir=3;
                 break;
            case SNAKE_PLAYPAUSE:
                pause = !pause;
                if (pause)
                    rb->splash (HZ, "Paused");
                else
                    snake_redraw();
                break;
#ifdef SNAKE_RC_QUIT
             case SNAKE_RC_QUIT:
#endif
             case SNAKE_QUIT:
                    pause = false;
                    if (snake_game_menu(true)==1)
                        return 1;
                break;
            default:
                if (rb->default_event_handler (button) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
    }
}

enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;

    configfile_load(CONFIG_FILE_NAME,config,2,0);
    rb->lcd_clear_display();
    snake_game_loop();
    configfile_save(CONFIG_FILE_NAME,config,2,0);
    return PLUGIN_OK;
}
#endif
