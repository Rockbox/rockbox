/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 Daniel Stenberg
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

#define PAD_HEIGHT LCD_HEIGHT / 6    /* Recorder: 10   iRiver: 21 */
#define PAD_WIDTH LCD_WIDTH / 50     /* Recorder: 2    iRiver: 2  */

#define BALL_HEIGHT LCD_HEIGHT / 32  /* Recorder: 2    iRiver: 4  */
#define BALL_WIDTH LCD_HEIGHT / 32   /* We want a square ball */

#define SPEEDX ( LCD_WIDTH * 3 ) / 2 /* Recorder: 168  iRiver: 240 */
#define SPEEDY LCD_HEIGHT * 2        /* Recorder: 128  iRiver: 256 */
/* This is the width of the dead spot where the
 * cpu player doesnt care about the ball -- 3/8 of the screen */
#define CPU_PLAYER_RIGHT_DIST   ( (LCD_WIDTH/8 ) * 5 )
#define CPU_PLAYER_LEFT_DIST    ( (LCD_WIDTH/8 ) * 3 )

#define RES 100

#define MOVE_STEP LCD_HEIGHT / 32 /* move pad this many steps up/down each move */

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define PONG_QUIT BUTTON_OFF
#define PONG_PAUSE BUTTON_ON
#define PONG_LEFT_UP BUTTON_F1
#define PONG_LEFT_DOWN BUTTON_LEFT
#define PONG_RIGHT_UP BUTTON_F3
#define PONG_RIGHT_DOWN BUTTON_RIGHT

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define PONG_QUIT BUTTON_OFF
#define PONG_PAUSE BUTTON_ON
#define PONG_LEFT_UP BUTTON_F1
#define PONG_LEFT_DOWN BUTTON_LEFT
#define PONG_RIGHT_UP BUTTON_F3
#define PONG_RIGHT_DOWN BUTTON_RIGHT

#elif CONFIG_KEYPAD == ONDIO_PAD
#define PONG_QUIT BUTTON_OFF
#define PONG_PAUSE BUTTON_RIGHT
#define PONG_LEFT_UP BUTTON_LEFT
#define PONG_LEFT_DOWN BUTTON_MENU
#define PONG_RIGHT_UP BUTTON_UP
#define PONG_RIGHT_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == IRIVER_H100_PAD
#define PONG_QUIT BUTTON_OFF
#define PONG_LEFT_UP BUTTON_UP
#define PONG_LEFT_DOWN BUTTON_DOWN
#define PONG_RIGHT_UP BUTTON_ON
#define PONG_RIGHT_DOWN BUTTON_MODE
#define PONG_RC_QUIT BUTTON_RC_STOP

#elif CONFIG_KEYPAD == IRIVER_H300_PAD
#define PONG_QUIT BUTTON_OFF
#define PONG_LEFT_UP BUTTON_UP
#define PONG_LEFT_DOWN BUTTON_DOWN
#define PONG_RIGHT_UP BUTTON_REC
#define PONG_RIGHT_DOWN BUTTON_MODE
#define PONG_RC_QUIT BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define PONG_QUIT BUTTON_SELECT
#define PONG_LEFT_UP BUTTON_MENU
#define PONG_LEFT_DOWN BUTTON_LEFT
#define PONG_RIGHT_UP BUTTON_RIGHT
#define PONG_RIGHT_DOWN BUTTON_PLAY

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define PONG_QUIT BUTTON_POWER
#define PONG_LEFT_UP BUTTON_UP
#define PONG_LEFT_DOWN BUTTON_DOWN
#define PONG_RIGHT_UP BUTTON_REC
#define PONG_RIGHT_DOWN BUTTON_PLAY

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define PONG_QUIT BUTTON_POWER
#define PONG_PAUSE BUTTON_SELECT
#define PONG_LEFT_UP BUTTON_UP
#define PONG_LEFT_DOWN BUTTON_DOWN
#define PONG_RIGHT_UP BUTTON_VOL_UP
#define PONG_RIGHT_DOWN BUTTON_VOL_DOWN

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
      (CONFIG_KEYPAD == SANSA_CLIP_PAD) || \
      (CONFIG_KEYPAD == SANSA_M200_PAD) || \
      (CONFIG_KEYPAD == SANSA_CONNECT_PAD)
