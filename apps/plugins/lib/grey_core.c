/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* New greyscale framework
* Core & miscellaneous functions
*
* This is a generic framework to display 129 shades of grey on low-depth
* bitmap LCDs (Archos b&w, Iriver & Ipod 4-grey) within plugins.
*
* Copyright (C) 2008 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "plugin.h"
#include "grey.h"

#if defined(HAVE_ADJUSTABLE_CPU_FREQ) && \
    (defined(CPU_PP) || (CONFIG_LCD == LCD_TL0350A))
#define NEED_BOOST
#endif

#ifndef SIMULATOR

#if CONFIG_LCD == LCD_SSD1815 || CONFIG_LCD == LCD_IFP7XX \
 || CONFIG_LCD == LCD_MROBE100
/* measured and interpolated curve */
/* TODO: check for iFP & m:robe 100 */
static const unsigned char lcdlinear[256] = {
      0,   3,   5,   8,  11,  13,  16,  18,
     21,  23,  26,  28,  31,  33,  36,  38,
     40,  42,  45,  47,  49,  51,  53,  55,
     57,  59,  60,  62,  64,  66,  67,  69,
     70,  72,  73,  74,  76,  77,  78,  79,
     81,  82,  83,  84,  85,  86,  87,  88,
     88,  89,  90,  91,  92,  92,  93,  94,
     95,  95,  96,  97,  97,  98,  99,  99,
    100, 101, 102, 102, 103, 104, 104, 105,
    106, 106, 107, 107, 108, 109, 109, 110,
    111, 111, 112, 113, 113, 114, 114, 115,
    116, 116, 117, 117, 118, 119, 119, 120,
    120, 121, 121, 122, 122, 123, 123, 124,
    124, 125, 125, 126, 126, 127, 127, 128,
    128, 128, 129, 129, 130, 130, 131, 131,
    132, 132, 133, 133, 133, 134, 134, 135,
    135, 136, 136, 137, 137, 138, 138, 138,
    139, 139, 140, 140, 141, 141, 142, 142,
    143, 143, 144, 144, 145, 145, 146, 146,
    147, 147, 148, 148, 148, 149, 149, 150,
    150, 151, 151, 152, 152, 153, 153, 153,
    154, 154, 155, 155, 156, 156, 157, 157,
    158, 158, 158, 159, 159, 160, 160, 161,
    161, 162, 162, 163, 163, 164, 164, 165,
    165, 166, 167, 167, 168, 168, 169, 169,
    170, 171, 171, 172, 173, 173, 174, 175,
    176, 176, 177, 178, 179, 180, 181, 181,
    182, 183, 184, 185, 186, 188, 189, 190,
    191, 192, 194, 195, 196, 198, 199, 201,
    202, 204, 205, 207, 209, 211, 213, 215,
    217, 219, 222, 224, 226, 229, 231, 234,
    236, 239, 242, 244, 247, 250, 252, 255
};
#elif CONFIG_LCD == LCD_S1D15E06
/* measured and interpolated curve */
static const unsigned char lcdlinear[256] = {
      0,   5,  11,  16,  21,  27,  32,  37,
     42,  47,  51,  56,  60,  64,  68,  72,
     75,  78,  81,  84,  87,  89,  91,  93,
     95,  96,  98,  99, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 111,
    112, 113, 113, 114, 115, 115, 116, 117,
    117, 118, 118, 119, 119, 120, 120, 121,
    121, 122, 122, 123, 123, 124, 124, 125,
    125, 126, 126, 127, 127, 127, 128, 128,
    129, 129, 130, 130, 131, 131, 132, 132,
    133, 133, 134, 134, 135, 135, 136, 136,
    137, 137, 138, 138, 138, 139, 139, 140,
    140, 141, 141, 141, 142, 142, 143, 143,
    143, 144, 144, 145, 145, 145, 146, 146,
    146, 147, 147, 147, 148, 148, 149, 149,
    149, 150, 150, 150, 151, 151, 151, 152,
    152, 153, 153, 153, 154, 154, 155, 155,
    155, 156, 156, 157, 157, 157, 158, 158,
    159, 159, 159, 160, 160, 161, 161, 162,
    162, 162, 163, 163, 164, 164, 164, 165,
    165, 166, 166, 167, 167, 167, 168, 168,
    169, 169, 170, 170, 170, 171, 171, 172,
    172, 173, 173, 174, 174, 175, 175, 176,
    176, 177, 177, 178, 178, 179, 179, 180,
    180, 181, 182, 182, 183, 184, 184, 185,
    186, 186, 187, 188, 188, 189, 190, 191,
    191, 192, 193, 194, 195, 196, 196, 197,
    198, 199, 200, 201, 202, 203, 204, 205,
    206, 207, 208, 209, 210, 211, 213, 214,
    215, 216, 218, 219, 220, 222, 223, 225,
    227, 228, 230, 232, 233, 235, 237, 239,
    241, 243, 245, 247, 249, 251, 253, 255
};
#elif (CONFIG_LCD == LCD_IPOD2BPP) || (CONFIG_LCD == LCD_IPODMINI)
/* measured and interpolated curve for mini LCD */
/* TODO: verify this curve on the fullsize greyscale LCD */
static const unsigned char lcdlinear[256] = {
      0,   3,   6,   8,  11,  14,  17,  19,
     22,  24,  27,  29,  32,  34,  36,  38,
     40,  42,  44,  45,  47,  48,  50,  51,
     52,  54,  55,  56,  57,  58,  58,  59,
     60,  61,  62,  62,  63,  64,  64,  65,
     66,  66,  67,  67,  68,  68,  69,  69,
     70,  70,  70,  71,  71,  71,  72,  72,
     73,  73,  73,  74,  74,  74,  74,  75,
     75,  75,  76,  76,  76,  77,  77,  77,
     78,  78,  78,  79,  79,  79,  80,  80,
     80,  80,  81,  81,  81,  82,  82,  82,
     83,  83,  83,  84,  84,  84,  85,  85,
     85,  85,  86,  86,  86,  87,  87,  87,
     87,  88,  88,  88,  89,  89,  89,  89,
     90,  90,  90,  91,  91,  91,  92,  92,
     92,  93,  93,  93,  94,  94,  94,  95,
     95,  96,  96,  96,  97,  97,  98,  98,
     99,  99,  99, 100, 100, 101, 101, 102,
    102, 103, 103, 104, 104, 105, 105, 106,
    106, 107, 107, 108, 108, 109, 109, 110,
    110, 111, 111, 112, 113, 113, 114, 114,
    115, 115, 116, 117, 117, 118, 118, 119,
    120, 120, 121, 122, 122, 123, 124, 124,
    125, 126, 126, 127, 128, 128, 129, 130,
    131, 131, 132, 133, 134, 134, 135, 136,
    137, 138, 139, 140, 141, 142, 143, 144,
    145, 146, 147, 148, 149, 150, 152, 153,
    154, 156, 157, 159, 160, 162, 163, 165,
    167, 168, 170, 172, 174, 176, 178, 180,
    182, 184, 187, 189, 192, 194, 197, 200,
    203, 206, 209, 212, 215, 219, 222, 226,
    229, 233, 236, 240, 244, 248, 251, 255
};
#elif CONFIG_LCD == LCD_TL0350A
/* measured and interpolated curve for iaudio remote */
static const unsigned char lcdlinear[256] = {
      5,   9,  13,  17,  21,  25,  29,  33,
     36,  39,  42,  45,  48,  51,  54,  57,
     60,  62,  65,  67,  70,  72,  75,  77,
     80,  82,  84,  86,  87,  89,  91,  93,
     94,  95,  96,  97,  97,  98,  99,  99,
    100, 100, 101, 102, 103, 103, 104, 105,
    106, 106, 107, 108, 108, 109, 110, 111,
    112, 112, 113, 113, 114, 114, 115, 115,
    116, 116, 117, 117, 118, 118, 119, 119,
    120, 120, 121, 121, 122, 122, 123, 123,
    124, 124, 124, 125, 125, 126, 126, 126,
    127, 127, 127, 128, 128, 129, 129, 129,
    130, 130, 131, 131, 132, 132, 133, 133,
    134, 134, 135, 135, 136, 136, 137, 137,
    138, 138, 139, 139, 140, 140, 141, 141,
    142, 142, 143, 143, 144, 144, 145, 145,
    146, 146, 147, 147, 148, 149, 149, 150,
    151, 151, 152, 152, 153, 154, 154, 155,
    156, 156, 157, 157, 158, 159, 159, 160,
    161, 161, 162, 163, 164, 164, 165, 166,
    167, 167, 168, 169, 170, 170, 171, 172,
    173, 173, 174, 175, 176, 176, 177, 178,
    179, 179, 180, 181, 182, 182, 183, 184,
    185, 186, 187, 188, 188, 189, 191, 191,
    192, 193, 194, 195, 195, 196, 197, 198,
    199, 200, 201, 202, 203, 204, 205, 206,
    207, 208, 209, 210, 211, 212, 213, 214,
    215, 216, 217, 218, 219, 220, 222, 223,
    224, 225, 226, 227, 228, 229, 230, 231,
    232, 233, 234, 235, 236, 237, 238, 239,
    240, 240, 241, 242, 243, 243, 244, 245,
    246, 246, 247, 248, 249, 250, 251, 252
};
#endif
#else /* SIMULATOR */
/* undo a (generic) PC display gamma of 2.0 to simulate target behaviour */
static const unsigned char lcdlinear[256] = {
      0,  16,  23,  28,  32,  36,  39,  42,
     45,  48,  50,  53,  55,  58,  60,  62,
     64,  66,  68,  70,  71,  73,  75,  77,
     78,  80,  81,  83,  84,  86,  87,  89,
     90,  92,  93,  94,  96,  97,  98, 100,
    101, 102, 103, 105, 106, 107, 108, 109,
    111, 112, 113, 114, 115, 116, 117, 118,
    119, 121, 122, 123, 124, 125, 126, 127,
    128, 129, 130, 131, 132, 133, 134, 135,
    135, 136, 137, 138, 139, 140, 141, 142,
    143, 144, 145, 145, 146, 147, 148, 149,
    150, 151, 151, 152, 153, 154, 155, 156,
    156, 157, 158, 159, 160, 160, 161, 162,
    163, 164, 164, 165, 166, 167, 167, 168,
    169, 170, 170, 171, 172, 173, 173, 174,
    175, 176, 176, 177, 178, 179, 179, 180,
    181, 181, 182, 183, 183, 184, 185, 186,
    186, 187, 188, 188, 189, 190, 190, 191,
    192, 192, 193, 194, 194, 195, 196, 196,
    197, 198, 198, 199, 199, 200, 201, 201,
    202, 203, 203, 204, 204, 205, 206, 206,
    207, 208, 208, 209, 209, 210, 211, 211,
    212, 212, 213, 214, 214, 215, 215, 216,
    217, 217, 218, 218, 219, 220, 220, 221,
    221, 222, 222, 223, 224, 224, 225, 225,
    226, 226, 227, 228, 228, 229, 229, 230,
    230, 231, 231, 232, 233, 233, 234, 234,
    235, 235, 236, 236, 237, 237, 238, 238,
    239, 240, 240, 241, 241, 242, 242, 243,
    243, 244, 244, 245, 245, 246, 246, 247,
    247, 248, 248, 249, 249, 250, 250, 251,
    251, 252, 252, 253, 253, 254, 254, 255
};
#endif /* SIMULATOR */

