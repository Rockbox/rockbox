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

#ifdef HAVE_LCD_BITMAP

PLUGIN_HEADER

#define PAD_HEIGHT LCD_HEIGHT / 6    /* Recorder: 10   iRiver: 21 */
#define PAD_WIDTH LCD_WIDTH / 50     /* Recorder: 2    iRiver: 2  */

#define BALL_HEIGHT LCD_HEIGHT / 32  /* Recorder: 2    iRiver: 4  */
#define BALL_WIDTH LCD_HEIGHT / 32   /* We want a square ball */

#define SPEEDX ( LCD_WIDTH * 3 ) / 2 /* Recorder: 168  iRiver: 240 */
#define SPEEDY LCD_HEIGHT * 2        /* Recorder: 128  iRiver: 256 */

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

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define PONG_QUIT BUTTON_POWER
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

#elif (CONFIG_KEYPAD == COWOND2_PAD)
#define PONG_QUIT BUTTON_POWER

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

static const struct plugin_api* rb;

struct pong {
    int ballx; /* current X*RES position of the ball */
    int bally; /* current Y*RES position of the ball */
    int w_pad[2]; /* wanted current Y positions of pads */
    int e_pad[2]; /* existing current Y positions of pads */
    int ballspeedx; /*  */
    int ballspeedy; /*  */

    int score[2];
};

void singlepad(int x, int y, int set)
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

static int xpos[2]={0, LCD_WIDTH-PAD_WIDTH};
void pad(struct pong *p, int pad)
{
    /* clear existing pad */
    singlepad(xpos[pad], p->e_pad[pad], 0);

    /* draw wanted pad */
    singlepad(xpos[pad], p->w_pad[pad], 1);

    /* existing is now the wanted */
    p->e_pad[pad] = p->w_pad[pad];
}

bool wallcollide(struct pong *p, int pad)
{
    /* we have already checked for pad-collision, just check if this hits
       the wall */
    if(pad) {
        /* right-side */
        if(p->ballx > LCD_WIDTH*RES)
            return true;
    }
    else {
        if(p->ballx < 0)
            return true;
    }
    return false;
}

/* returns true if the ball has hit a pad, and then the info variable
   will have extra angle info */

