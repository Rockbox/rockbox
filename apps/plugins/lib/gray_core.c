/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Greyscale framework
* Core & miscellaneous functions
*
* This is a generic framework to use grayscale display within Rockbox
* plugins. It obviously does not work for the player.
*
* Copyright (C) 2004-2005 Jens Arnold
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
struct _gray_info _gray_info;       /* global info structure */
short _gray_random_buffer;          /* buffer for random number generator */

/* Prototypes */
static void _timer_isr(void);

/* Timer interrupt handler: display next bitplane */
static void _timer_isr(void)
{
    _gray_rb->lcd_blit(_gray_info.plane_data + MULU16(_gray_info.plane_size,
                       _gray_info.cur_plane), _gray_info.x, _gray_info.by,
                       _gray_info.width, _gray_info.bheight, _gray_info.width);

    if (++_gray_info.cur_plane >= _gray_info.depth)
        _gray_info.cur_plane = 0;

    if (_gray_info.flags & _GRAY_DEFERRED_UPDATE)  /* lcd_update() requested? */
    {
        int x1 = MAX(_gray_info.x, 0);
        int x2 = MIN(_gray_info.x + _gray_info.width, LCD_WIDTH);
        int y1 = MAX(_gray_info.by << _PBLOCK_EXP, 0);
        int y2 = MIN((_gray_info.by << _PBLOCK_EXP) + _gray_info.height, LCD_HEIGHT);

        if (y1 > 0)  /* refresh part above overlay, full width */
            _gray_rb->lcd_update_rect(0, 0, LCD_WIDTH, y1);

        if (y2 < LCD_HEIGHT) /* refresh part below overlay, full width */
            _gray_rb->lcd_update_rect(0, y2, LCD_WIDTH, LCD_HEIGHT - y2);

        if (x1 > 0) /* refresh part to the left of overlay */
            _gray_rb->lcd_update_rect(0, y1, x1, y2 - y1);

        if (x2 < LCD_WIDTH) /* refresh part to the right of overlay */
            _gray_rb->lcd_update_rect(x2, y1, LCD_WIDTH - x2, y2 - y1);

        _gray_info.flags &= ~_GRAY_DEFERRED_UPDATE; /* clear request */
    }
}

/* Initialise the framework and prepare the greyscale display buffer

 arguments:
   newrb     = pointer to plugin api
   gbuf      = pointer to the memory area to use (e.g. plugin buffer)
   gbuf_size = max usable size of the buffer
   buffered  = use chunky pixel buffering with delta buffer?
               This allows to use all drawing functions, but needs more
               memory. Unbuffered operation provides only a subset of
               drawing functions. (only gray_bitmap drawing and scrolling)
   width     = width in pixels  (1..LCD_WIDTH)
   bheight   = height in LCD pixel-block units (8 pixels for b&w LCD,
               4 pixels for 4-grey LCD) (1..LCD_HEIGHT/block_size)
   depth     = number of bitplanes to use (1..32 for b&w LCD, 1..16 for
               4-grey LCD).

 result:
   = depth  if there was enough memory
   < depth  if there wasn't enough memory. The number of displayable
            shades is smaller than desired, but it still works
   = 0      if there wasn't even enough memory for 1 bitplane

   You can request any depth in the allowed range, not just powers of 2. The
   routine performs "graceful degradation" if the memory is not sufficient for
   the desired depth. As long as there is at least enough memory for 1 bitplane,
   it creates as many bitplanes as fit into memory, although 1 bitplane won't
   deliver an enhancement over the native display.
 
   The number of displayable shades is calculated as follows:
   b&w LCD:    shades = depth + 1
   4-grey LCD: shades = 3 * depth + 1

   If you need info about the memory taken by the greyscale buffer, supply a
   long* as the last parameter. This long will then contain the number of bytes
   used. The total memory needed can be calculated as follows:
 total_mem =
     shades * sizeof(long)               (bitpatterns)
   + (width * bheight) * depth           (bitplane data)
   + buffered ?                          (chunky front- & backbuffer)
       (width * bheight * 8(4) * 2) : 0
   + 0..3                                (longword alignment) */