/* Prototypes */
static inline void _deferred_update(void) __attribute__ ((always_inline));
static int exp_s16p16(int x);
static int log_s16p16(int x);
static void grey_screendump_hook(int fd);
#ifdef SIMULATOR
static unsigned long _grey_get_pixel(int x, int y);
#else
static void _timer_isr(void);
#endif

/* Update LCD areas not covered by the greyscale overlay */
static inline void _deferred_update(void)
{
    int x1 = MAX(_grey_info.x, 0);
    int x2 = MIN(_grey_info.x + _grey_info.width, LCD_WIDTH);
    int y1 = MAX(_grey_info.y, 0);
    int y2 = MIN(_grey_info.y + _grey_info.height, LCD_HEIGHT);

    if (y1 > 0)  /* refresh part above overlay, full width */
        _grey_info.rb->lcd_update_rect(0, 0, LCD_WIDTH, y1);

    if (y2 < LCD_HEIGHT) /* refresh part below overlay, full width */
        _grey_info.rb->lcd_update_rect(0, y2, LCD_WIDTH, LCD_HEIGHT - y2);

    if (x1 > 0) /* refresh part to the left of overlay */
        _grey_info.rb->lcd_update_rect(0, y1, x1, y2 - y1);

    if (x2 < LCD_WIDTH) /* refresh part to the right of overlay */
        _grey_info.rb->lcd_update_rect(x2, y1, LCD_WIDTH - x2, y2 - y1);
}