bool padcollide(struct pong *p, int pad, int *info)
{
    int x = p->ballx/RES;
    int y = p->bally/RES;

    if((y < (p->e_pad[pad]+PAD_HEIGHT)) &&
       (y + BALL_HEIGHT > p->e_pad[pad])) {
        /* Y seems likely right */

        /* store the delta between ball-middle MINUS pad-middle, so
           it returns:
           0 when the ball hits exactly the middle of the pad
           positive numbers when the ball is below the middle of the pad
           negative numbers when the ball is above the middle of the pad

           max number is +- PAD_HEIGHT/2
        */

        *info = (y+BALL_HEIGHT/2) - (p->e_pad[pad] + PAD_HEIGHT/2);

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

void bounce(struct pong *p, int pad, int info)
{
    (void)pad; /* not used right now */
    p->ballspeedx = -p->ballspeedx;

    /* info is the hit-angle into the pad */
    if(p->ballspeedy > 0) {
        /* downwards */
        if(info > 0) {
            /* below the middle of the pad */
            p->ballspeedy += info * RES/3;
        }
        else if(info < 0) {
            /* above the middle */
            p->ballspeedy = info * RES/2;
        }
    }
    else {
        /* upwards */
        if(info > 0) {
            /* below the middle of the pad */
            p->ballspeedy = info * RES/2;
        }
        else if(info < 0) {
            /* above the middle */
            p->ballspeedy -= info * RES/3;
        }
    }

    p->ballspeedy += rb->rand()%21-10;

#if 0
    fprintf(stderr, "INFO: %d YSPEED: %d\n", info, p->ballspeedy);
#endif
}

void score(struct pong *p, int pad)
{
    if(pad)
        rb->splash(HZ/4, "right scores!");
    else
        rb->splash(HZ/4, "left scores!");
    rb->lcd_clear_display();
    p->score[pad]++;

    /* then move the X-speed of the ball and give it a random Y position */
    p->ballspeedx = -p->ballspeedx;
    p->bally = rb->rand()%(LCD_HEIGHT*RES - BALL_HEIGHT);

    /* avoid hitting the pad with the new ball */
    p->ballx = (p->ballx < 0) ?
        (RES * PAD_WIDTH) : (RES * (LCD_WIDTH - PAD_WIDTH - BALL_WIDTH));
    
    /* restore Y-speed to default */
    p->ballspeedy = (p->ballspeedy > 0) ? SPEEDY : -SPEEDY;

    /* set the existing pad positions to something weird to force pad
       updates */
    p->e_pad[0] = -1;
    p->e_pad[1] = -1;
}

void ball(struct pong *p)
{
    int x = p->ballx/RES;
    int y = p->bally/RES;

    int newx;
    int newy;

    int info;

    /* movement */
    p->ballx += p->ballspeedx;
    p->bally += p->ballspeedy;

    newx = p->ballx/RES;
    newy = p->bally/RES;

    /* detect if ball hits a wall */
    if(newy + BALL_HEIGHT > LCD_HEIGHT) {
        /* hit floor, bounce */
        p->ballspeedy = -p->ballspeedy;
        newy = LCD_HEIGHT - BALL_HEIGHT;
        p->bally = newy * RES;
    }
    else if(newy < 0) {
        /* hit ceiling, bounce */
        p->ballspeedy = -p->ballspeedy;
        p->bally = 0;
        newy = 0;
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

        newx = p->ballx/RES;
        newy = p->bally/RES;

    /* clear old position */
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(x, y, BALL_WIDTH, BALL_HEIGHT);
    rb->lcd_set_drawmode(DRMODE_SOLID);

    /* draw the new ball position */
    rb->lcd_fillrect(newx, newy, BALL_WIDTH, BALL_HEIGHT);
}

void padmove(int *pos, int dir)
{
    *pos += dir;
    if(*pos > (LCD_HEIGHT-PAD_HEIGHT))
        *pos = (LCD_HEIGHT-PAD_HEIGHT);
    else if(*pos < 0)
        *pos = 0;
}

int keys(struct pong *p)
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

    while(end > *rb->current_tick) {
        key = rb->button_get_w_tmo(end - *rb->current_tick);
        
#ifdef HAVE_TOUCHSCREEN
    short touch_x, touch_y;
    if(key & BUTTON_TOUCHSCREEN)
    {
        touch_x = rb->button_get_data() >> 16;
        touch_y = rb->button_get_data() & 0xFFFF;
        if(touch_x >= xpos[0] && touch_x <= xpos[0]+(PAD_WIDTH*4))
            padmove(&p->w_pad[0], touch_y-(p->e_pad[0]*2+PAD_HEIGHT)/2);
        
        if(touch_x >= xpos[1]-(PAD_WIDTH*4) && touch_x <= xpos[1])
            padmove(&p->w_pad[1], touch_y-(p->e_pad[1]*2+PAD_HEIGHT)/2);
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

        if(key & PONG_LEFT_DOWN) /* player left goes down */
            padmove(&p->w_pad[0], MOVE_STEP);

        if(key & PONG_LEFT_UP)   /* player left goes up */
            padmove(&p->w_pad[0], -MOVE_STEP);

        if(key & PONG_RIGHT_DOWN) /* player right goes down */
            padmove(&p->w_pad[1], MOVE_STEP);

        if(key & PONG_RIGHT_UP)   /* player right goes up */
            padmove(&p->w_pad[1], -MOVE_STEP);
        
        if(rb->default_event_handler(key) == SYS_USB_CONNECTED)
            return -1; /* exit game because of USB */
    }
    return 1; /* return 0 to exit game */
}

void showscore(struct pong *p)
{
    static char buffer[20];
    int w;

    rb->snprintf(buffer, sizeof(buffer), "%d - %d", p->score[0], p->score[1]);
    w = rb->lcd_getstringsize((unsigned char *)buffer, NULL, NULL);
    rb->lcd_putsxy( (LCD_WIDTH / 2) - (w / 2), 0, (unsigned char *)buffer);
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    struct pong pong;
    int game = 1;

    /* init the struct with some silly values to start with */

    pong.ballx = 20*RES;
    pong.bally = 20*RES;

    pong.e_pad[0] = 0;
    pong.w_pad[0] = 7;
    pong.e_pad[1] = 0;
    pong.w_pad[1] = 40;

    pong.ballspeedx = SPEEDX;
    pong.ballspeedy = SPEEDY;

    pong.score[0] = pong.score[1] = 0; /* lets start at 0 - 0 ;-) */

    /* if you don't use the parameter, you can do like
       this to avoid the compiler warning about it */
    (void)parameter;
  
    rb = api; /* use the "standard" rb pointer */

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
        showscore(&pong);
        pad(&pong, 0); /* draw left pad */
        pad(&pong, 1); /* draw right pad */
        ball(&pong); /* move and draw ball */

        rb->lcd_update();

        game = keys(&pong); /* deal with keys */
    }

    return (game == 0) ? PLUGIN_OK : PLUGIN_USB_CONNECTED;
}

#endif /* HAVE_LCD_BITMAP */