int gray_init(struct plugin_api* newrb, unsigned char *gbuf, long gbuf_size,
              bool buffered, int width, int bheight, int depth, long *buf_taken)
{
    int possible_depth, i, j;
    long plane_size, buftaken;
    
    _gray_rb = newrb;

    if ((unsigned) width > LCD_WIDTH
        || (unsigned) bheight > (LCD_HEIGHT/_PBLOCK)
        || depth < 1)
        return 0;

    /* the buffer has to be long aligned */
    buftaken = (-(long)gbuf) & 3;
    gbuf += buftaken;

    /* chunky front- & backbuffer */
    if (buffered)  
    {
        plane_size = MULU16(width, bheight << _PBLOCK_EXP);
        buftaken += 2 * plane_size;
        if (buftaken > gbuf_size)
            return 0;

        _gray_info.cur_buffer = gbuf;
        gbuf += plane_size;
        /* set backbuffer to 0xFF to guarantee the initial full update */
        _gray_rb->memset(gbuf, 0xFF, plane_size);
        _gray_info.back_buffer = gbuf;
        gbuf += plane_size;
    }

    plane_size = MULU16(width, bheight);
    possible_depth = (gbuf_size - buftaken - sizeof(long))
                     / (plane_size + _LEVEL_FAC * sizeof(long));

    if (possible_depth < 1)
        return 0;

    depth = MIN(depth, _MAX_DEPTH);
    depth = MIN(depth, possible_depth);

    _gray_info.x = 0;
    _gray_info.by = 0;
    _gray_info.width = width;
    _gray_info.height = bheight << _PBLOCK_EXP;
    _gray_info.bheight = bheight;
    _gray_info.plane_size = plane_size;
    _gray_info.depth = depth;
    _gray_info.cur_plane = 0;
    _gray_info.flags = 0;
    _gray_info.plane_data = gbuf;
    gbuf += depth * plane_size;
    _gray_info.bitpattern = (unsigned long *)gbuf;
    buftaken += depth * plane_size
              + (_LEVEL_FAC * depth + 1) * sizeof(long);

    i = depth - 1;
    j = 8;
    while (i != 0)
    {
        i >>= 1;
        j--;
    }
    _gray_info.randmask = 0xFFu >> j;

    /* Precalculate the bit patterns for all possible pixel values */
#if LCD_DEPTH == 1
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

        _gray_info.bitpattern[i] = pattern;
    }
#elif LCD_DEPTH == 2
    for (i = 0; i < depth; i++)
    {
        unsigned long pattern = 0;
        int value = 0;

        for (j = 0; j < depth; j++)
        {
            pattern <<= 2;
            value += i;

            if (value >= depth)
                value -= depth;
            else
                pattern |= 3;
        }

        _gray_info.bitpattern[i] = pattern | (~pattern & 0xaaaaaaaa);
        _gray_info.bitpattern[i+depth] = (pattern & 0xaaaaaaaa) 
                                         | (~pattern & 0x55555555);
        _gray_info.bitpattern[i+2*depth] = pattern & 0x55555555;
    }
    _gray_info.bitpattern[3*depth] = 0;
#endif /* LCD_DEPTH */

    _gray_info.fg_brightness = 0;
    _gray_info.bg_brightness = _LEVEL_FAC * depth;
    _gray_info.drawmode = DRMODE_SOLID;
    _gray_info.curfont = FONT_SYSFIXED;

    if (buf_taken)  /* caller requested info about space taken */
        *buf_taken = buftaken;

    return depth;
}

/* Release the greyscale display buffer and the library
   DO CALL either this function or at least gray_show_display(false)
   before you exit, otherwise nasty things may happen. */
void gray_release(void)
{
    gray_show(false);
}

/* Switch the greyscale overlay on or off
   DO NOT call lcd_update() or any other api function that directly accesses
   the lcd while the greyscale overlay is running! If you need to do
   lcd_update() to update something outside the greyscale overlay area, use
   gray_deferred_update() instead.

 Other functions to avoid are:
   lcd_blit() (obviously), lcd_update_rect(), lcd_set_contrast(),
   lcd_set_invert_display(), lcd_set_flip(), lcd_roll() */
void gray_show(bool enable)
{
    if (enable)
    {
        _gray_info.flags |= _GRAY_RUNNING;
        _gray_rb->timer_register(1, NULL, FREQ / 67, 1, _timer_isr);
    }
    else
    {
        _gray_rb->timer_unregister();
        _gray_info.flags &= ~_GRAY_RUNNING;
        _gray_rb->lcd_update(); /* restore whatever there was before */
    }
}

