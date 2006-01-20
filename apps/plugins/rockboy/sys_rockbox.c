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
#include "lcd-gb.h"
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

extern int debug_trace;

void vid_settitle(char *title)
{
    rb->splash(HZ/2, true, title);
}

void joy_init(void)
{
}

void joy_close(void)
{
}

unsigned int oldbuttonstate = 0, newbuttonstate,holdbutton;

int released, pressed;

void ev_poll(void)
{
    event_t ev;
    newbuttonstate = rb->button_status();
    released = ~newbuttonstate & oldbuttonstate;
    pressed = newbuttonstate & ~oldbuttonstate;
    oldbuttonstate = newbuttonstate;
#if CONFIG_KEYPAD == IRIVER_H100_PAD
    if (rb->button_hold()&~holdbutton)
        fb.mode=(fb.mode+1)%4;
    holdbutton=rb->button_hold();
#elif CONFIG_KEYPAD == RECORDER_PAD
    if (pressed & BUTTON_ON)
        fb.mode=(fb.mode+1)%4;
#endif
    if(released) {
        ev.type = EV_RELEASE;
        if(released & BUTTON_LEFT) { ev.code=PAD_LEFT; ev_postevent(&ev); }
        if(released & BUTTON_RIGHT) {ev.code=PAD_RIGHT; ev_postevent(&ev);}
        if(released & BUTTON_DOWN) { ev.code=PAD_DOWN; ev_postevent(&ev); }
        if(released & BUTTON_UP) { ev.code=PAD_UP; ev_postevent(&ev); }
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
        if(pressed & BUTTON_LEFT) { ev.code=PAD_LEFT; ev_postevent(&ev); }
        if(pressed & BUTTON_RIGHT) { ev.code=PAD_RIGHT; ev_postevent(&ev);}
        if(pressed & BUTTON_DOWN) { ev.code=PAD_DOWN; ev_postevent(&ev); }
        if(pressed & BUTTON_UP) { ev.code=PAD_UP; ev_postevent(&ev); }
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
        if(pressed & options.MENU) {
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
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

inline void vid_begin(void) // New frameskip, makes more sense to me and performs as well
{
	static int skip = 0;
   if (skip<options.frameskip) {
      skip++;
      fb.enabled=0;
   }
   else {
      skip=0;
      fb.enabled=1;
   }
}

void vid_init(void)
{
    fb.h=144;
    fb.w=160;
    fb.pitch=160;
    fb.enabled=1;
    fb.dirty=0;
    fb.mode=3;

   fb.ptr=rb->lcd_framebuffer;

#if defined(HAVE_LCD_COLOR)
    fb.pelsize=2; // 16 bit framebuffer

	fb.indexed = 0; // no palette on lcd
	fb.cc[0].r = 3;  // 8-5 (wasted bits on red)
	fb.cc[0].l = 11;  //this is the offset to the R bits (16-5)
	fb.cc[1].r = 2;  // 8-6 (wasted bits on green)
	fb.cc[1].l = 5;  // This is the offset to the G bits (16-5-6)
	fb.cc[2].r = 3;  // 8-5 (wasted bits on red)
	fb.cc[2].l = 0;  // This is the offset to the B bits (16-5-6-5)
	fb.cc[3].r = 0;  // no alpha
	fb.cc[3].l = 0;
	fb.yuv = 0;		// not in yuv format
#else // ***** NEED TO LOOK INTO THIS MORE FOR THE H100 (Should be able to get rid of some IFDEF's elsewhere)
    fb.pelsize=1; // 8 bit framebuffer.. (too much.. but lowest gnuboy will support.. so yea...
#endif
}

#if LCD_HEIGHT<144
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
#elif (LCD_HEIGHT >= 144) && defined(HAVE_LCD_COLOR) /* iriver H3x0, colour iPod */
    // handled in lcd.c now
#endif /* LCD_HEIGHT */
}
#endif

void vid_end(void)
{
}

long timerresult;

void *sys_timer(void) 
{/*
   timerresult=*rb->current_tick;
   return &timerresult;*/
   return 0;
}

// returns microseconds passed since sys_timer
int sys_elapsed(long *oldtick) 
{
/*
   int elap,mytime=microtick;

   elap=mytime-*oldtick;
   *oldtick=mytime;
   return elap;*/
//   return ((*rb->current_tick-(*oldtick))*1000000)/HZ;
   return *oldtick;
}

void sys_sleep(int us)
{
   if(us<=0) return;
   int i=0;
   while(i< us*11)
      i++;
//  if (us <= 0) return;
}