#define PONG_QUIT BUTTON_POWER
#define PONG_PAUSE BUTTON_SELECT
#define PONG_LEFT_UP BUTTON_LEFT
#define PONG_LEFT_DOWN BUTTON_DOWN
#define PONG_RIGHT_UP BUTTON_UP
#define PONG_RIGHT_DOWN BUTTON_RIGHT

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#define PONG_QUIT BUTTON_HOME
#define PONG_PAUSE BUTTON_SELECT
#define PONG_LEFT_UP BUTTON_LEFT
#define PONG_LEFT_DOWN BUTTON_DOWN
#define PONG_RIGHT_UP BUTTON_UP
#define PONG_RIGHT_DOWN BUTTON_RIGHT

#elif (CONFIG_KEYPAD == SANSA_C200_PAD)
#define PONG_QUIT BUTTON_POWER
#define PONG_PAUSE BUTTON_SELECT
#define PONG_LEFT_UP BUTTON_VOL_UP
#define PONG_LEFT_DOWN BUTTON_VOL_DOWN
#define PONG_RIGHT_UP BUTTON_UP
#define PONG_RIGHT_DOWN BUTTON_DOWN

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define PONG_QUIT BUTTON_POWER
#define PONG_LEFT_UP BUTTON_SCROLL_UP
#define PONG_LEFT_DOWN BUTTON_SCROLL_DOWN
#define PONG_RIGHT_UP BUTTON_REW
#define PONG_RIGHT_DOWN BUTTON_FF

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#define PONG_QUIT BUTTON_BACK
#define PONG_LEFT_UP BUTTON_UP
#define PONG_LEFT_DOWN BUTTON_DOWN
#define PONG_RIGHT_UP BUTTON_VOL_UP
#define PONG_RIGHT_DOWN BUTTON_VOL_DOWN

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#define PONG_QUIT BUTTON_POWER
#define PONG_PAUSE BUTTON_SELECT
#define PONG_LEFT_UP BUTTON_MENU
#define PONG_LEFT_DOWN BUTTON_LEFT
#define PONG_RIGHT_UP BUTTON_PLAY
#define PONG_RIGHT_DOWN BUTTON_RIGHT

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define PONG_QUIT BUTTON_RC_REC
#define PONG_PAUSE BUTTON_RC_PLAY
#define PONG_LEFT_UP BUTTON_RC_VOL_UP
#define PONG_LEFT_DOWN BUTTON_RC_VOL_DOWN
#define PONG_RIGHT_UP BUTTON_VOL_UP
#define PONG_RIGHT_DOWN BUTTON_VOL_DOWN

#elif (CONFIG_KEYPAD == COWON_D2_PAD)
#define PONG_QUIT BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define PONG_QUIT BUTTON_POWER
#define PONG_PAUSE BUTTON_MENU
#define PONG_LEFT_UP BUTTON_VOLUP
#define PONG_LEFT_DOWN BUTTON_VOLDOWN
#define PONG_RIGHT_UP BUTTON_RIGHT
#define PONG_RIGHT_DOWN BUTTON_LEFT

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define PONG_QUIT BUTTON_BACK
#define PONG_LEFT_UP BUTTON_UP
#define PONG_LEFT_DOWN BUTTON_DOWN
#define PONG_RIGHT_UP BUTTON_PLAY
#define PONG_RIGHT_DOWN BUTTON_MENU

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define PONG_QUIT BUTTON_POWER
#define PONG_PAUSE BUTTON_MENU
#define PONG_LEFT_UP BUTTON_UP
#define PONG_LEFT_DOWN BUTTON_DOWN
#define PONG_RIGHT_UP BUTTON_VOL_UP
#define PONG_RIGHT_DOWN BUTTON_VOL_DOWN

#elif CONFIG_KEYPAD == PHILIPS_HDD6330_PAD
#define PONG_QUIT BUTTON_POWER
#define PONG_PAUSE BUTTON_MENU
#define PONG_LEFT_UP BUTTON_UP
#define PONG_LEFT_DOWN BUTTON_DOWN
#define PONG_RIGHT_UP BUTTON_VOL_UP
#define PONG_RIGHT_DOWN BUTTON_VOL_DOWN

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define PONG_QUIT BUTTON_POWER
#define PONG_PAUSE BUTTON_MENU
#define PONG_LEFT_UP BUTTON_UP
#define PONG_LEFT_DOWN BUTTON_DOWN
#define PONG_RIGHT_UP BUTTON_VOL_UP
#define PONG_RIGHT_DOWN BUTTON_VOL_DOWN