/* Update a rectangular area of the greyscale overlay */
void gray_update_rect(int x, int y, int width, int height)
{
    int ymax;
    long srcofs;
    unsigned char *dst;

    if (width <= 0)
        return; /* nothing to do */

    /* The Y coordinates have to work on whole pixel block rows */
    ymax = (y + height - 1) >> _PBLOCK_EXP;
    y >>= _PBLOCK_EXP;

    if (x + width > _gray_info.width)
        width = _gray_info.width - x;
    if (ymax >= _gray_info.bheight)
        ymax = _gray_info.bheight - 1;
        
    srcofs = (y << _PBLOCK_EXP) + MULU16(_gray_info.height, x);
    dst = _gray_info.plane_data + MULU16(_gray_info.width, y) + x;
    
    /* Copy specified rectange bitmap to hardware */
    for (; y <= ymax; y++)
    {
        long srcofs_row = srcofs;
        unsigned char *dst_row = dst;
        unsigned char *dst_end = dst_row + width;

        do
        {
#if (CONFIG_CPU == SH7034) && (LCD_DEPTH == 1)
            unsigned mask;
            unsigned long change;
            unsigned long pat_stack[8];
            unsigned long *pat_ptr;
            unsigned char *end_addr;

            asm (     
                "mov.l   @(%3,%1),r1     \n"
                "mov.l   @(%3,%2),r2     \n"
                "xor     r1,r2           \n"
                "add     #4,%3           \n"
                "mov.l   @(%3,%1),r1     \n"
                "mov.l   @(%3,%2),%0     \n"
                "xor     r1,%0           \n"
                "or      r2,%0           \n"
                : /* outputs */
                /* %0 */ "=r"(change)
                : /* inputs */
                /* %1 */ "r"(_gray_info.cur_buffer),
                /* %2 */ "r"(_gray_info.back_buffer),
                /* %3 */ "z"(srcofs_row)
                : /* clobbers */
                 "r1", "r2"
            );

            if (change != 0)
            {
                pat_ptr = &pat_stack[8];

                /* precalculate the bit patterns with random shifts
                 * for all 8 pixels and put them on an extra "stack" */
                asm volatile (
                    "add     %5,%3       \n"
                    "add     %5,%4       \n"
                    "mov     #8,r3       \n"  /* loop count in r3: 8 pixels */

                ".ur_pre_loop:           \n"
                    "mov.b   @%3+,r0     \n"  /* read current buffer */
                    "mov.b   @%4,r1      \n"  /* read back buffer */
                    "mov     #0,r2       \n"  /* preset for skipped pixel */
                    "mov.b   r0,@%4      \n"  /* update back buffer */
                    "add     #1,%4       \n"
                    "cmp/eq  r0,r1       \n"  /* no change? */
                    "bt      .ur_skip    \n"  /* -> skip */

                    "shll2   r0          \n"  /* pixel value -> pattern offset */
                    "mov.l   @(r0,%7),r4 \n"  /* r4 = bitpattern[byte]; */

                    "mov     #75,r0      \n"
                    "mulu    r0,%0       \n"  /* multiply by 75 */
                    "sts     macl,%0     \n"
                    "add     #74,%0      \n"  /* add another 74 */
                    /* Since the lower bits are not very random: */
                    "swap.b  %0,r1       \n"  /* get bits 8..15 (need max. 5) */
                    "and     %8,r1       \n"  /* mask out unneeded bits */

                    "cmp/hs  %6,r1       \n"  /* random >= depth ? */
                    "bf      .ur_ntrim   \n"
                    "sub     %6,r1       \n"  /* yes: random -= depth; */
                ".ur_ntrim:              \n"
                
                    "mov.l   .ashlsi3,r0 \n"  /** rotate pattern **/
                    "jsr     @r0         \n"  /* r4 -> r0, shift left by r5 */
                    "mov     r1,r5       \n"

                    "mov     %6,r5       \n"
                    "sub     r1,r5       \n"  /* r5 = depth - r1 */
                    "mov.l   .lshrsi3,r1 \n"
                    "jsr     @r1         \n"  /* r4 -> r0, shift right by r5 */
                    "mov     r0,r2       \n"  /* store previous result in r2 */
                                         
                    "or      r0,r2       \n"  /* rotated_pattern = r2 | r0 */
                    "clrt                \n"  /* mask bit = 0 (replace) */

                ".ur_skip:               \n"  /* T == 1 if skipped */
                    "rotcr   %2          \n"  /* get mask bit */
                    "mov.l   r2,@-%1     \n"  /* push on pattern stack */

                    "add     #-1,r3      \n"  /* decrease loop count */
                    "cmp/pl  r3          \n"  /* loop count > 0? */
                    "bt      .ur_pre_loop\n"  /* yes: loop */
                    "shlr8   %2          \n"
                    "shlr16  %2          \n"
                    : /* outputs */
                    /* %0, in & out */ "+r"(_gray_random_buffer),
                    /* %1, in & out */ "+r"(pat_ptr),
                    /* %2 */ "=&r"(mask)
                    : /* inputs */
                    /* %3 */ "r"(_gray_info.cur_buffer),
                    /* %4 */ "r"(_gray_info.back_buffer),
                    /* %5 */ "2"(srcofs_row),
                    /* %6 */ "r"(_gray_info.depth),
                    /* %7 */ "r"(_gray_info.bitpattern),
                    /* %8 */ "r"(_gray_info.randmask)
                    : /* clobbers */
                    "r0", "r1", "r2", "r3", "r4", "r5", "macl", "pr"
                );

                end_addr = dst_row + MULU16(_gray_info.depth, _gray_info.plane_size);

                /* set the bits for all 8 pixels in all bytes according to the
                 * precalculated patterns on the pattern stack */
                asm (
                    "mov.l   @%3+,r1     \n"  /* pop all 8 patterns */
                    "mov.l   @%3+,r2     \n"
                    "mov.l   @%3+,r3     \n"
                    "mov.l   @%3+,r6     \n"
                    "mov.l   @%3+,r7     \n"
                    "mov.l   @%3+,r8     \n"
                    "mov.l   @%3+,r9     \n"
                    "mov.l   @%3+,r10    \n"

                    "tst     %4,%4       \n"  /* nothing to keep? */
                    "bt      .ur_sloop   \n"  /* yes: jump to short loop */

                ".ur_floop:              \n"  /** full loop (there are bits to keep)**/
                    "shlr    r1          \n"  /* rotate lsb of pattern 1 to t bit */
                    "rotcl   r0          \n"  /* rotate t bit into r0 */
                    "shlr    r2          \n"
                    "rotcl   r0          \n"
                    "shlr    r3          \n"
                    "rotcl   r0          \n"
                    "shlr    r6          \n"
                    "rotcl   r0          \n"
                    "shlr    r7          \n"
                    "rotcl   r0          \n"
                    "shlr    r8          \n"
                    "rotcl   r0          \n"
                    "shlr    r9          \n"
                    "rotcl   r0          \n"
                    "shlr    r10         \n"
                    "mov.b   @%0,%3      \n"  /* read old value */
                    "rotcl   r0          \n"
                    "and     %4,%3       \n"  /* mask out unneeded bits */
                    "or      r0,%3       \n"  /* set new bits */
                    "mov.b   %3,@%0      \n"  /* store value to bitplane */
                    "add     %2,%0       \n"  /* advance to next bitplane */
                    "cmp/hi  %0,%1       \n"  /* last bitplane done? */
                    "bt      .ur_floop   \n"  /* no: loop */

                    "bra     .ur_end     \n"
                    "nop                 \n"

                ".ur_sloop:              \n"  /** short loop (nothing to keep) **/
                    "shlr    r1          \n"  /* rotate lsb of pattern 1 to t bit */
                    "rotcl   r0          \n"  /* rotate t bit into r0 */
                    "shlr    r2          \n"
                    "rotcl   r0          \n"
                    "shlr    r3          \n"
                    "rotcl   r0          \n"
                    "shlr    r6          \n"
                    "rotcl   r0          \n"
                    "shlr    r7          \n"
                    "rotcl   r0          \n"
                    "shlr    r8          \n"
                    "rotcl   r0          \n"
                    "shlr    r9          \n"
                    "rotcl   r0          \n"
                    "shlr    r10         \n"
                    "rotcl   r0          \n"
                    "mov.b   r0,@%0      \n"  /* store byte to bitplane */
                    "add     %2,%0       \n"  /* advance to next bitplane */
                    "cmp/hi  %0,%1       \n"  /* last bitplane done? */
                    "bt      .ur_sloop   \n"  /* no: loop */

                    ".ur_end:                    \n"
                    : /* outputs */
                    : /* inputs */
                    /* %0 */ "r"(dst_row),
                    /* %1 */ "r"(end_addr),
                    /* %2 */ "r"(_gray_info.plane_size),
                    /* %3 */ "r"(pat_ptr),
                    /* %4 */ "r"(mask)
                    : /* clobbers */
                    "r0", "r1", "r2", "r3", "r6", "r7", "r8", "r9", "r10"
                );
            }
#endif
            srcofs_row += _gray_info.height;
            dst_row++;
        }
        while (dst_row < dst_end);
        
        srcofs += _PBLOCK;
        dst += _gray_info.width;
    }
}

