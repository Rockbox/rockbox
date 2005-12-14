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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP

#define PAD_HEIGHT LCD_HEIGHT / 6    /* Recorder: 10   iRiver: 21 */
#define PAD_WIDTH LCD_WIDTH / 50     /* Recorder: 2    iRiver: 2  */

#define BALL_HEIGTH LCD_HEIGHT / 32  /* Recorder: 2    iRiver: 4  */
#define BALL_WIDTH LCD_HEIGHT / 32   /* We want a square ball */

#define SPEEDX ( LCD_WIDTH * 3 ) / 2 /* Recorder: 168  iRiver: 240 */
#define SPEEDY LCD_HEIGHT * 2        /* Recorder: 128  iRiver: 256 */

#define RES 100

#define MOVE_STEP LCD_HEIGHT / 32 /* move pad this many steps up/down each move */

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define PONG_QUIT BUTTON_OFF
#define PONG_LEFT_UP BUTTON_F1
#define PONG_LEFT_DOWN BUTTON_LEFT
#define PONG_RIGHT_UP BUTTON_F3
#define PONG_RIGHT_DOWN BUTTON_RIGHT

#elif CONFIG_KEYPAD == ONDIO_PAD
#define PONG_QUIT BUTTON_OFF
#define PONG_LEFT_UP BUTTON_LEFT
#define PONG_LEFT_DOWN BUTTON_MENU
#define PONG_RIGHT_UP BUTTON_UP
#define PONG_RIGHT_DOWN BUTTON_DOWN

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define PONG_QUIT BUTTON_OFF
#define PONG_LEFT_UP BUTTON_UP
#define PONG_LEFT_DOWN BUTTON_DOWN
#define PONG_RIGHT_UP BUTTON_ON
#define PONG_RIGHT_DOWN BUTTON_MODE

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_NANO_PAD)
#define PONG_QUIT BUTTON_SELECT
#define PONG_LEFT_UP BUTTON_MENU
#define PONG_LEFT_DOWN BUTTON_LEFT
#define PONG_RIGHT_UP BUTTON_RIGHT
#define PONG_RIGHT_DOWN BUTTON_PLAY

#endif

static struct plugin_api* rb;

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

void pad(struct pong *p, int pad)
{
    static int xpos[2]={0, LCD_WIDTH-PAD_WIDTH};

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
       (y + BALL_HEIGTH > p->e_pad[pad])) {
        /* Y seems likely right */

        /* store the delta between ball-middle MINUS pad-middle, so
           it returns:
           0 when the ball hits exactly the middle of the pad
           positive numbers when the ball is below the middle of the pad
           negative numbers when the ball is above the middle of the pad

           max number is +- PAD_HEIGHT/2
        */

        *info = (y+BALL_HEIGTH/2) - (p->e_pad[pad] + PAD_HEIGHT/2);

        if(pad) {
            /* right-side */
            if((x + BALL_WIDTH) > (LCD_WIDTH - PAD_WIDTH))
                return true; /* phump */
        }
        else {
            if(x <= 0)
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
    rb->splash(HZ/4, true, "%s scores!", pad?"right":"left");
    rb->lcd_clear_display();
    p->score[pad]++;

    /* then move the X-speed of the ball and give it a random Y position */
    p->ballspeedx = -p->ballspeedx;
    p->bally = rb->rand()%(LCD_HEIGHT-BALL_HEIGTH);

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
    if(newy + BALL_HEIGTH > LCD_HEIGHT) {
        /* hit floor, bounce */
        p->ballspeedy = -p->ballspeedy;
        newy = LCD_HEIGHT - BALL_HEIGTH;
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

    /* clear old position */
    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    rb->lcd_fillrect(x, y, BALL_WIDTH, BALL_HEIGTH);
    rb->lcd_set_drawmode(DRMODE_SOLID);

    /* draw the new ball position */
    rb->lcd_fillrect(newx, newy, BALL_WIDTH, BALL_HEIGTH);
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

    int time = 4; /* number of ticks this function will loop reading keys */
    int start = *rb->current_tick;
    int end = start + time;

    while(end > *rb->current_tick) {
        key = rb->button_get_w_tmo(end - *rb->current_tick);

        if(key & PONG_QUIT)
            return 0; /* exit game NOW */

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
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
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

    TEST_PLUGIN_API(api);
    
    rb = api; /* use the "standard" rb pointer */

    /* Clear screen */
    rb->lcd_clear_display();

    /* go go go */
    while(game > 0) {
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