#elif CONFIG_KEYPAD == ONDAVX747_PAD || \
CONFIG_KEYPAD == ONDAVX777_PAD || \
CONFIG_KEYPAD == MROBE500_PAD
#define PONG_QUIT BUTTON_POWER

#elif CONFIG_KEYPAD == SAMSUNG_YH_PAD
#define PONG_QUIT        BUTTON_REC
#define PONG_PAUSE       BUTTON_PLAY
#define PONG_LEFT_UP     BUTTON_UP
#define PONG_LEFT_DOWN   BUTTON_DOWN
#define PONG_RIGHT_UP    BUTTON_FFWD
#define PONG_RIGHT_DOWN  BUTTON_REW

#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD
#define PONG_QUIT        BUTTON_REC
#define PONG_PAUSE       BUTTON_OK
#define PONG_LEFT_UP     BUTTON_MENU
#define PONG_LEFT_DOWN   BUTTON_PREV
#define PONG_RIGHT_UP    BUTTON_PLAY
#define PONG_RIGHT_DOWN  BUTTON_NEXT

#elif CONFIG_KEYPAD == MPIO_HD200_PAD
#define PONG_QUIT (BUTTON_REC|BUTTON_PLAY)
#define PONG_LEFT_UP BUTTON_REW
#define PONG_LEFT_DOWN BUTTON_FF
#define PONG_RIGHT_UP BUTTON_VOL_UP
#define PONG_RIGHT_DOWN BUTTON_VOL_DOWN

#elif CONFIG_KEYPAD == MPIO_HD300_PAD
#define PONG_QUIT (BUTTON_MENU|BUTTON_REPEAT)
#define PONG_LEFT_UP BUTTON_REW
#define PONG_LEFT_DOWN BUTTON_REC
#define PONG_RIGHT_UP BUTTON_FF
#define PONG_RIGHT_DOWN BUTTON_PLAY

#elif CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD
#define PONG_QUIT         BUTTON_POWER
#define PONG_LEFT_UP      BUTTON_BACK
#define PONG_LEFT_DOWN    BUTTON_BOTTOMLEFT
#define PONG_RIGHT_UP     BUTTON_PLAYPAUSE
#define PONG_RIGHT_DOWN   BUTTON_BOTTOMRIGHT
#define PONG_PAUSE        BUTTON_SELECT

#elif (CONFIG_KEYPAD == SAMSUNG_YPR0_PAD)
#define PONG_QUIT BUTTON_BACK
#define PONG_PAUSE BUTTON_SELECT
#define PONG_LEFT_UP BUTTON_UP
#define PONG_LEFT_DOWN BUTTON_DOWN
#define PONG_RIGHT_UP BUTTON_MENU
#define PONG_RIGHT_DOWN BUTTON_POWER

#elif (CONFIG_KEYPAD == HM60X_PAD) || \
    (CONFIG_KEYPAD == HM801_PAD)
#define PONG_QUIT BUTTON_POWER
#define PONG_PAUSE BUTTON_SELECT
#define PONG_LEFT_UP BUTTON_UP
#define PONG_LEFT_DOWN BUTTON_DOWN
#define PONG_RIGHT_UP BUTTON_RIGHT
#define PONG_RIGHT_DOWN BUTTON_LEFT

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef PONG_QUIT
#define PONG_QUIT       BUTTON_TOPMIDDLE
#endif
#ifndef PONG_LEFT_UP
#define PONG_LEFT_UP    BUTTON_TOPLEFT
#endif
#ifndef PONG_LEFT_DOWN
#define PONG_LEFT_DOWN  BUTTON_BOTTOMLEFT
#endif
#ifndef PONG_RIGHT_UP
#define PONG_RIGHT_UP   BUTTON_TOPRIGHT
#endif
#ifndef PONG_RIGHT_DOWN
#define PONG_RIGHT_DOWN BUTTON_BOTTOMRIGHT
#endif
#ifndef PONG_PAUSE
#define PONG_PAUSE      BUTTON_CENTER
#endif
#endif

struct player {
    int xpos;  /* X position of pad */
    int w_pad; /* wanted current Y position of pad */
    int e_pad; /* existing current Y position of pad */
    int score;
    bool iscpu; /* Status of AI player */
};