#if CONFIG_CPU == SH7034
/* References to C library routines used in gray_update_rect() */
asm (
    ".align  2           \n"
".ashlsi3:               \n"  /* C library routine: */
    ".long   ___ashlsi3  \n"  /* shift r4 left by r5, return in r0 */
".lshrsi3:               \n"  /* C library routine: */
    ".long   ___lshrsi3  \n"  /* shift r4 right by r5, return in r0 */
    /* both routines preserve r4, destroy r5 and take ~16 cycles */
);
#endif

/* Update the whole greyscale overlay */
void gray_update(void)
{
    gray_update_rect(0, 0, _gray_info.width, _gray_info.height);
}

/* Do an lcd_update() to show changes done by rb->lcd_xxx() functions
   (in areas of the screen not covered by the greyscale overlay). */
void gray_deferred_lcd_update(void)
{
    if (_gray_info.flags & _GRAY_RUNNING)
        _gray_info.flags |= _GRAY_DEFERRED_UPDATE;
    else
        _gray_rb->lcd_update();
}

/*** Screenshot ***/

#define BMP_NUMCOLORS  (_MAX_DEPTH * _LEVEL_FAC + 1)
#define BMP_BPP        8
#define BMP_LINESIZE   ((LCD_WIDTH + 3) & ~3)
#define BMP_HEADERSIZE (54 + 4 * BMP_NUMCOLORS)
#define BMP_DATASIZE   (BMP_LINESIZE * LCD_HEIGHT)
#define BMP_TOTALSIZE  (BMP_HEADERSIZE + BMP_DATASIZE)