#ifdef SIMULATOR

/* Callback function for grey_ub_gray_bitmap_part() to read a pixel from the
 * greybuffer. Note that x and y are in LCD coordinates, not greybuffer
 * coordinates! */
static unsigned long _grey_get_pixel(int x, int y)
{
    int xg = x - _grey_info.x;
    int yg = y - _grey_info.y;
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
    int idx = _grey_info.width * yg + xg;
#else /* vertical packing or vertical interleaved */
    int idx = _grey_info.width * (yg & ~_GREY_BMASK) 
            + (xg << _GREY_BSHIFT) + (~yg & _GREY_BMASK);
#endif

    return _grey_info.values[idx] + (1 << LCD_DEPTH);
}

#else /* !SIMULATOR */

/* Timer interrupt handler: display next frame */
static void _timer_isr(void)
{
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
    _grey_info.rb->lcd_blit_grey_phase(_grey_info.values, _grey_info.phases,
                                       _grey_info.bx, _grey_info.y,
                                       _grey_info.bwidth, _grey_info.height,
                                       _grey_info.width);
#else /* vertical packing or vertical interleaved */
    _grey_info.rb->lcd_blit_grey_phase(_grey_info.values, _grey_info.phases,
                                       _grey_info.x, _grey_info.by,
                                       _grey_info.width, _grey_info.bheight,
                                       _grey_info.width);
#endif

    if (_grey_info.flags & _GREY_DEFERRED_UPDATE) /* lcd_update() requested? */
    {
        _deferred_update();
        _grey_info.flags &= ~_GREY_DEFERRED_UPDATE; /* clear request */
    }
}

