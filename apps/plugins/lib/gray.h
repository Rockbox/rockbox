/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Grayscale framework
*
* This is a generic framework to use grayscale display within Rockbox
* plugins. It obviously does not work for the player.
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

#ifndef __GRAY_H__
#define __GRAY_H__

#ifndef SIMULATOR /* not for simulator by now */
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP /* and also not for the Player */

/*===========================================================================
 Public functions and definitions, to be used within plugins
 ============================================================================
 */

/*---------------------------------------------------------------------------
 Initialize the framework
 ----------------------------------------------------------------------------
 every framework needs such a function, and it has to be called as the very
 first one
 */
void gray_init(struct plugin_api* newrb);

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 General functions 
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

/*---------------------------------------------------------------------------
 Prepare the grayscale display buffer
 ----------------------------------------------------------------------------
 arguments:
   gbuf      = pointer to the memory area to use (e.g. plugin buffer)
   gbuf_size = max usable size of the buffer
   width     = width in pixels  (1..112)
   bheight   = height in 8-pixel units  (1..8)
   depth     = desired number of shades - 1  (1..32)
 
 result:
   = depth  if there was enough memory
   < depth  if there wasn't enough memory. The number of displayable
            shades is smaller than desired, but it still works
   = 0      if there wasn't even enough memory for 1 bitplane (black & white)
 
 You can request any depth from 1 to 32, not just powers of 2. The routine
 performs "graceful degradation" if the memory is not sufficient for the
 desired depth. As long as there is at least enough memory for 1 bitplane,
 it creates as many bitplanes as fit into memory, although 1 bitplane will
 only deliver black & white display.

 If you need info about the memory taken by the grayscale buffer, supply an
 int* as the last parameter. This int will then contain the number of bytes
 used. The total memory needed can be calculated as follows:
   total_mem =
     sizeof(_tGraybuf)      (= 64 bytes currently)
   + sizeof(long)          (=  4 bytes)
   + (width * bheight + sizeof(long)) * depth
   + 0..3                  (longword alignment of grayscale display buffer)
 */
int gray_init_buffer(unsigned char *gbuf, int gbuf_size, int width,
                     int bheight, int depth, int *buf_taken);
                     
/*---------------------------------------------------------------------------
 Release the grayscale display buffer
 ----------------------------------------------------------------------------
 Switches the grayscale overlay off at first if it is still running,
 then sets the pointer to NULL.
 DO CALL either this function or at least gray_show_display(false)
 before you exit, otherwise nasty things may happen.
 */
void gray_release_buffer(void);

/*---------------------------------------------------------------------------
 Switch the grayscale overlay on or off
 ----------------------------------------------------------------------------
   enable = true:  the grayscale overlay is switched on if initialized
          = false: the grayscale overlay is switched off and the regular lcd
                   content is restored

 DO NOT call lcd_update() or any other api function that directly accesses
 the lcd while the grayscale overlay is running! If you need to do
 lcd_update() to update something outside the grayscale overlay area, use
 gray_deferred_update() instead.

 Other functions to avoid are:
   lcd_blit() (obviously), lcd_update_rect(), lcd_set_contrast(),
   lcd_set_invert_display(), lcd_set_flip(), lcd_roll()

 The grayscale display consumes ~50 % CPU power (for a full screen overlay,
 less if the overlay is smaller) when switched on. You can switch the overlay
 on and off as many times as you want.
 */
void gray_show_display(bool enable);

/*---------------------------------------------------------------------------
 Set position of the top left corner of the grayscale overlay
 ----------------------------------------------------------------------------
   x  = left margin in pixels
   by = top margin in 8-pixel units
 
 You may set this in a way that the overlay spills across the right or
 bottom display border. In this case it will simply be clipped by the
 LCD controller. You can even set negative values, this will clip at the
 left or top border. I did not test it, but the limits may be +127 / -128

 If you use this while the grayscale overlay is running, the now-freed area
 will be restored.
 */
void gray_position_display(int x, int by);

/*---------------------------------------------------------------------------
 Set the draw mode for subsequent drawing operations
 ----------------------------------------------------------------------------
   drawmode =
     GRAY_DRAW_INVERSE: Foreground pixels are inverted, background pixels are
                        left untouched
     GRAY_DRAW_FG:      Only foreground pixels are drawn
     GRAY_DRAW_BG:      Only background pixels are drawn
     GRAY_DRAW_SOLID:   Foreground and background pixels are drawn

 Default after initialization: GRAY_DRAW_SOLID
 */
void gray_set_drawmode(int drawmode);

/*---------------------------------------------------------------------------
 Draw modes, see above
 ----------------------------------------------------------------------------
 */
#define GRAY_DRAW_INVERSE 0
#define GRAY_DRAW_FG      1
#define GRAY_DRAW_BG      2
#define GRAY_DRAW_SOLID   3

/*---------------------------------------------------------------------------
 Set the foreground shade for subsequent drawing operations
 ----------------------------------------------------------------------------
 brightness = 0 (black) .. 255 (white)

 Default after initialization: 0
 */
void gray_set_foreground(int brightness);

