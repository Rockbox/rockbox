/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Grayscale framework & demo plugin
*
* Copyright (C) 2004 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

/* You do not want to touch these if you don't know exactly what you're
 * doing. */

#define GRAY_RUNNING          0x0001  /* grayscale overlay is running */
#define GRAY_DEFERRED_UPDATE  0x0002  /* lcd_update() requested */

/* unsigned 16 bit multiplication (a single instruction on the SH) */
#define MULU16(a, b) (((unsigned short) (a)) * ((unsigned short) (b)))

typedef struct
{
    int x;
    int by;        /* 8-pixel units */
    int width;
    int height;
    int bheight;   /* 8-pixel units */
    int plane_size;
    int depth;     /* number_of_bitplanes  = (number_of_grayscales - 1) */
    int cur_plane; /* for the timer isr */
    unsigned long randmask; /* mask for random value in graypixel() */
    unsigned long flags;    /* various flags, see #defines */
    unsigned char *data;    /* pointer to start of bitplane data */
    unsigned long *bitpattern; /* pointer to start of pattern table */
} tGraybuf;

static tGraybuf *graybuf = NULL;
static short gray_random_buffer;

/** prototypes **/

void gray_timer_isr(void);
void graypixel(int x, int y, unsigned long pattern);
void grayblock(int x, int by, unsigned char* src, int stride);
void grayinvertmasked(int x, int by, unsigned char mask);
int gray_init_buffer(unsigned char *gbuf, int gbuf_size, int width,
                     int bheight, int depth);
void gray_release_buffer(void);
void gray_position_display(int x, int by);
void gray_show_display(bool enable);

/* functions affecting the whole display */
void gray_clear_display(void);
void gray_black_display(void);
void gray_deferred_update(void);

/* scrolling functions */
void gray_scroll_left(int count, bool black_border);
void gray_scroll_right(int count, bool black_border);
void gray_scroll_up8(bool black_border);
void gray_scroll_down8(bool black_border);
void gray_scroll_up(int count, bool black_border);
void gray_scroll_down(int count, bool black_border);

/* pixel functions */
void gray_drawpixel(int x, int y, int brightness);
void gray_invertpixel(int x, int y);

/* line functions */
void gray_drawline(int x1, int y1, int x2, int y2, int brightness);
void gray_invertline(int x1, int y1, int x2, int y2);

/* rectangle functions */
void gray_drawrect(int x1, int y1, int x2, int y2, int brightness);
void gray_fillrect(int x1, int y1, int x2, int y2, int brightness);
void gray_invertrect(int x1, int y1, int x2, int y2);

/* bitmap functions */
void gray_drawgraymap(unsigned char *src, int x, int y, int nx, int ny,
                      int stride);
void gray_drawbitmap(unsigned char *src, int x, int y, int nx, int ny,
                     int stride, bool draw_bg, int fg_brightness,
                     int bg_brightness);
