/*****************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _// __ \_/ ___\|  |/ /| __ \ / __ \  \/  /
 *   Jukebox    |    |   ( (__) )  \___|    ( | \_\ ( (__) )    (
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 - 2008 Alexander Papst
 * Idea from http://www.tetris1d.org
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

PLUGIN_HEADER

#ifdef HAVE_LCD_BITMAP

#if CONFIG_KEYPAD == RECORDER_PAD
#define ONEDROCKBLOX_DOWN              BUTTON_PLAY
#define ONEDROCKBLOX_QUIT              BUTTON_OFF

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define ONEDROCKBLOX_DOWN              BUTTON_SELECT
#define ONEDROCKBLOX_QUIT              BUTTON_OFF

#elif CONFIG_KEYPAD == ONDIO_PAD
#define ONEDROCKBLOX_DOWN              BUTTON_RIGHT
#define ONEDROCKBLOX_QUIT              BUTTON_OFF

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define ONEDROCKBLOX_DOWN              BUTTON_SELECT
#define ONEDROCKBLOX_QUIT              BUTTON_POWER

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define ONEDROCKBLOX_DOWN              BUTTON_SELECT
#define ONEDROCKBLOX_QUIT              (BUTTON_SELECT | BUTTON_MENU)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define ONEDROCKBLOX_DOWN              BUTTON_SELECT
#define ONEDROCKBLOX_QUIT              BUTTON_OFF

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define ONEDROCKBLOX_DOWN              BUTTON_SELECT
#define ONEDROCKBLOX_QUIT              BUTTON_POWER

#elif CONFIG_KEYPAD == SANSA_E200_PAD || CONFIG_KEYPAD == SANSA_C200_PAD || \
CONFIG_KEYPAD == SANSA_CLIP_PAD
#define ONEDROCKBLOX_DOWN              BUTTON_SELECT
#define ONEDROCKBLOX_QUIT              BUTTON_POWER

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define ONEDROCKBLOX_DOWN              BUTTON_PLAY
#define ONEDROCKBLOX_QUIT              BUTTON_POWER

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#define ONEDROCKBLOX_DOWN              BUTTON_SELECT
#define ONEDROCKBLOX_QUIT              BUTTON_BACK

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#define ONEDROCKBLOX_DOWN              BUTTON_SELECT
#define ONEDROCKBLOX_QUIT              BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define ONEDROCKBLOX_DOWN              BUTTON_RC_PLAY
#define ONEDROCKBLOX_QUIT              BUTTON_RC_REC

#elif (CONFIG_KEYPAD == COWOND2_PAD)
#define ONEDROCKBLOX_DOWN              BUTTON_MENU
#define ONEDROCKBLOX_QUIT              BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define ONEDROCKBLOX_DOWN              BUTTON_MENU
#define ONEDROCKBLOX_QUIT              BUTTON_POWER

#else
#error No keymap defined!
#endif

#define mrand(max) (short)(rb->rand()%max)

#define TILES 11

/**********
** Lots of defines for the drawing stuff :-)
** The most ugly way i could think of...
*/

#if (LCD_WIDTH > LCD_HEIGHT)
   /* Any screens larger than the minis LCD */
#if (LCD_WIDTH > 132)
     /* Max size of one block */
#    define WIDTH (int)((LCD_HEIGHT * 0.85) / TILES)
     /* Align the playing filed centered */
#    define CENTER_X (int)(LCD_WIDTH/2-(WIDTH/2))
#    define CENTER_Y (int)(LCD_HEIGHT/2-((WIDTH*TILES+TILES)/2))
     /* Score box */
#    define SCORE_X (int)(((CENTER_X+WIDTH+4) + (LCD_WIDTH-(CENTER_X+WIDTH+4))/2)-(f_width/2))
#    define SCORE_Y (int)((LCD_HEIGHT/2)-(f_height/2))
     /* Max. size of bricks is 4 blocks */
#    define NEXT_H (WIDTH*4+3)
#    define NEXT_X (int)(CENTER_X/2-WIDTH/2)
#    define NEXT_Y (int)(LCD_HEIGHT/2-NEXT_H/2)
#else
     /* Max size of one block */
#    define WIDTH (int)((LCD_HEIGHT * 0.85) / TILES)
     /* Align the playing left centered */
#    define CENTER_X (int)(LCD_WIDTH*0.2)
#    define CENTER_Y (int)(LCD_HEIGHT/2-((WIDTH*TILES+TILES)/2))
     /* Score box */
#    define SCORE_X (int)(((CENTER_X+WIDTH+4) + (LCD_WIDTH-(CENTER_X+WIDTH+4))/2)-(f_width/2))
#    define SCORE_Y 16
     /* Max. size of bricks is 4 blocks */
#    define NEXT_H (WIDTH*4+3)
#    define NEXT_X (score_x+f_width+7)
#    define NEXT_Y (int)(LCD_HEIGHT-((4*WIDTH)+13))
#endif
#else
   /* Max size of one block */
#  define WIDTH (int)((LCD_HEIGHT * 0.8) / TILES)
   /* Align the playing filed centered */
#  define CENTER_X (int)(LCD_WIDTH/2-(WIDTH/2))
#  define CENTER_Y 2
   /* Score box */
#  define SCORE_X (int)((LCD_WIDTH/2)-(f_width/2))
#  define SCORE_Y (LCD_HEIGHT-(f_height+2))
   /* Max. size of bricks is 4 blocks */