struct ball {
    int x; /* current X*RES position of the ball */
    int y; /* current Y*RES position of the ball */
    int speedx; /*  */
    int speedy; /*  */
};

struct pong {
    struct ball ball;
    struct player player[2];
};

static void singlepad(int x, int y, int set)
{
    if(set) {
        rb->lcd_fillrect(x, y, PAD_WIDTH, PAD_HEIGHT);
    }
    else {
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_fillrect(x, y, PAD_WIDTH, PAD_HEIGHT);
        rb->lcd_set_drawmode(DRMODE_SOLID);
    }
}

static void pad(struct pong *p, int pad)
{
    struct player *player = &p->player[pad];
    /* clear existing pad */
    singlepad(player->xpos, player->e_pad, 0);

    /* draw wanted pad */
    singlepad(player->xpos, player->w_pad, 1);

    /* existing is now the wanted */
    player->e_pad = player->w_pad;
}

static bool wallcollide(struct pong *p, int pad)
{
    /* we have already checked for pad-collision, just check if this hits
       the wall */
    if(pad) {
        /* right-side */
        if(p->ball.x > ( LCD_WIDTH*RES ) - PAD_WIDTH )
            return true;
    }
    else {
        if(p->ball.x + ( BALL_WIDTH*RES ) < PAD_WIDTH )
            return true;
    }
    return false;
}

/* returns true if the ball has hit a pad, and then the info variable
   will have extra angle info */

static bool padcollide(struct pong *p, int pad, int *info)
{
    struct player *player = &p->player[pad];
    int x = p->ball.x/RES;
    int y = p->ball.y/RES;

    if((y < (player->e_pad+PAD_HEIGHT)) &&
       (y + BALL_HEIGHT > player->e_pad)) {
        /* Y seems likely right */

        /* store the delta between ball-middle MINUS pad-middle, so
           it returns:
           0 when the ball hits exactly the middle of the pad
           positive numbers when the ball is below the middle of the pad
           negative numbers when the ball is above the middle of the pad

           max number is +- PAD_HEIGHT/2
        */

        *info = (y+BALL_HEIGHT/2) - (player->e_pad + PAD_HEIGHT/2);

        if(pad) {
            /* right-side */
            if((x + BALL_WIDTH) >= (LCD_WIDTH - PAD_WIDTH))
                return true; /* phump */
        }
        else {
            if(x <= PAD_WIDTH)
                return true;
        }
    }
    return false; /* nah */
}

static void bounce(struct pong *p, int pad, int info)
{
    p->ball.speedx = -p->ball.speedx;

    /* Give ball a little push to keep it from getting stuck between wall and pad */
    if(pad) {
        /* right side */
        p->ball.x -= PAD_WIDTH*RES/4;
    }
    else {
        p->ball.x += PAD_WIDTH*RES/4;
    }

    /* info is the hit-angle into the pad */
    if(p->ball.speedy > 0) {
        /* downwards */
        if(info > 0) {
            /* below the middle of the pad */
            p->ball.speedy += info * RES/3;
        }
        else if(info < 0) {
            /* above the middle */
            p->ball.speedy = info * RES/2;
        }
    }
    else {
        /* upwards */
        if(info > 0) {
            /* below the middle of the pad */
            p->ball.speedy = info * RES/2;
        }
        else if(info < 0) {
            /* above the middle */
            p->ball.speedy += info * RES/3;
        }
    }

    p->ball.speedy += rb->rand()%21-10;

#if 0
    fprintf(stderr, "INFO: %d YSPEED: %d\n", info, p->ball.speedy);
#endif
}

static void score(struct pong *p, int pad)
{
    if(pad)
        rb->splash(HZ/4, "right scores!");
    else
        rb->splash(HZ/4, "left scores!");
    rb->lcd_clear_display();
    p->player[pad].score++;

    /* then move the X-speed of the ball and give it a random Y position */
    p->ball.speedx = -p->ball.speedx;
    p->ball.y = rb->rand()%((LCD_HEIGHT-BALL_HEIGHT)*RES);

    /* avoid hitting the pad with the new ball */
    p->ball.x = (p->ball.x < 0) ?
        (RES * PAD_WIDTH) : (RES * (LCD_WIDTH - PAD_WIDTH - BALL_WIDTH));

    /* restore Y-speed to default */
    p->ball.speedy = (p->ball.speedy > 0) ? SPEEDY : -SPEEDY;

    /* set the existing pad positions to something weird to force pad
       updates */
    p->player[0].e_pad = -1;
    p->player[1].e_pad = -1;
}