/*---------------------------------------------------------------------------
 Set the background shade for subsequent drawing operations
 ----------------------------------------------------------------------------
 brightness = 0 (black) .. 255 (white)

 Default after initialization: 255
 */
void gray_set_background(int brightness);

/*---------------------------------------------------------------------------
 Set draw mode, foreground and background shades at once
 ----------------------------------------------------------------------------
 If you hand it -1 (or in fact any other out-of-bounds value) for a
 parameter, that particular setting won't be changed

 Default after initialization: GRAY_DRAW_SOLID, 0, 255
 */
void gray_set_drawinfo(int drawmode, int fg_brightness, int bg_brightness);

/*---------------------------------------------------------------------------
 Save the current display content (b&w and grayscale overlay) to an 8-bit
 BMP file in the root directory
 ----------------------------------------------------------------------------
 *
 * This one is rather slow if used with larger bit depths, but it's intended
 * primary use is for documenting the grayscale plugins. A much faster version
 * would be possible, but would take more than twice the RAM
 */
void gray_screendump(void);

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Functions affecting the whole display
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

/*---------------------------------------------------------------------------
 Clear the grayscale display (sets all pixels to white)
 ----------------------------------------------------------------------------
 */
void gray_clear_display(void);

/*---------------------------------------------------------------------------
 Set the grayscale display to all black 
 ----------------------------------------------------------------------------
 */
void gray_black_display(void);

/*---------------------------------------------------------------------------
 Do an lcd_update() to show changes done by rb->lcd_xxx() functions (in areas
 of the screen not covered by the grayscale overlay).
 ----------------------------------------------------------------------------
 If the grayscale overlay is running, the update will be done in the next
 call of the interrupt routine, otherwise it will be performed right away.
 See also comment for the gray_show_display() function.
 */
void gray_deferred_update(void);

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Scrolling functions
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

/*---------------------------------------------------------------------------
 Scroll the whole grayscale buffer left by <count> pixels
 ----------------------------------------------------------------------------
 black_border determines if the pixels scrolled in at the right are black
 or white

 Scrolling left/right by an even pixel count is almost twice as fast as
 scrolling by an odd pixel count.
 */
void gray_scroll_left(int count, bool black_border);

/*---------------------------------------------------------------------------
 Scroll the whole grayscale buffer right by <count> pixels
 ----------------------------------------------------------------------------
 black_border determines if the pixels scrolled in at the left are black
 or white

 Scrolling left/right by an even pixel count is almost twice as fast as
 scrolling by an odd pixel count.
 */
void gray_scroll_right(int count, bool black_border);

/*---------------------------------------------------------------------------
 Scroll the whole grayscale buffer up by 8 pixels
 ----------------------------------------------------------------------------
 black_border determines if the pixels scrolled in at the bottom are black
 or white

 Scrolling up/down by 8 pixels is very fast.
 */
void gray_scroll_up8(bool black_border);

/*---------------------------------------------------------------------------
 Scroll the whole grayscale buffer down by 8 pixels
 ----------------------------------------------------------------------------
 black_border determines if the pixels scrolled in at the top are black
 or white

 Scrolling up/down by 8 pixels is very fast.
 */
void gray_scroll_down8(bool black_border);

/*---------------------------------------------------------------------------
 Scroll the whole grayscale buffer up by <count> pixels (<= 7)
 ----------------------------------------------------------------------------
 black_border determines if the pixels scrolled in at the bottom are black
 or white

 Scrolling up/down pixel-wise is significantly slower than scrolling
 left/right or scrolling up/down byte-wise because it involves bit
 shifting. That's why it is asm optimized.
 */
void gray_scroll_up(int count, bool black_border);

/*---------------------------------------------------------------------------
 Scroll the whole grayscale buffer down by <count> pixels (<= 7)
 ----------------------------------------------------------------------------
 black_border determines if the pixels scrolled in at the top are black
 or white

 Scrolling up/down pixel-wise is significantly slower than scrolling
 left/right or scrolling up/down byte-wise because it involves bit
 shifting. That's why it is asm optimized.
 */
void gray_scroll_down(int count, bool black_border);

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Pixel and line functions
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

/*---------------------------------------------------------------------------
 Set a pixel with the current drawinfo
 ----------------------------------------------------------------------------
 If the drawmode is GRAY_DRAW_INVERSE, the pixel is inverted
 GRAY_DRAW_FG and GRAY_DRAW_SOLID draw the pixel in the foreground shade
 GRAY_DRAW_BG draws the pixel in the background shade
 */
void gray_drawpixel(int x, int y);

/*---------------------------------------------------------------------------
 Draw a line from (x1, y1) to (x2, y2) with the current drawinfo
 ----------------------------------------------------------------------------
 See gray_drawpixel() for details
 */
void gray_drawline(int x1, int y1, int x2, int y2);

/*---------------------------------------------------------------------------
 Draw a horizontal line from (x1, y) to (x2, y) with the current drawinfo
 ----------------------------------------------------------------------------
 See gray_drawpixel() for details
 */
void gray_horline(int x1, int x2, int y);