#endif /* !SIMULATOR */

/* fixed point exp() */
static int exp_s16p16(int x)
{
    int t;
    int y = 0x00010000;

    if (x < 0) x += 0xb1721,            y >>= 16;
    t = x - 0x58b91; if (t >= 0) x = t, y <<= 8;
    t = x - 0x2c5c8; if (t >= 0) x = t, y <<= 4;
    t = x - 0x162e4; if (t >= 0) x = t, y <<= 2;
    t = x - 0x0b172; if (t >= 0) x = t, y <<= 1;
    t = x - 0x067cd; if (t >= 0) x = t, y += y >> 1;
    t = x - 0x03920; if (t >= 0) x = t, y += y >> 2;
    t = x - 0x01e27; if (t >= 0) x = t, y += y >> 3;
    t = x - 0x00f85; if (t >= 0) x = t, y += y >> 4;
    t = x - 0x007e1; if (t >= 0) x = t, y += y >> 5;
    t = x - 0x003f8; if (t >= 0) x = t, y += y >> 6;
    t = x - 0x001fe; if (t >= 0) x = t, y += y >> 7;
    y += ((y >> 8) * x) >> 8;

    return y;
}

/* fixed point log() */
static int log_s16p16(int x)
{
    int t;
    int y = 0xa65af;

    if (x < 0x00008000) x <<=16,                        y -= 0xb1721;
    if (x < 0x00800000) x <<= 8,                        y -= 0x58b91;
    if (x < 0x08000000) x <<= 4,                        y -= 0x2c5c8;
    if (x < 0x20000000) x <<= 2,                        y -= 0x162e4;
    if (x < 0x40000000) x <<= 1,                        y -= 0x0b172;
    t = x + (x >> 1); if ((t & 0x80000000) == 0) x = t, y -= 0x067cd;
    t = x + (x >> 2); if ((t & 0x80000000) == 0) x = t, y -= 0x03920;
    t = x + (x >> 3); if ((t & 0x80000000) == 0) x = t, y -= 0x01e27;
    t = x + (x >> 4); if ((t & 0x80000000) == 0) x = t, y -= 0x00f85;
    t = x + (x >> 5); if ((t & 0x80000000) == 0) x = t, y -= 0x007e1;
    t = x + (x >> 6); if ((t & 0x80000000) == 0) x = t, y -= 0x003f8;
    t = x + (x >> 7); if ((t & 0x80000000) == 0) x = t, y -= 0x001fe;
    x = 0x80000000 - x;
    y -= x >> 15;

    return y;
}

