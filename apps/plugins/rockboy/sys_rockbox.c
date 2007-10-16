/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Michiel van der Kolk, Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "rockmacros.h"
#include "fb.h"
#include "input.h"
#include "lcd-gb.h"
#include "hw.h"
#include "config.h"

#if CONFIG_KEYPAD == SANSA_E200_PAD
#define ROCKBOY_SCROLLWHEEL
#define ROCKBOY_SCROLLWHEEL_CC  BUTTON_SCROLL_UP
#define ROCKBOY_SCROLLWHEEL_CW  BUTTON_SCROLL_DOWN
#endif

struct fb fb IBSS_ATTR;

extern int debug_trace;

unsigned int oldbuttonstate = 0, newbuttonstate,holdbutton;
#ifdef HAVE_WHEEL_POSITION
int oldwheel = -1, wheel;

static int wheelmap[8] = {
    PAD_UP,     /* Top */
    PAD_A,      /* Top-right */
    PAD_RIGHT,  /* Right */
    PAD_START,  /* Bottom-right */
    PAD_DOWN,   /* Bottom */
    PAD_SELECT, /* Bottom-left */
    PAD_LEFT,   /* Left */
    PAD_B       /* Top-left */
};
#endif

int released, pressed;


#ifdef ROCKBOY_SCROLLWHEEL
/* Scrollwheel events are posted directly and not polled by the button
   driver - synthesize polling */
static inline unsigned int read_scroll_wheel(void)
{
    unsigned int buttons = BUTTON_NONE;
    unsigned int btn;

    /* Empty out the button queue and see if any scrollwheel events were
       posted */
    do
    {
        btn = rb->button_get_w_tmo(0);
        buttons |= btn;
    }
    while (btn != BUTTON_NONE);

    return buttons & (ROCKBOY_SCROLLWHEEL_CC | ROCKBOY_SCROLLWHEEL_CW);
}
#endif