/*---------------------------------------------------------------------------
 Draw a vertical line from (x, y1) to (x, y2) with the current drawinfo
 ----------------------------------------------------------------------------
 See gray_drawpixel() for details
 This one uses the block drawing optimization, so it is rather fast.
 */
void gray_verline(int x, int y1, int y2);

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Rectangle functions
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

/*---------------------------------------------------------------------------
 Draw a (hollow) rectangle with the current drawinfo
 ----------------------------------------------------------------------------
 See gray_drawpixel() for details
 */
void gray_drawrect(int x, int y, int nx, int ny);

/*---------------------------------------------------------------------------
 Draw a filled rectangle with the current drawinfo
 ----------------------------------------------------------------------------
 See gray_drawpixel() for details
 This one uses the block drawing optimization, so it is rather fast.
 */
void gray_fillrect(int x, int y, int nx, int ny);

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Bitmap functions
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

/*---------------------------------------------------------------------------
 Copy a grayscale bitmap into the display
 ----------------------------------------------------------------------------
 A grayscale bitmap contains one byte for every pixel that defines the
 brightness of the pixel (0..255). Bytes are read in row-major order.
 The <stride> parameter is useful if you want to show only a part of a
 bitmap. It should always be set to the "row length" of the bitmap, so
 for displaying the whole bitmap, nx == stride.
 
 This is the only drawing function NOT using the drawinfo.
 */
void gray_drawgraymap(const unsigned char *src, int x, int y, int nx, int ny,
                      int stride);

/*---------------------------------------------------------------------------
 Display a bitmap with the current drawinfo
 ----------------------------------------------------------------------------
 The drawmode is used as described for gray_set_drawmode()

 This (now) uses the same bitmap format as the core b&w graphics routines,
 so you can use bmp2rb to generate bitmaps for use with this function as
 well.

 A bitmap contains one bit for every pixel that defines if that pixel is
 foreground (1) or background (0). Bits within a byte are arranged
 vertically, LSB at top.
 The bytes are stored in row-major order, with byte 0 being top left,
 byte 1 2nd from left etc. The first row of bytes defines pixel rows
 0..7, the second row defines pixel row 8..15 etc.

 The <stride> parameter is useful if you want to show only a part of a
 bitmap. It should always be set to the "row length" of the bitmap.
 */
void gray_drawbitmap(const unsigned char *src, int x, int y, int nx, int ny,
                     int stride);

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Font support
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

/*---------------------------------------------------------------------------
 Set font for the font routines
 ----------------------------------------------------------------------------
 newfont can be FONT_SYSFIXED or FONT_UI the same way as with the Rockbox
 core routines
 
 Default after initialization: FONT_SYSFIXED
 */
void gray_setfont(int newfont);

/*---------------------------------------------------------------------------
 Calculate width and height of the given text in pixels when rendered with
 the currently selected font.
 ----------------------------------------------------------------------------
 This works exactly the same way as the core lcd_getstringsize(), only that
 it uses the selected font for grayscale.
 */
int gray_getstringsize(const unsigned char *str, int *w, int *h);

/*---------------------------------------------------------------------------
 Display text starting at (x, y) with the current font and drawinfo
 ----------------------------------------------------------------------------
 The drawmode is used as described for gray_set_drawmode()
 */
void gray_putsxy(int x, int y, const unsigned char *str);

/*===========================================================================
 Private functions and definitions, for use within the grayscale core only
 ============================================================================
 */

/* flag definitions */
#define _GRAY_RUNNING          0x0001  /* grayscale overlay is running */
#define _GRAY_DEFERRED_UPDATE  0x0002  /* lcd_update() requested */

/* unsigned 16 bit multiplication (a single instruction on the SH) */
#define MULU16(a, b) ((unsigned long) \
                     (((unsigned short) (a)) * ((unsigned short) (b))))

/* The grayscale buffer management structure */
typedef struct
{
    int x;
    int by;         /* 8-pixel units */
    int width;
    int height;
    int bheight;    /* 8-pixel units */
    int plane_size;
    int depth;      /* number_of_bitplanes  = (number_of_grayscales - 1) */
    int cur_plane;  /* for the timer isr */
    unsigned long randmask;    /* mask for random value in _writepixel() */
    unsigned long flags;       /* various flags, see #defines */
    unsigned long *bitpattern; /* pointer to start of pattern table */
    unsigned char *data;       /* pointer to start of bitplane data */
    unsigned long fg_pattern;  /* current foreground pattern */
    unsigned long bg_pattern;  /* current background pattern */
    int drawmode;              /* current draw mode */
    struct font *curfont;      /* current selected font */
} _tGraybuf;

/* Global variables */
extern struct plugin_api *_gray_rb;
extern _tGraybuf *_graybuf;
extern short _gray_random_buffer;

/* Global function pointers */
extern void (* const _gray_pixelfuncs[4])(int x, int y, unsigned long pattern);
extern void (* const _gray_blockfuncs[4])(unsigned char *address, unsigned mask,
                                          unsigned bits);

#endif /* HAVE_LCD_BITMAP */
#endif /* SIMULATOR */
#endif /* __GRAY_H__ */