#  define NEXT_H (WIDTH*4+3)
#  define NEXT_X (int)(CENTER_X/2-WIDTH/2)
#  define NEXT_Y (int)((LCD_HEIGHT * 0.8)/2-NEXT_H/2)
#endif

static const struct plugin_api* rb; /* global api struct pointer */

void draw_brick(int pos, int length) {
    int i = pos;
    rb->lcd_set_drawmode(DRMODE_BG|DRMODE_INVERSEVID);
    rb->lcd_fillrect(CENTER_X, CENTER_Y, WIDTH, WIDTH * TILES + TILES);
    rb->lcd_set_drawmode(DRMODE_SOLID);

    for (i = pos; i < length + pos; ++i) {
        if (i >= 0) rb->lcd_fillrect(CENTER_X, CENTER_Y+i+(WIDTH*i), WIDTH, WIDTH);
    }
}

enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    int i;
    int f_width, f_height;
    int score_x;

    bool quit = false;
    int button;
    
    int cycletime = 300;
    int end;

    int pos_cur_brick = 0;
    int type_cur_brick = 0;
    int type_next_brick = 0;
    
    unsigned long int score = 34126;
    char score_buf[10];
    
    rb = api;
    (void)parameter;

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_foreground(LCD_WHITE);
#endif

    rb->lcd_setfont(FONT_SYSFIXED);
    
    rb->lcd_getstringsize("100000000", &f_width, &f_height);
    
    rb->lcd_clear_display();
    
    /***********
    ** Draw EVERYTHING
    */
    
    /* Playing filed box */
    rb->lcd_vline(CENTER_X-2, CENTER_Y, CENTER_Y + (WIDTH*TILES+TILES));
    rb->lcd_vline(CENTER_X + WIDTH + 1, CENTER_Y,
                  CENTER_Y + (WIDTH*TILES+TILES));
    rb->lcd_hline(CENTER_X-2, CENTER_X + WIDTH + 1, 
                  CENTER_Y + (WIDTH*TILES+TILES));

    /* Score box */
#if (LCD_WIDTH > LCD_HEIGHT)
    rb->lcd_drawrect(SCORE_X-4, SCORE_Y-5, f_width+8, f_height+9);
    rb->lcd_putsxy(SCORE_X-4, SCORE_Y-6-f_height, "score");
#else
    rb->lcd_hline(0, LCD_WIDTH, SCORE_Y-5);
    rb->lcd_putsxy(2, SCORE_Y-6-f_height, "score");
#endif
    score_x = SCORE_X;
    
    /* Next box */
    rb->lcd_getstringsize("next", &f_width, NULL);
#if (LCD_WIDTH > LCD_HEIGHT) && !(LCD_WIDTH > 132)
    rb->lcd_drawrect(NEXT_X-5, NEXT_Y-5, WIDTH+10, NEXT_H+10);
    rb->lcd_putsxy(score_x-4, NEXT_Y-5, "next");
#else
    rb->lcd_drawrect(NEXT_X-5, NEXT_Y-5, WIDTH+10, NEXT_H+10);
    rb->lcd_putsxy(NEXT_X-5, NEXT_Y-5-f_height-1, "next");
#endif

    /***********
    ** GAMELOOP
    */
    rb->srand( *rb->current_tick );
    
    type_cur_brick = 2 + mrand(3);
    type_next_brick = 2 + mrand(3);
    
    do {
        end = *rb->current_tick + (cycletime * HZ) / 1000;
        
        draw_brick(pos_cur_brick, type_cur_brick);

        /* Draw next brick */
        rb->lcd_set_drawmode(DRMODE_BG|DRMODE_INVERSEVID);
        rb->lcd_fillrect(NEXT_X, NEXT_Y, WIDTH, WIDTH * 4 + 4);
        rb->lcd_set_drawmode(DRMODE_SOLID);

        for (i = 0; i < type_next_brick; ++i) {
            rb->lcd_fillrect(NEXT_X, 
                             NEXT_Y + ((type_next_brick % 2) ? (int)(WIDTH/2) : ((type_next_brick == 2) ? (WIDTH+1) : 0)) + (WIDTH*i) + i,
                             WIDTH, WIDTH);
        } 

        /* Score box */
        rb->snprintf(score_buf, sizeof(score_buf), "%8ld0", score);
        rb->lcd_putsxy(score_x, SCORE_Y, score_buf);

        rb->lcd_update();

        button = rb->button_status();

        switch(button) {
            case ONEDROCKBLOX_DOWN:
            case (ONEDROCKBLOX_DOWN|BUTTON_REPEAT):
                cycletime = 100;
                break;
            case ONEDROCKBLOX_QUIT:
                quit = true;
                break;
            default:
                cycletime = 300;
                if(rb->default_event_handler(button) == SYS_USB_CONNECTED) {
                    quit = true;
                }
        }
        
        if ((pos_cur_brick + type_cur_brick) > 10) {
             type_cur_brick = type_next_brick;
             type_next_brick = 2 + mrand(3);
             score += (type_cur_brick - 1) * 2;
             pos_cur_brick = 1 - type_cur_brick;
        } else {
            ++pos_cur_brick;
        }

        if (end > *rb->current_tick)
            rb->sleep(end-*rb->current_tick);
        else
            rb->yield();

    } while (!quit);
 
    return PLUGIN_OK;
}
#endif