/* Initialise the framework and prepare the greyscale display buffer

 arguments:
   newrb     = pointer to plugin api
   gbuf      = pointer to the memory area to use (e.g. plugin buffer)
   gbuf_size = max usable size of the buffer
   features  = flags for requesting features
               GREY_BUFFERED: use chunky pixel buffering
               This allows to use all drawing functions, but needs more
               memory. Unbuffered operation provides only a subset of
               drawing functions. (only grey_bitmap drawing and scrolling)
               GREY_RAWMAPPED: no LCD linearisation and gamma correction
   width     = width in pixels  (1..LCD_WIDTH)
   height    = height in pixels (1..LCD_HEIGHT)
               Note that depending on the target LCD, either height or
               width are rounded up to a multiple of 4 or 8.

 result:
   true on success, false on failure

   If you need info about the memory taken by the greyscale buffer, supply a
   long* as the last parameter. This long will then contain the number of bytes
   used. The total memory needed can be calculated as follows:
 total_mem =
     width * height * 2               [grey display data]
   + buffered ? (width * height) : 0  [chunky buffer]
   + 0..3                             [alignment]

   The function is authentic regarding memory usage on the simulator, even
   if it doesn't use all of the allocated memory. */
bool grey_init(struct plugin_api* newrb, unsigned char *gbuf, long gbuf_size,
               unsigned features, int width, int height, long *buf_taken)
{
    int bdim, i;
    long plane_size, buftaken;
    unsigned data;
#ifndef SIMULATOR
    unsigned *dst, *end;
#endif

    _grey_info.rb = newrb;

    if ((unsigned) width > LCD_WIDTH
        || (unsigned) height > LCD_HEIGHT)
        return false;

#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
    bdim = (width + 7) >> 3;
    width = bdim << 3;
#else /* vertical packing or vertical interleaved */
#if (LCD_DEPTH == 1) || (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED)
    bdim = (height + 7) >> 3;
    height = bdim << 3;
#elif LCD_DEPTH == 2
    bdim = (height + 3) >> 2;
    height = bdim << 2;
#endif
#endif

    plane_size = _GREY_MULUQ(width, height);
#ifdef CPU_COLDFIRE
    plane_size += (-plane_size) & 0xf; /* All buffers should be line aligned */
    buftaken    = (-(long)gbuf) & 0xf;
#else
    buftaken    = (-(long)gbuf) & 3;   /* All buffers must be long aligned. */
#endif
    gbuf += buftaken;

    if (features & GREY_BUFFERED) /* chunky buffer */
    {
        _grey_info.buffer = gbuf;
        gbuf     += plane_size;
        buftaken += plane_size;
    }
    _grey_info.values = gbuf;
    gbuf += plane_size;
    _grey_info.phases = gbuf;
    buftaken += 2 * plane_size;

    if (buftaken > gbuf_size)
        return false;

    /* Init to white */
    _grey_info.rb->memset(_grey_info.values, 0x80, plane_size);

#ifndef SIMULATOR
    /* Init phases with random bits */
    dst = (unsigned*)(_grey_info.phases);
    end = (unsigned*)(_grey_info.phases + plane_size);

    do
        *dst++ = _grey_info.rb->rand();
    while (dst < end);
#endif

    _grey_info.x = 0;
    _grey_info.y = 0;
    _grey_info.width = width;
    _grey_info.height = height;
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
    _grey_info.bx = 0;
    _grey_info.bwidth = bdim;
#else /* vertical packing or vertical interleaved */
    _grey_info.by = 0;
    _grey_info.bheight = bdim;
#endif
    _grey_info.flags = 0;
    _grey_info.fg_brightness = 0;
    _grey_info.bg_brightness = 255;
    _grey_info.drawmode = DRMODE_SOLID;
    _grey_info.curfont = FONT_SYSFIXED;

    /* precalculate the value -> pattern index conversion table, taking
       linearisation and gamma correction into account */
    if (features & GREY_RAWMAPPED)
        for (i = 0; i < 256; i++)
        {
            data = i << 7;
            _grey_info.gvalue[i] = (data + (data >> 8)) >> 8;
        }
    else
        for (i = 0; i < 256; i++)
        {
            data = exp_s16p16(((2<<8) * log_s16p16(i * 257 + 1)) >> 8) + 128;
            data = (data - (data >> 8)) >> 8; /* approx. data /= 257 */
            data = (lcdlinear[data] << 7) + 127;
            _grey_info.gvalue[i] = (data + (data >> 8)) >> 8;
                                          /* approx. data / 255 */
        }

    if (buf_taken)  /* caller requested info about space taken */
        *buf_taken = buftaken;

    return true;
}

