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
* Core functions
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

#ifndef SIMULATOR /* not for simulator by now */
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP /* and also not for the Player */
#include "gray.h"

/* Global variables */
struct plugin_api *_gray_rb = NULL; /* global api struct pointer */
_tGraybuf *_graybuf = NULL;         /* pointer to grayscale buffer */
short _gray_random_buffer;          /* buffer for random number generator */

/* Prototypes */
static void _timer_isr(void);

/* timer interrupt handler: display next bitplane */
static void _timer_isr(void)
{
    _gray_rb->lcd_blit(_graybuf->data + MULU16(_graybuf->plane_size,
                       _graybuf->cur_plane), _graybuf->x, _graybuf->by,
                       _graybuf->width, _graybuf->bheight, _graybuf->width);

    if (++_graybuf->cur_plane >= _graybuf->depth)
        _graybuf->cur_plane = 0;

    if (_graybuf->flags & _GRAY_DEFERRED_UPDATE)  /* lcd_update() requested? */
    {
        int x1 = MAX(_graybuf->x, 0);
        int x2 = MIN(_graybuf->x + _graybuf->width, LCD_WIDTH);
        int y1 = MAX(_graybuf->by << 3, 0);
        int y2 = MIN((_graybuf->by << 3) + _graybuf->height, LCD_HEIGHT);

        if (y1 > 0)  /* refresh part above overlay, full width */
            _gray_rb->lcd_update_rect(0, 0, LCD_WIDTH, y1);

        if (y2 < LCD_HEIGHT) /* refresh part below overlay, full width */
            _gray_rb->lcd_update_rect(0, y2, LCD_WIDTH, LCD_HEIGHT - y2);

        if (x1 > 0) /* refresh part to the left of overlay */
            _gray_rb->lcd_update_rect(0, y1, x1, y2 - y1);

        if (x2 < LCD_WIDTH) /* refresh part to the right of overlay */
            _gray_rb->lcd_update_rect(x2, y1, LCD_WIDTH - x2, y2 - y1);

        _graybuf->flags &= ~_GRAY_DEFERRED_UPDATE; /* clear request */
    }
}

/*---------------------------------------------------------------------------
 Initialize the framework
 ----------------------------------------------------------------------------
 Every framework needs such a function, and it has to be called as
 the very first one */
void gray_init(struct plugin_api* newrb)
{
    _gray_rb = newrb;
}

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
                     int bheight, int depth, int *buf_taken)
{
    int possible_depth, plane_size;
    int i, j, align;

    if ((unsigned) width > LCD_WIDTH
        || (unsigned) bheight > (LCD_HEIGHT/8)
        || depth < 1)
        return 0;

    /* the buffer has to be long aligned */
    align = 3 - (((unsigned long)gbuf + 3) & 3);
    gbuf += align;
    gbuf_size -= align;

    plane_size = MULU16(width, bheight);
    possible_depth = (gbuf_size - sizeof(_tGraybuf) - sizeof(long))
                     / (plane_size + sizeof(long));

    if (possible_depth < 1)
        return 0;

    depth = MIN(depth, 32);
    depth = MIN(depth, possible_depth);

    _graybuf = (_tGraybuf *) gbuf;  /* global pointer to buffer structure */
                                  
    _graybuf->x = 0;
    _graybuf->by = 0;
    _graybuf->width = width;
    _graybuf->height = bheight << 3;
    _graybuf->bheight = bheight;
    _graybuf->plane_size = plane_size;
    _graybuf->depth = depth;
    _graybuf->cur_plane = 0;
    _graybuf->flags = 0;
    _graybuf->bitpattern = (unsigned long *) (gbuf + sizeof(_tGraybuf));
    _graybuf->data = (unsigned char *) (_graybuf->bitpattern + depth + 1);

    i = depth - 1;
    j = 8;
    while (i != 0)
    {
        i >>= 1;
        j--;
    }
    _graybuf->randmask = 0xFFu >> j;

    /* initial state is all white */
    _gray_rb->memset(_graybuf->data, 0, MULU16(depth, plane_size));
    
    /* Precalculate the bit patterns for all possible pixel values */
    for (i = 0; i <= depth; i++)
    {
        unsigned long pattern = 0;
        int value = 0;

        for (j = 0; j < depth; j++)
        {
            pattern <<= 1;
            value += i;

            if (value >= depth)
                value -= depth;   /* "white" bit */
            else
                pattern |= 1;     /* "black" bit */
        }
        /* now the lower <depth> bits contain the pattern */

        _graybuf->bitpattern[i] = pattern;
    }
    
    _graybuf->fg_pattern = _graybuf->bitpattern[0]; /* black */
    _graybuf->bg_pattern = _graybuf->bitpattern[depth]; /* white */
    _graybuf->drawmode = GRAY_DRAW_SOLID;
    _graybuf->curfont = FONT_SYSFIXED;

    if (buf_taken)  /* caller requested info about space taken */
    {
        *buf_taken = sizeof(_tGraybuf) + sizeof(long)
                   + MULU16(plane_size + sizeof(long), depth) + align;
    }

    return depth;
}

/*---------------------------------------------------------------------------
 Release the grayscale display buffer
 ----------------------------------------------------------------------------
 Switches the grayscale overlay off at first if it is still running,
 then sets the pointer to NULL.
 DO CALL either this function or at least gray_show_display(false)
 before you exit, otherwise nasty things may happen.
 */
void gray_release_buffer(void)
{
    gray_show_display(false);
    _graybuf = NULL;
}

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
void gray_show_display(bool enable)
{
    if (enable)
    {
        _graybuf->flags |= _GRAY_RUNNING;
        _gray_rb->plugin_register_timer(FREQ / 67, 1, _timer_isr);
    }
    else
    {
        _gray_rb->plugin_unregister_timer();
        _graybuf->flags &= ~_GRAY_RUNNING;
        _gray_rb->lcd_update(); /* restore whatever there was before */
    }
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR

