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
#include "rc.h"
#include "lcd.h"
#include "hw.h"
#include "config.h"

rcvar_t joy_exports[] =
{
            RCV_END
};

rcvar_t vid_exports[] =
{
            RCV_END
};

struct fb fb;
byte *video_base_buf;

extern int debug_trace;

static byte frameb[145][160];

void vid_settitle(char *title)
{
    rb->splash(HZ*2, true, title);
}

void joy_init(void)
{
}

void joy_close(void)
{
}

#if CONFIG_KEYPAD == IRIVER_H100_PAD
#define ROCKBOY_PAD_A BUTTON_ON
#define ROCKBOY_PAD_B BUTTON_OFF
#define ROCKBOY_PAD_START BUTTON_REC
#define ROCKBOY_PAD_SELECT BUTTON_SELECT
#define ROCKBOY_MENU BUTTON_MODE

#elif CONFIG_KEYPAD == RECORDER_PAD
#define ROCKBOY_PAD_A BUTTON_F1
#define ROCKBOY_PAD_B BUTTON_F2
#define ROCKBOY_PAD_START BUTTON_F3
#define ROCKBOY_PAD_SELECT BUTTON_PLAY
#define ROCKBOY_MENU BUTTON_OFF

#endif

unsigned int oldbuttonstate = 0, newbuttonstate,holdbutton;

void ev_poll(void)
{
    event_t ev;
    int released, pressed;
    newbuttonstate = rb->button_status();
    released = ~newbuttonstate & oldbuttonstate;
    pressed = newbuttonstate & ~oldbuttonstate;
    oldbuttonstate = newbuttonstate;
#if CONFIG_KEYPAD == IRIVER_H100_PAD
    if (rb->button_hold()&~holdbutton)
	fb.mode=(fb.mode+1)%4;
    holdbutton=rb->button_hold();
#endif
    if(released) {
        ev.type = EV_RELEASE;
        if(released & BUTTON_LEFT) { ev.code=PAD_LEFT; ev_postevent(&ev); }
        if(released & BUTTON_RIGHT) {ev.code=PAD_RIGHT; ev_postevent(&ev);}
        if(released & BUTTON_DOWN) { ev.code=PAD_DOWN; ev_postevent(&ev); }
        if(released & BUTTON_UP) { ev.code=PAD_UP; ev_postevent(&ev); }
        if(released & ROCKBOY_PAD_A) { ev.code=PAD_A; ev_postevent(&ev); }
        if(released & ROCKBOY_PAD_B) { ev.code=PAD_B; ev_postevent(&ev); }
        if(released & ROCKBOY_PAD_START) {
            ev.code=PAD_START;
            ev_postevent(&ev);
        }
        if(released & ROCKBOY_PAD_SELECT) {
            ev.code=PAD_SELECT;
            ev_postevent(&ev);
        }
    }
    if(pressed) { /* button press */
        ev.type = EV_PRESS;
        if(pressed & BUTTON_LEFT) { ev.code=PAD_LEFT; ev_postevent(&ev); }
        if(pressed & BUTTON_RIGHT) { ev.code=PAD_RIGHT; ev_postevent(&ev);}
        if(pressed & BUTTON_DOWN) { ev.code=PAD_DOWN; ev_postevent(&ev); }
        if(pressed & BUTTON_UP) { ev.code=PAD_UP; ev_postevent(&ev); }
        if(pressed & ROCKBOY_PAD_A) { ev.code=PAD_A; ev_postevent(&ev); }
        if(pressed & ROCKBOY_PAD_B) { ev.code=PAD_B; ev_postevent(&ev); }
        if(pressed & ROCKBOY_PAD_START) {
            ev.code=PAD_START;
            ev_postevent(&ev); 
        }
        if(pressed & ROCKBOY_PAD_SELECT) {
            ev.code=PAD_SELECT;
            ev_postevent(&ev);
        }
        if(pressed & ROCKBOY_MENU) {
#if CONFIG_KEYPAD == IRIVER_H100_PAD
          if (do_user_menu() == USER_MENU_QUIT) 
#endif
          {
            die("");
            cleanshut=1;
          }
        }
    }
    
}

void vid_setpal(int i, int r, int g, int b)
{
    (void)i;
    (void)r;
    (void)g;
    (void)b;
}

void vid_begin(void)
{
}

void vid_init(void)
{
    fb.pelsize=1; // 8 bit framebuffer.. (too much.. but lowest gnuboy will support.. so yea...
    fb.h=144;
    fb.w=160;
    fb.pitch=160;
    fb.enabled=1;
    fb.dirty=0;
    video_base_buf=fb.ptr=(byte *)frameb;
    fb.mode=3;
}

void vid_update(int scanline) 
{
    int cnt=0,scanline_remapped;
    byte *frameb;
#if LCD_HEIGHT == 64 /* Archos */
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
#ifdef SIMULATOR /* simulator uses C */
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
#else
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
#endif
             balance -= 160;
        }
        cnt ++;
    }
    rb->lcd_update_rect(0, (scanline/2) & ~7, LCD_WIDTH, 8);
#else /* LCD_HEIGHT != 64, iRiver */
    if (fb.mode==1)
      scanline-=16;
    else if (fb.mode==2)
      scanline-=8;
#ifdef GRAYSCALE
    scanline_remapped = scanline / 4;
#else
    scanline_remapped = scanline / 8;
#endif	    
    frameb = rb->lcd_framebuffer + scanline_remapped * LCD_WIDTH;
    while (cnt < 160) {
#ifdef GRAYSCALE
        *(frameb++) = (scan.buf[0][cnt]&0x3) |
                      ((scan.buf[1][cnt]&0x3)<<2) |
                      ((scan.buf[2][cnt]&0x3)<<4) |
                      ((scan.buf[3][cnt]&0x3)<<6);
        cnt++;
    }
    rb->lcd_update_rect(0, scanline & ~3, LCD_WIDTH, 4); //8);
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
        cnt++;
    }
    rb->lcd_update_rect(0, scanline & ~7, LCD_WIDTH, 8);
#endif /* GRAYSCALE */
#endif /* LCD_HEIGHT */
}

void vid_end(void)
{
}

long timerresult;

void *sys_timer(void) 
{
   timerresult=*rb->current_tick;
   return &timerresult;
}

// returns microseconds passed since sys_timer
int sys_elapsed(long *oldtick) 
{
   return ((*rb->current_tick-(*oldtick))*1000000)/HZ;
}

void sys_sleep(int us)
{
  if (us <= 0) return;
//  rb->sleep(HZ*us/1000000);
}