/* Release the greyscale display buffer and the library
   DO CALL either this function or at least grey_show_display(false)
   before you exit, otherwise nasty things may happen. */
void grey_release(void)
{
    grey_show(false);
}

/* Switch the greyscale overlay on or off
   DO NOT call lcd_update() or any other api function that directly accesses
   the lcd while the greyscale overlay is running! If you need to do
   lcd_update() to update something outside the greyscale overlay area, use
   grey_deferred_update() instead.

 Other functions to avoid are:
   lcd_blit_mono(), lcd_update_rect(), lcd_set_contrast(),
   lcd_set_invert_display(), lcd_set_flip() */
void grey_show(bool enable)
{
    if (enable && !(_grey_info.flags & _GREY_RUNNING))
    {
        _grey_info.flags |= _GREY_RUNNING;
#ifdef SIMULATOR
        _grey_info.rb->sim_lcd_ex_init(129, _grey_get_pixel);
        _grey_info.rb->sim_lcd_ex_update_rect(_grey_info.x, _grey_info.y,
                                              _grey_info.width, _grey_info.height);
#else /* !SIMULATOR */
#ifdef NEED_BOOST
        _grey_info.rb->cpu_boost(true);
#endif
#if CONFIG_LCD == LCD_SSD1815
        _grey_info.rb->timer_register(1, NULL, TIMER_FREQ / 67, 1, _timer_isr);
#elif CONFIG_LCD == LCD_S1D15E06
        _grey_info.rb->timer_register(1, NULL, TIMER_FREQ / 70, 1, _timer_isr);
#elif CONFIG_LCD == LCD_IPOD2BPP
#ifdef IPOD_1G2G
        _grey_info.rb->timer_register(1, NULL, TIMER_FREQ / 96, 1, _timer_isr); /* verified */
#elif defined IPOD_3G
        _grey_info.rb->timer_register(1, NULL, TIMER_FREQ / 87, 1, _timer_isr); /* verified */
#else
        /* FIXME: verify value */
        _grey_info.rb->timer_register(1, NULL, TIMER_FREQ / 80, 1, _timer_isr);
#endif
#elif CONFIG_LCD == LCD_IPODMINI
        _grey_info.rb->timer_register(1, NULL, TIMER_FREQ / 88, 1, _timer_isr);
#elif CONFIG_LCD == LCD_IFP7XX
        _grey_info.rb->timer_register(1, NULL, TIMER_FREQ / 83, 1, _timer_isr);
#elif CONFIG_LCD == LCD_MROBE100
        _grey_info.rb->timer_register(1, NULL, TIMER_FREQ / 83, 1, _timer_isr); /* not calibrated/tested */
#elif CONFIG_LCD == LCD_TL0350A
        _grey_info.rb->timer_register(1, NULL, TIMER_FREQ / 75, 1, _timer_isr); /* verified */
        /* This is half of the actual frame frequency, but 150Hz is too much */
#endif /* CONFIG_LCD */
#endif /* !SIMULATOR */
        _grey_info.rb->screen_dump_set_hook(grey_screendump_hook);
    }
    else if (!enable && (_grey_info.flags & _GREY_RUNNING))
    {
#ifdef SIMULATOR
        _grey_info.rb->sim_lcd_ex_init(0, NULL);
#else
        _grey_info.rb->timer_unregister();
#ifdef NEED_BOOST
        _grey_info.rb->cpu_boost(false);
#endif
#endif
        _grey_info.flags &= ~_GREY_RUNNING;
        _grey_info.rb->screen_dump_set_hook(NULL);
        _grey_info.rb->lcd_update(); /* restore whatever there was before */
    }
}