void ev_poll(void)
{
    event_t ev;
    newbuttonstate = rb->button_status();
#ifdef ROCKBOY_SCROLLWHEEL
    newbuttonstate |= read_scroll_wheel();
#endif
    released = ~newbuttonstate & oldbuttonstate;
    pressed = newbuttonstate & ~oldbuttonstate;
    oldbuttonstate = newbuttonstate;
#if (LCD_WIDTH == 160) && (LCD_HEIGHT == 128) && (LCD_DEPTH == 2)
    if (rb->button_hold()&~holdbutton)
        fb.mode=(fb.mode+1)%4;
    holdbutton=rb->button_hold();
#elif CONFIG_KEYPAD == RECORDER_PAD
    if (pressed & BUTTON_ON)
        fb.mode=(fb.mode+1)%4;
#endif

#ifdef HAVE_WHEEL_POSITION
    /* Get the current wheel position - 0..95 or -1 for untouched */
    wheel = rb->wheel_status(); 

    /* Convert to number from 0 to 7 - clockwise from top */
    if ( wheel > 0 ){
        wheel += 6;
        wheel /= 12;
        if ( wheel > 7 ) wheel = 0; 
    }

    if ( wheel != oldwheel ) {
        if (oldwheel >= 0) {
            ev.type = EV_RELEASE;
            ev.code = wheelmap[oldwheel];
            ev_postevent(&ev);
        }

        if (wheel >= 0) {
            ev.type = EV_PRESS;
            ev.code = wheelmap[wheel];
            ev_postevent(&ev);
        }
    }

    oldwheel = wheel;
    if(released) {
        ev.type = EV_RELEASE;
        if ( released & (~BUTTON_SELECT) ) { ev.code=PAD_B; ev_postevent(&ev); }
        if ( released & BUTTON_SELECT ) { ev.code=PAD_A; ev_postevent(&ev); }
    }
    if(pressed) { /* button press */
        ev.type = EV_PRESS;
        if ( pressed & (~BUTTON_SELECT) ) { ev.code=PAD_B; ev_postevent(&ev); }
        if ( pressed & BUTTON_SELECT ) { ev.code=PAD_A; ev_postevent(&ev); }
    }    
#else
    if(released) {
        ev.type = EV_RELEASE;
        if(released & options.LEFT) { ev.code=PAD_LEFT; ev_postevent(&ev); }
        if(released & options.RIGHT) {ev.code=PAD_RIGHT; ev_postevent(&ev);}
        if(released & options.DOWN) { ev.code=PAD_DOWN; ev_postevent(&ev); }
        if(released & options.UP) { ev.code=PAD_UP; ev_postevent(&ev); }
        if(released & options.A) { ev.code=PAD_A; ev_postevent(&ev); }
        if(released & options.B) { ev.code=PAD_B; ev_postevent(&ev); }
        if(released & options.START) {
            ev.code=PAD_START;
            ev_postevent(&ev);
        }
        if(released & options.SELECT) {
            ev.code=PAD_SELECT;
            ev_postevent(&ev);
        }
    }
    if(pressed) { /* button press */
        ev.type = EV_PRESS;
        if(pressed & options.LEFT) { ev.code=PAD_LEFT; ev_postevent(&ev); }
        if(pressed & options.RIGHT) { ev.code=PAD_RIGHT; ev_postevent(&ev);}
        if(pressed & options.DOWN) { ev.code=PAD_DOWN; ev_postevent(&ev); }
        if(pressed & options.UP) { ev.code=PAD_UP; ev_postevent(&ev); }
        if(pressed & options.A) { ev.code=PAD_A; ev_postevent(&ev); }
        if(pressed & options.B) { ev.code=PAD_B; ev_postevent(&ev); }
        if(pressed & options.START) {
            ev.code=PAD_START;
            ev_postevent(&ev); 
        }
        if(pressed & options.SELECT) {
            ev.code=PAD_SELECT;
            ev_postevent(&ev);
        }
#endif
#if CONFIG_KEYPAD == IPOD_4G_PAD
        if(rb->button_hold()) {
#else
        if(pressed & options.MENU) {
#endif
#if (CONFIG_KEYPAD != RECORDER_PAD)
#ifdef HAVE_WHEEL_POSITION
            rb->wheel_send_events(true);
#endif
            if (do_user_menu() == USER_MENU_QUIT) 
#endif
            {
                die("");
                cleanshut=1;
            }
#ifdef HAVE_WHEEL_POSITION
            rb->wheel_send_events(false);
#endif
        }

#ifndef HAVE_WHEEL_POSITION
    }
#endif    
}

/* New frameskip, makes more sense to me and performs as well */
inline void vid_begin(void)
{
    static int skip = 0;
    if (skip<options.frameskip)
    {
        skip++;
        fb.enabled=0;
    }
    else
    {
        skip=0;
        fb.enabled=1;
    }
}

void vid_init(void)
{
    fb.enabled=1;

#if defined(HAVE_LCD_COLOR)
    fb.cc[0].r = 3;  /* 8-5 (wasted bits on red) */
    fb.cc[0].l = 11; /* this is the offset to the R bits (16-5) */
    fb.cc[1].r = 2;  /* 8-6 (wasted bits on green) */
    fb.cc[1].l = 5;  /* This is the offset to the G bits (16-5-6) */
    fb.cc[2].r = 3;  /* 8-5 (wasted bits on red) */
    fb.cc[2].l = 0;  /* This is the offset to the B bits (16-5-6-5) */
#else
    fb.mode=3;
#endif
}

#if !defined(HAVE_LCD_COLOR)
/* Color targets are handled in lcd.c */
fb_data *frameb;
void vid_update(int scanline) 
{ 
   register int cnt=0;
    int scanline_remapped;
#if (LCD_HEIGHT == 64) && (LCD_DEPTH == 1) /* Archos */
    int balance = 0;
    if (fb.mode==1)
        scanline-=16;
    else if (fb.mode==2)
        scanline-=8;
    scanline_remapped = scanline / 16;
    frameb = rb->lcd_framebuffer + scanline_remapped * LCD_WIDTH;
    while (cnt < 160) {
        balance += LCD_WIDTH;
        if (balance > 0)
        {
#if (CONFIG_CPU == SH7034) && !defined(SIMULATOR)
             asm volatile (
                 "mov.b   @%0,r0         \n"
                 "add     %1,%0          \n"
                 "tst     #0x02, r0      \n"  /* ~bit 1 */
                 "rotcr   r1             \n"
                 "mov.b   @%0,r0         \n"
                 "add     %1,%0          \n"
                 "tst     #0x02, r0      \n"  /* ~bit 1 */
                 "rotcr   r1             \n"
                 "mov.b   @%0,r0         \n"
                 "add     %1,%0          \n"
                 "tst     #0x02, r0      \n"  /* ~bit 1 */
                 "rotcr   r1             \n"
                 "mov.b   @%0,r0         \n"
                 "add     %1,%0          \n"
                 "tst     #0x02, r0      \n"  /* ~bit 1 */
                 "rotcr   r1             \n"
                 "mov.b   @%0,r0         \n"
                 "add     %1,%0          \n"
                 "tst     #0x02, r0      \n"  /* ~bit 1 */
                 "rotcr   r1             \n"
                 "mov.b   @%0,r0         \n"
                 "add     %1,%0          \n"
                 "tst     #0x02, r0      \n"  /* ~bit 1 */
                 "rotcr   r1             \n"
                 "mov.b   @%0,r0         \n"
                 "add     %1,%0          \n"
                 "tst     #0x02, r0      \n"  /* ~bit 1 */
                 "rotcr   r1             \n"
                 "mov.b   @%0,r0         \n"
                 "add     %1,%0          \n"
                 "tst     #0x02, r0      \n"  /* ~bit 1 */
                 "rotcr   r1             \n"

                 "shlr16  r1             \n"
                 "shlr8   r1             \n"
                 "not     r1,r1          \n"  /* account for negated bits */
                 "mov.b   r1,@%2         \n"
                 : /* outputs */
                 : /* inputs */
                 /* %0 */ "r"(scan.buf[0] + cnt),
                 /* %1 */ "r"(256), /* scan.buf line length */
                 /* %2 */ "r"(frameb++)
                 : /* clobbers */
                 "r0", "r1"
             );
#else
             register unsigned scrbyte = 0;
             if (scan.buf[0][cnt] & 0x02)  scrbyte |= 0x01;
             if (scan.buf[1][cnt] & 0x02)  scrbyte |= 0x02;
             if (scan.buf[2][cnt] & 0x02)  scrbyte |= 0x04;
             if (scan.buf[3][cnt] & 0x02)  scrbyte |= 0x08;
             if (scan.buf[4][cnt] & 0x02)  scrbyte |= 0x10;
             if (scan.buf[5][cnt] & 0x02)  scrbyte |= 0x20;
             if (scan.buf[6][cnt] & 0x02)  scrbyte |= 0x40;
             if (scan.buf[7][cnt] & 0x02)  scrbyte |= 0x80;
             *(frameb++) = scrbyte;
#endif
             balance -= 160;
        }
        cnt ++;
    }
    rb->lcd_update_rect(0, (scanline/2) & ~7, LCD_WIDTH, 8);
#elif (LCD_HEIGHT == 128) && (LCD_DEPTH == 2) /* iriver H1x0 */
    if (fb.mode==1)
        scanline-=16;
    else if (fb.mode==2)
        scanline-=8;
    scanline_remapped = scanline / 4;
    frameb = rb->lcd_framebuffer + scanline_remapped * LCD_WIDTH;
    while (cnt < 160) {
        *(frameb++) = (scan.buf[0][cnt]&0x3) |
                      ((scan.buf[1][cnt]&0x3)<<2) |
                      ((scan.buf[2][cnt]&0x3)<<4) |
                      ((scan.buf[3][cnt]&0x3)<<6);
        cnt++;
    }
    rb->lcd_update_rect(0, scanline & ~3, LCD_WIDTH, 4);
#elif defined(HAVE_LCD_COLOR)
    /* handled in lcd.c now */
#endif /* LCD_HEIGHT */
}
#endif