static void ball(struct pong *p)
{
    int oldx = p->ball.x/RES;
    int oldy = p->ball.y/RES;

    int newx;
    int newy;

    int info;

    /* movement */
    p->ball.x += p->ball.speedx;
    p->ball.y += p->ball.speedy;

    newx = p->ball.x/RES;
    newy = p->ball.y/RES;

    /* detect if ball hits a wall */
    if(newy + BALL_HEIGHT > LCD_HEIGHT) {
        /* hit floor, bounce */
        p->ball.speedy = -p->ball.speedy;
        p->ball.y = (LCD_HEIGHT - BALL_HEIGHT) * RES;
    }
    else if(newy < 0) {
        /* hit ceiling, bounce */
        p->ball.speedy = -p->ball.speedy;
        p->ball.y = 0;
    }

    /* detect if ball hit pads */
    if(padcollide(p, 0, &info))
        bounce(p, 0, info);
    else if(padcollide(p, 1, &info))
        bounce(p, 1, info);
    else if(wallcollide(p, 0))
        score(p, 1);
    else if(wallcollide(p, 1))
        score(p, 0);

    newx = p->ball.x/RES;
    newy = p->ball.y/RES;

    /* clear old position */
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(oldx, oldy, BALL_WIDTH, BALL_HEIGHT);
    rb->lcd_set_drawmode(DRMODE_SOLID);

    /* draw the new ball position */
    rb->lcd_fillrect(newx, newy, BALL_WIDTH, BALL_HEIGHT);
}

static void padmove(int *pos, int dir)
{
    *pos += dir;
    if(*pos > (LCD_HEIGHT-PAD_HEIGHT))
        *pos = (LCD_HEIGHT-PAD_HEIGHT);
    else if(*pos < 0)
        *pos = 0;
}

static void key_pad(struct pong *p, int pad, int up, int down)
{
    struct player *player = &p->player[pad];
    if(player->iscpu) {
        if((pad && (p->ball.x/RES > CPU_PLAYER_RIGHT_DIST)) /* cpu right */
            || (!pad && (p->ball.x/RES < CPU_PLAYER_LEFT_DIST)) /* cpu left */)
        {
            if(p->ball.y/RES > player->w_pad) /* player goes down */
                padmove(&player->w_pad, MOVE_STEP);

            if(p->ball.y/RES < player->w_pad) /* player goes up */
                padmove(&player->w_pad, -MOVE_STEP);
        }

        if(down || up) {
            /* if player presses control keys stop cpu player */
            player->iscpu = false;
            p->player[0].score = p->player[1].score = 0; /* reset the score */
            rb->lcd_clear_display(); /* get rid of the text */
        }
    }
    else {
        if(down) /* player goes down */
            padmove(&player->w_pad, MOVE_STEP);

        if(up)   /* player goes up */
            padmove(&player->w_pad, -MOVE_STEP);
    }
}