#define LE16_CONST(x) (x)&0xff, ((x)>>8)&0xff
#define LE32_CONST(x) (x)&0xff, ((x)>>8)&0xff, ((x)>>16)&0xff, ((x)>>24)&0xff

static const unsigned char bmpheader[] =
{
    0x42, 0x4d,                 /* 'BM' */
    LE32_CONST(BMP_TOTALSIZE),  /* Total file size */
    0x00, 0x00, 0x00, 0x00,     /* Reserved */
    LE32_CONST(BMP_HEADERSIZE), /* Offset to start of pixel data */

    0x28, 0x00, 0x00, 0x00,     /* Size of (2nd) header */
    LE32_CONST(LCD_WIDTH),      /* Width in pixels */
    LE32_CONST(LCD_HEIGHT),     /* Height in pixels */
    0x01, 0x00,                 /* Number of planes (always 1) */
    LE16_CONST(BMP_BPP),        /* Bits per pixel 1/4/8/16/24 */
    0x00, 0x00, 0x00, 0x00,     /* Compression mode, 0 = none */
    LE32_CONST(BMP_DATASIZE),   /* Size of bitmap data */
    0xc4, 0x0e, 0x00, 0x00,     /* Horizontal resolution (pixels/meter) */
    0xc4, 0x0e, 0x00, 0x00,     /* Vertical resolution (pixels/meter) */
    LE32_CONST(BMP_NUMCOLORS),  /* Number of used colours */
    LE32_CONST(BMP_NUMCOLORS),  /* Number of important colours */
};

#if LCD_DEPTH == 1
#define BMP_RED   0x90
#define BMP_GREEN 0xee
#define BMP_BLUE  0x90
#elif LCD_DEPTH == 2
#define BMP_RED   0xad
#define BMP_GREEN 0xd8
#define BMP_BLUE  0xe6
#endif

static unsigned char linebuf[BMP_LINESIZE];