void grey_update_rect(int x, int y, int width, int height)
{
    grey_ub_gray_bitmap_part(_grey_info.buffer, x, y, _grey_info.width,
                             x, y, width, height);
}

/* Update the whole greyscale overlay */
void grey_update(void)
{
    grey_ub_gray_bitmap_part(_grey_info.buffer, 0, 0, _grey_info.width,
                             0, 0, _grey_info.width, _grey_info.height);
}

/* Do an lcd_update() to show changes done by rb->lcd_xxx() functions
   (in areas of the screen not covered by the greyscale overlay). */
void grey_deferred_lcd_update(void)
{
    if (_grey_info.flags & _GREY_RUNNING)
    {
#ifdef SIMULATOR
        _deferred_update();
#else
        _grey_info.flags |= _GREY_DEFERRED_UPDATE;
#endif
    }
    else
        _grey_info.rb->lcd_update();
}

/*** Screenshot ***/

#define BMP_FIXEDCOLORS (1 << LCD_DEPTH)
#define BMP_VARCOLORS   129
#define BMP_NUMCOLORS   (BMP_FIXEDCOLORS + BMP_VARCOLORS)
#define BMP_BPP         8
#define BMP_LINESIZE    ((LCD_WIDTH + 3) & ~3)
#define BMP_HEADERSIZE  (54 + 4 * BMP_NUMCOLORS)
#define BMP_DATASIZE    (BMP_LINESIZE * LCD_HEIGHT)
#define BMP_TOTALSIZE   (BMP_HEADERSIZE + BMP_DATASIZE)

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

    /* Fixed colours */
#if LCD_DEPTH == 1
    0x90, 0xee, 0x90, 0x00,     /* Colour #0 */
    0x00, 0x00, 0x00, 0x00      /* Colour #1 */
#elif LCD_DEPTH == 2
    0xe6, 0xd8, 0xad, 0x00,     /* Colour #0 */
    0x99, 0x90, 0x73, 0x00,     /* Colour #1 */
    0x4c, 0x48, 0x39, 0x00,     /* Colour #2 */
    0x00, 0x00, 0x00, 0x00      /* Colour #3 */
#endif
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

/* Hook function for core screen_dump() to save the current display
   content (b&w and greyscale overlay) to an 8-bit BMP file. */