static int keys(struct pong *p)
{
    int key;
#ifdef PONG_PAUSE
    static bool pause = false;
#endif

    /* number of ticks this function will loop reading keys */
#ifndef HAVE_TOUCHSCREEN
    int time = 4;
#else
    int time = 1;
#endif
    int start = *rb->current_tick;
    int end = start + time;

    while(TIME_BEFORE(*rb->current_tick, end)) {
        key = rb->button_get_w_tmo(end - *rb->current_tick);

#ifdef HAVE_TOUCHSCREEN
        short touch_x, touch_y;
        if(key & BUTTON_TOUCHSCREEN)
        {
            struct player *player;
            touch_x = rb->button_get_data() >> 16;
            touch_y = rb->button_get_data() & 0xFFFF;

            player = &p->player[0];
            if(touch_x >= player->xpos && touch_x <= player->xpos+(PAD_WIDTH*4))
            {
                padmove(&player->w_pad, touch_y-(player->e_pad*2+PAD_HEIGHT)/2);
                if (player->iscpu) {
                    /* if left player presses control keys stop cpu player */
                    player->iscpu = false;
                    p->player[0].score = p->player[1].score = 0; /* reset the score */
                    rb->lcd_clear_display(); /* get rid of the text */
                }
            }

            player = &p->player[1];
            if(touch_x >= player->xpos-(PAD_WIDTH*4) && touch_x <= player->xpos)
            {
                padmove(&player->w_pad, touch_y-(player->e_pad*2+PAD_HEIGHT)/2);
                if (player->iscpu) {
                    /* if right player presses control keys stop cpu player */
                    player->iscpu = false;
                    p->player[0].score = p->player[1].score = 0; /* reset the score */
                    rb->lcd_clear_display(); /* get rid of the text */
                }
            }
        }
#endif

#ifdef HAS_BUTTON_HOLD
        if (rb->button_hold())
            return 2; /* Pause game */
#endif

        if(key & PONG_QUIT
#ifdef PONG_RC_QUIT
           || key & PONG_RC_QUIT
#endif
        )
            return 0; /* exit game NOW */

#ifdef PONG_PAUSE
        if(key == PONG_PAUSE)
            pause = !pause;
        if(pause)
            return 2; /* Pause game */
#endif

        key = rb->button_status(); /* ignore BUTTON_REPEAT */

        key_pad(p, 0, (key & PONG_LEFT_UP), (key & PONG_LEFT_DOWN));
        key_pad(p, 1, (key & PONG_RIGHT_UP), (key & PONG_RIGHT_DOWN));

        if(rb->default_event_handler(key) == SYS_USB_CONNECTED)
            return -1; /* exit game because of USB */
    }
    return 1; /* return 0 to exit game */
}

static void showscore(struct pong *p)
{
    static char buffer[20];
    int w;

    rb->snprintf(buffer, sizeof(buffer), "%d - %d",
        p->player[0].score, p->player[1].score);
    w = rb->lcd_getstringsize((unsigned char *)buffer, NULL, NULL);
    rb->lcd_putsxy( (LCD_WIDTH / 2) - (w / 2), 0, (unsigned char *)buffer);
}

static void blink_demo(void)
{
    static char buffer[30];
    int w;

    rb->snprintf(buffer, sizeof(buffer), "Press Key To Play");
    w = rb->lcd_getstringsize((unsigned char *)buffer, NULL, NULL);
    if(LCD_WIDTH > ( (w/8)*7 ) ) /* make sure text isn't too long for screen */
        rb->lcd_putsxy( (LCD_WIDTH / 2) - (w / 2), (LCD_HEIGHT / 2),
                        (unsigned char *)buffer);
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    struct pong pong;
    int game = 1;

    int blink_timer = 0;
    int blink_rate = 20;
    bool blink = true;

    /* init the struct with some silly values to start with */

    pong.ball.x = 20*RES;
    pong.ball.y = 20*RES;
    pong.ball.speedx = SPEEDX;
    pong.ball.speedy = SPEEDY;

    pong.player[0].xpos = 0;
    pong.player[0].e_pad = 0;
    pong.player[0].w_pad = 7;
    pong.player[1].xpos = LCD_WIDTH-PAD_WIDTH;
    pong.player[1].e_pad = 0;
    pong.player[1].w_pad = 40;

    /* start every game in demo mode */
    pong.player[0].iscpu = pong.player[1].iscpu = true;

    pong.player[0].score = pong.player[1].score = 0; /* lets start at 0 - 0 ;-) */

    /* if you don't use the parameter, you can do like
       this to avoid the compiler warning about it */
    (void)parameter;

    /* Turn off backlight timeout */
    backlight_ignore_timeout();
    /* Clear screen */
    rb->lcd_clear_display();

    /* go go go */
    while(game > 0) {
        if (game == 2) { /* Game Paused */
            rb->splash(0, "PAUSED");
            while(game == 2)
                game = keys(&pong); /* short circuit */
            rb->lcd_clear_display();
        }

        if( pong.player[0].iscpu && pong.player[1].iscpu ) {
            if(blink_timer<blink_rate) {
                ++blink_timer;
            }
            else {
                blink_timer=0;
                blink = !blink;
            }

            if(blink==true) {
                blink_demo();
            }
            else {
                rb->lcd_clear_display();
            }
        }

        showscore(&pong);
        pad(&pong, 0); /* draw left pad */
        pad(&pong, 1); /* draw right pad */
        ball(&pong); /* move and draw ball */

        rb->lcd_update();

        game = keys(&pong); /* deal with keys */
    }

    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings();
    return (game == 0) ? PLUGIN_OK : PLUGIN_USB_CONNECTED;
}