/* Save the current display content (b&w and greyscale overlay) to an 8-bit
 * BMP file in the root directory. */
void gray_screendump(void)
{
    int fh, i, bright;
    int x, y, by, mask;
    int gx, gby;
    unsigned char *lcdptr, *grayptr, *grayptr2;
    char filename[MAX_PATH];

#ifdef HAVE_RTC
    struct tm *tm = _gray_rb->get_time();

    _gray_rb->snprintf(filename, MAX_PATH,
                       "/graydump %04d-%02d-%02d %02d-%02d-%02d.bmp",
                       tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                       tm->tm_hour, tm->tm_min, tm->tm_sec);
#else
    {
        DIR* dir;
        int max_dump_file = 1; /* default to graydump_0001.bmp */
        dir = _gray_rb->opendir("/");
        if (dir) /* found */
        {
            /* Search for the highest screendump filename present,
               increment behind that. So even with "holes"
               (deleted files), the newest will always have the
               highest number. */
            while(true)
            {   
                struct dirent* entry;
                int curr_dump_file;
                /* walk through the directory content */
                entry = _gray_rb->readdir(dir);
                if (!entry)
                {
                    _gray_rb->closedir(dir);
                    break; /* end of dir */
                }
                if (_gray_rb->strncasecmp(entry->d_name, "graydump_", 9))
                    continue; /* no screendump file */
                curr_dump_file = _gray_rb->atoi(&entry->d_name[9]);
                if (curr_dump_file >= max_dump_file)
                    max_dump_file = curr_dump_file + 1;
            }
        }
        _gray_rb->snprintf(filename, MAX_PATH, "/graydump_%04d.bmp",
                           max_dump_file);
    }
#endif

    fh = _gray_rb->creat(filename, O_WRONLY);

    if (fh < 0)
        return;
        
    _gray_rb->write(fh, bmpheader, sizeof(bmpheader));  /* write header */

    /* build clut, always 33 entries */
    linebuf[3] = 0;

    for (i = 0; i < BMP_NUMCOLORS; i++)
    {
        bright = MIN(i, _LEVEL_FAC * _gray_info.depth);
        linebuf[0] = MULU16(BMP_BLUE,  bright) / (_LEVEL_FAC * _gray_info.depth);
        linebuf[1] = MULU16(BMP_GREEN, bright) / (_LEVEL_FAC * _gray_info.depth);
        linebuf[2] = MULU16(BMP_RED,   bright) / (_LEVEL_FAC * _gray_info.depth);
        _gray_rb->write(fh, linebuf, 4);
    }

    /* 8-bit BMP image goes bottom -> top */
    for (y = LCD_HEIGHT - 1; y >= 0; y--)
    {
#if LCD_DEPTH == 1
        _gray_rb->memset(linebuf, BMP_NUMCOLORS-1, LCD_WIDTH);

        mask = 1 << (y & 7);
        by = y >> 3;
        lcdptr = _gray_rb->lcd_framebuffer + MULU16(LCD_WIDTH, by);
        gby = by - _gray_info.by;

        if ((_gray_info.flags & _GRAY_RUNNING)
            && (unsigned) gby < (unsigned) _gray_info.bheight)
        {   
            /* line contains greyscale (and maybe b&w) graphics */
            grayptr = _gray_info.plane_data + MULU16(_gray_info.width, gby);

            for (x = 0; x < LCD_WIDTH; x++)
            {
                if (*lcdptr++ & mask)
                    linebuf[x] = 0;
            
                gx = x - _gray_info.x;
                
                if ((unsigned)gx < (unsigned)_gray_info.width)
                {
                    bright = 0;
                    grayptr2 = grayptr + gx;

                    for (i = 0; i < _gray_info.depth; i++)
                    {
                        if (!(*grayptr2 & mask))
                            bright++;
                        grayptr2 += _gray_info.plane_size;
                    }
                    linebuf[x] = bright;
                }
            }
        }
        else  
        {   
            /* line contains only b&w graphics */
            for (x = 0; x < LCD_WIDTH; x++)
                if (*lcdptr++ & mask)
                    linebuf[x] = 0;
        }
#endif

        _gray_rb->write(fh, linebuf, sizeof(linebuf));
    }

    _gray_rb->close(fh);
}

#endif /* HAVE_LCD_BITMAP */
#endif /* !SIMULATOR */