static void grey_screendump_hook(int fd)
{
    int i;
    int x, y, gx, gy;
#if LCD_PIXELFORMAT == VERTICAL_PACKING
#if LCD_DEPTH == 1
    unsigned mask;
#elif LCD_DEPTH == 2
    int shift;
#endif
#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
    unsigned data;
    int shift;
#endif /* LCD_PIXELFORMAT */
    fb_data *lcdptr;
    unsigned char *clut_entry;
    unsigned char linebuf[MAX(4*BMP_VARCOLORS,BMP_LINESIZE)];

    _grey_info.rb->write(fd, bmpheader, sizeof(bmpheader));  /* write header */

    /* build clut */
    _grey_info.rb->memset(linebuf, 0, 4*BMP_VARCOLORS);
    clut_entry = linebuf;

    for (i = 0; i <= 128; i++)
    {
        *clut_entry++ = _GREY_MULUQ(BMP_BLUE,  i) >> 7;
        *clut_entry++ = _GREY_MULUQ(BMP_GREEN, i) >> 7;
        *clut_entry++ = _GREY_MULUQ(BMP_RED,   i) >> 7;
        clut_entry++;
    }
    _grey_info.rb->write(fd, linebuf, 4*BMP_VARCOLORS);

    /* BMP image goes bottom -> top */
    for (y = LCD_HEIGHT - 1; y >= 0; y--)
    {
        _grey_info.rb->memset(linebuf, 0, BMP_LINESIZE);

        gy = y - _grey_info.y;
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
#if LCD_DEPTH == 2
        lcdptr = _grey_info.rb->lcd_framebuffer + _GREY_MULUQ(LCD_FBWIDTH, y);

        for (x = 0; x < LCD_WIDTH; x += 4)
        {
            gx = x - _grey_info.x;

            if (((unsigned)gy < (unsigned)_grey_info.height)
                && ((unsigned)gx < (unsigned)_grey_info.width))
            {
                unsigned char *src = _grey_info.values
                                   + _GREY_MULUQ(_grey_info.width, gy) + gx;
                for (i = 0; i < 4; i++)
                    linebuf[x + i] = BMP_FIXEDCOLORS + *src++;
            }
            else
            {
                unsigned data = *lcdptr;
                linebuf[x]     = (data >> 6) & 3;
                linebuf[x + 1] = (data >> 4) & 3;
                linebuf[x + 2] = (data >> 2) & 3;
                linebuf[x + 3] = data & 3;
            }
            lcdptr++;
        }
#endif /* LCD_DEPTH */
#elif LCD_PIXELFORMAT == VERTICAL_PACKING
#if LCD_DEPTH == 1
        mask = 1 << (y & 7);
        lcdptr = _grey_info.rb->lcd_framebuffer + _GREY_MULUQ(LCD_WIDTH, y >> 3);

        for (x = 0; x < LCD_WIDTH; x++)
        {
            gx = x - _grey_info.x;

            if (((unsigned)gy < (unsigned)_grey_info.height)
                && ((unsigned)gx < (unsigned)_grey_info.width))
            {
                linebuf[x] = BMP_FIXEDCOLORS
                           + _grey_info.values[_GREY_MULUQ(_grey_info.width,
                                                           gy & ~_GREY_BMASK)
                                               + (gx << _GREY_BSHIFT)
                                               + (~gy & _GREY_BMASK)];
            }
            else
            {
                linebuf[x] = (*lcdptr & mask) ? 1 : 0;
            }
            lcdptr++;
        }
#elif LCD_DEPTH == 2
        shift = 2 * (y & 3);
        lcdptr = _grey_info.rb->lcd_framebuffer + _GREY_MULUQ(LCD_WIDTH, y >> 2);

        for (x = 0; x < LCD_WIDTH; x++)
        {
            gx = x - _grey_info.x;

            if (((unsigned)gy < (unsigned)_grey_info.height)
                && ((unsigned)gx < (unsigned)_grey_info.width))
            {
                linebuf[x] = BMP_FIXEDCOLORS
                           + _grey_info.values[_GREY_MULUQ(_grey_info.width,
                                                           gy & ~_GREY_BMASK)
                                               + (gx << _GREY_BSHIFT)
                                               + (~gy & _GREY_BMASK)];
            }
            else
            {
                linebuf[x] = (*lcdptr >> shift) & 3;
            }
            lcdptr++;
        }
#endif /* LCD_DEPTH */
#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED
#if LCD_DEPTH == 2
        shift = y & 7;
        lcdptr = _grey_info.rb->lcd_framebuffer + _GREY_MULUQ(LCD_WIDTH, y >> 3);

        for (x = 0; x < LCD_WIDTH; x++)
        {
            gx = x - _grey_info.x;

            if (((unsigned)gy < (unsigned)_grey_info.height)
                && ((unsigned)gx < (unsigned)_grey_info.width))
            {
                linebuf[x] = BMP_FIXEDCOLORS
                           + _grey_info.values[_GREY_MULUQ(_grey_info.width,
                                                           gy & ~_GREY_BMASK)
                                               + (gx << _GREY_BSHIFT)
                                               + (~gy & _GREY_BMASK)];
            }
            else
            {
                data = (*lcdptr >> shift) & 0x0101;
                linebuf[x] = ((data >> 7) | data) & 3;
            }
            lcdptr++;
        }
#endif /* LCD_DEPTH */
#endif /* LCD_PIXELFORMAT */

        _grey_info.rb->write(fd, linebuf, BMP_LINESIZE);
    }
}
