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

#if defined ARCHOS_RECORDER     /* verified */  \
 || defined ARCHOS_FMRECORDER   /* should be identical */  \
 || defined ARCHOS_RECORDERV2   /* should be identical */  \
 || defined ARCHOS_ONDIOFM      /* verified */  \
 || defined ARCHOS_ONDIOSP      /* verified */
/* Average measurements of a Recorder v1, an Ondio FM, a backlight-modded 
 * Ondio FM, and an Ondio SP. */
static const unsigned char lcdlinear[256] = {
  5,   8,  10,  12,  14,  16,  18,  20,  22,  24,  26,  28,  29,  31,  33,  35,
 37,  39,  40,  42,  43,  45,  46,  48,  49,  50,  51,  53,  54,  55,  57,  58,
 59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  68,  69,  70,  71,  71,  72,
 73,  74,  74,  75,  76,  77,  77,  78,  79,  79,  80,  80,  81,  81,  82,  82,
 83,  84,  84,  85,  86,  86,  87,  87,  88,  88,  89,  89,  90,  90,  91,  91,
 92,  92,  93,  93,  94,  94,  95,  95,  96,  96,  97,  98,  98,  99, 100, 100,
101, 101, 102, 103, 103, 104, 105, 105, 106, 106, 107, 107, 108, 108, 109, 109,
110, 110, 111, 112, 112, 113, 114, 114, 115, 115, 116, 117, 117, 118, 119, 119,
120, 120, 121, 122, 123, 123, 124, 125, 126, 126, 127, 128, 129, 129, 130, 131,
132, 132, 133, 134, 135, 135, 136, 137, 138, 138, 139, 140, 140, 141, 141, 142,
143, 144, 145, 146, 147, 147, 148, 149, 150, 151, 152, 153, 154, 154, 155, 156,
157, 158, 159, 160, 161, 161, 162, 163, 164, 165, 166, 167, 168, 168, 169, 170,
171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 184, 185, 186, 187,
188, 189, 191, 192, 194, 195, 197, 198, 199, 200, 202, 203, 204, 205, 207, 208,
209, 210, 212, 213, 215, 216, 218, 219, 220, 221, 222, 223, 225, 226, 227, 228,
229, 230, 232, 233, 234, 235, 237, 238, 239, 240, 242, 243, 244, 246, 247, 248
};
/* The actual LCD scanrate varies a lot with temperature on these targets */
#define LCD_SCANRATE 67 /* Hz */

#elif defined IAUDIO_M3         /* verified */
/* Average measurements of 2 iAudio remotes connected to an M3. */
static const unsigned char lcdlinear[256] = {
  5,   9,  13,  17,  21,  26,  30,  34,  38,  42,  46,  50,  54,  58,  62,  66,
 70,  73,  76,  78,  80,  82,  84,  86,  88,  90,  91,  92,  94,  95,  96,  97,
 98,  99,  99, 100, 101, 102, 102, 103, 104, 104, 105, 105, 106, 107, 107, 108,
109, 109, 110, 110, 111, 111, 112, 112, 113, 113, 114, 114, 115, 115, 116, 116,
117, 117, 118, 118, 119, 119, 120, 120, 121, 121, 121, 122, 122, 123, 123, 123,
124, 124, 124, 125, 125, 126, 126, 126, 127, 127, 127, 128, 128, 129, 129, 129,
130, 130, 131, 131, 132, 132, 133, 133, 134, 134, 134, 135, 135, 136, 136, 136,
137, 137, 137, 138, 138, 139, 139, 139, 140, 140, 141, 141, 142, 142, 143, 143,
144, 144, 145, 145, 146, 147, 147, 148, 149, 149, 150, 150, 151, 151, 152, 152,
153, 153, 154, 154, 155, 155, 156, 156, 157, 157, 158, 158, 159, 160, 160, 161,
162, 162, 163, 164, 164, 165, 166, 167, 168, 168, 169, 169, 170, 171, 171, 172,
173, 173, 174, 175, 176, 176, 177, 178, 179, 179, 180, 181, 182, 182, 183, 184,
185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 198, 199, 200, 201,
202, 203, 204, 205, 207, 208, 209, 210, 211, 212, 213, 214, 216, 217, 218, 219,
220, 221, 222, 223, 225, 226, 227, 228, 229, 230, 231, 232, 234, 235, 236, 237,
238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 249, 250, 251, 252
};
/* The actual LCD scanrate is twice as high, but we cannot transfer fast enough
 * for 150Hz. Even at 75Hz, greyscale display is very smooth. Average from
 * 2 iAudio remotes. */
#define LCD_SCANRATE 75 /* Hz */

#elif defined IAUDIO_M5         /* verified */
/* Measurement of one iAudio M5L */
static const unsigned char lcdlinear[256] = {
  4,   6,   8,  10,  11,  13,  15,  17,  19,  21,  22,  24,  25,  27,  28,  30,
 32,  33,  35,  36,  37,  39,  40,  42,  43,  44,  45,  46,  48,  49,  50,  51,
 52,  52,  53,  54,  55,  55,  56,  57,  58,  58,  59,  60,  61,  61,  62,  63,
 64,  64,  65,  65,  66,  66,  67,  67,  68,  68,  69,  70,  70,  71,  72,  72,
 73,  73,  74,  75,  75,  76,  77,  77,  78,  78,  79,  79,  80,  80,  81,  81,
 82,  82,  83,  84,  84,  85,  86,  86,  87,  87,  88,  89,  89,  90,  91,  91,
 92,  92,  93,  93,  94,  94,  95,  95,  96,  96,  97,  98,  98,  99, 100, 100,
101, 101, 102, 102, 103, 103, 104, 104, 105, 105, 106, 106, 107, 107, 108, 108,
109, 109, 110, 110, 111, 111, 112, 112, 113, 113, 114, 114, 115, 115, 116, 116,
117, 117, 118, 119, 119, 120, 121, 121, 122, 122, 123, 124, 124, 125, 126, 126,
127, 127, 128, 129, 130, 130, 131, 132, 133, 133, 134, 135, 135, 136, 137, 137,
138, 139, 140, 141, 142, 142, 143, 144, 145, 146, 147, 148, 149, 149, 150, 151,
152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 163, 164, 165, 167, 168, 169,
170, 172, 173, 175, 177, 179, 180, 182, 184, 186, 188, 190, 192, 194, 196, 198,
200, 202, 204, 205, 207, 209, 210, 212, 214, 216, 218, 219, 221, 223, 224, 226,
228, 230, 231, 233, 235, 236, 237, 239, 241, 243, 244, 246, 248, 249, 250, 252
};
#define LCD_SCANRATE 73 /* Hz */

#elif defined IPOD_1G2G         /* verified */
/* Average measurements of an iPod 1st Gen (0x00010001) and an iPod 2nd Gen
 * (0x00020000), measured with both backlight off & backlight on (flipped
 * curves) and medium load (white background when measuring with backlight on),
 * as the curve is load dependent (the controller's step-up converter doesn't
 * provide enough juice). Table is for backlight_off state. */
static const unsigned char lcdlinear[256] = {
  4,   6,   8,   9,  11,  13,  14,  16,  17,  18,  20,  21,  23,  24,  26,  27,
 29,  30,  31,  32,  34,  35,  36,  37,  38,  39,  40,  41,  41,  42,  43,  44,
 45,  45,  46,  47,  47,  48,  49,  49,  50,  50,  51,  51,  52,  52,  53,  53,
 54,  54,  54,  55,  55,  56,  56,  56,  57,  57,  57,  58,  58,  59,  59,  59,
 60,  60,  60,  61,  61,  62,  62,  62,  63,  63,  63,  64,  64,  65,  65,  65,
 66,  66,  67,  67,  68,  68,  69,  69,  70,  70,  71,  71,  72,  72,  73,  73,
 74,  74,  74,  75,  75,  76,  76,  76,  77,  77,  78,  78,  79,  79,  80,  80,
 81,  81,  82,  82,  83,  83,  84,  84,  85,  85,  86,  86,  87,  87,  88,  88,
 89,  89,  90,  91,  92,  92,  93,  94,  95,  95,  96,  97,  97,  98,  99,  99,
100, 100, 101, 102, 102, 103, 104, 104, 105, 105, 106, 107, 107, 108, 109, 109,
110, 110, 111, 112, 113, 113, 114, 115, 116, 116, 117, 118, 119, 119, 120, 121,
122, 122, 123, 124, 125, 125, 126, 127, 128, 129, 130, 131, 131, 132, 133, 134,
135, 137, 138, 139, 141, 142, 144, 145, 146, 147, 149, 150, 151, 152, 154, 155,
156, 158, 159, 161, 162, 164, 165, 167, 169, 171, 172, 174, 175, 177, 178, 180,
182, 184, 186, 188, 189, 191, 193, 195, 197, 199, 201, 203, 206, 208, 210, 212,
214, 217, 219, 221, 224, 226, 229, 231, 233, 236, 238, 240, 243, 245, 247, 250
};
/* Average from an iPod 1st Gen and an iPod 2nd Gen */
#define LCD_SCANRATE 96 /* Hz */

#elif defined IPOD_MINI2G       /* verified */  \
   || defined IPOD_MINI         /* should be identical */  \
   || defined IPOD_3G           /* TODO: verify */  \
   || defined IPOD_4G           /* TODO: verify */
/* Measurement of one iPod Mini G2 */
static const unsigned char lcdlinear[256] = {
  2,   5,   7,  10,  12,  15,  17,  20,  22,  24,  26,  28,  30,  32,  34,  36,
 38,  40,  41,  42,  44,  45,  46,  47,  48,  49,  50,  50,  51,  52,  52,  53,
 54,  54,  55,  55,  56,  56,  57,  57,  58,  58,  58,  59,  59,  60,  60,  60,
 61,  61,  61,  62,  62,  63,  63,  63,  64,  64,  64,  64,  65,  65,  65,  65,
 66,  66,  66,  66,  67,  67,  67,  67,  68,  68,  68,  68,  69,  69,  69,  69,
 70,  70,  70,  70,  71,  71,  71,  71,  72,  72,  72,  73,  73,  74,  74,  74,
 75,  75,  75,  75,  76,  76,  76,  76,  77,  77,  77,  78,  78,  79,  79,  79,
 80,  80,  80,  81,  81,  82,  82,  82,  83,  83,  83,  84,  84,  85,  85,  85,
 86,  86,  86,  87,  87,  88,  88,  88,  89,  89,  90,  90,  91,  91,  92,  92,
 93,  93,  94,  94,  95,  95,  96,  96,  97,  97,  98,  99,  99, 100, 101, 101,
102, 102, 103, 104, 104, 105, 106, 106, 107, 108, 109, 110, 110, 111, 112, 113,
114, 115, 115, 116, 117, 118, 118, 119, 120, 121, 121, 122, 123, 124, 124, 125,
126, 127, 128, 129, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140,
141, 142, 143, 144, 146, 147, 148, 149, 150, 151, 153, 154, 155, 156, 158, 159,
160, 162, 163, 165, 166, 168, 169, 171, 172, 175, 177, 180, 182, 185, 187, 190,
192, 196, 199, 203, 206, 210, 213, 217, 220, 223, 227, 230, 234, 238, 242, 246
};
/* Average of an iPod Mini G2 and 2 3rd Gen iPods. */
#define LCD_SCANRATE 87 /* Hz */

#elif defined IRIVER_H100_SERIES /* verified */
/* Measurement of one Iriver H140 */
static const unsigned char lcdlinear[256] = {
  5,   8,  12,  15,  18,  22,  25,  28,  31,  34,  36,  39,  42,  44,  47,  50,
 53,  55,  57,  59,  62,  64,  66,  68,  70,  71,  72,  73,  75,  76,  77,  78,
 79,  80,  80,  81,  82,  83,  83,  84,  85,  85,  86,  86,  87,  87,  88,  88,
 89,  89,  90,  90,  91,  91,  92,  92,  93,  93,  93,  94,  94,  95,  95,  95,
 96,  96,  96,  97,  97,  98,  98,  98,  99,  99,  99, 100, 100, 101, 101, 101,
102, 102, 102, 103, 103, 104, 104, 104, 105, 105, 106, 106, 107, 107, 108, 108,
109, 109, 109, 110, 110, 111, 111, 111, 112, 112, 113, 113, 114, 114, 115, 115,
116, 116, 117, 117, 118, 118, 119, 119, 120, 120, 121, 121, 122, 122, 123, 123,
124, 124, 125, 125, 126, 127, 127, 128, 129, 129, 130, 130, 131, 131, 132, 132,
133, 133, 134, 135, 135, 136, 137, 137, 138, 138, 139, 139, 140, 140, 141, 141,
142, 142, 143, 143, 144, 145, 145, 146, 147, 147, 148, 148, 149, 150, 150, 151,
152, 152, 153, 153, 154, 155, 155, 156, 157, 157, 158, 159, 160, 160, 161, 162,
163, 164, 165, 166, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177,
178, 179, 181, 182, 183, 184, 186, 187, 188, 189, 191, 192, 193, 194, 196, 197,
198, 200, 202, 203, 205, 207, 208, 210, 212, 214, 215, 217, 218, 220, 221, 223,
224, 226, 228, 229, 231, 233, 235, 236, 238, 240, 241, 242, 244, 245, 246, 248
};
#define LCD_SCANRATE 70 /* Hz */

#else  /* not yet calibrated targets - generic linear mapping */
/* TODO: calibrate iFP7xx
 * TODO: Olympus m:robe 100 */
static const unsigned char lcdlinear[256] = {
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};
/* generic default */
#define LCD_SCANRATE 70 /* Hz */

#endif
#else /* SIMULATOR */
/* undo a (generic) PC display gamma of 2.0 to simulate target behaviour */
static const unsigned char lcdlinear[256] = {
  0,  16,  23,  28,  32,  36,  39,  42,  45,  48,  50,  53,  55,  58,  60,  62,
 64,  66,  68,  70,  71,  73,  75,  77,  78,  80,  81,  83,  84,  86,  87,  89,
 90,  92,  93,  94,  96,  97,  98, 100, 101, 102, 103, 105, 106, 107, 108, 109,
111, 112, 113, 114, 115, 116, 117, 118, 119, 121, 122, 123, 124, 125, 126, 127,
128, 129, 130, 131, 132, 133, 134, 135, 135, 136, 137, 138, 139, 140, 141, 142,
143, 144, 145, 145, 146, 147, 148, 149, 150, 151, 151, 152, 153, 154, 155, 156,
156, 157, 158, 159, 160, 160, 161, 162, 163, 164, 164, 165, 166, 167, 167, 168,
169, 170, 170, 171, 172, 173, 173, 174, 175, 176, 176, 177, 178, 179, 179, 180,
181, 181, 182, 183, 183, 184, 185, 186, 186, 187, 188, 188, 189, 190, 190, 191,
192, 192, 193, 194, 194, 195, 196, 196, 197, 198, 198, 199, 199, 200, 201, 201,
202, 203, 203, 204, 204, 205, 206, 206, 207, 208, 208, 209, 209, 210, 211, 211,
212, 212, 213, 214, 214, 215, 215, 216, 217, 217, 218, 218, 219, 220, 220, 221,
221, 222, 222, 223, 224, 224, 225, 225, 226, 226, 227, 228, 228, 229, 229, 230,
230, 231, 231, 232, 233, 233, 234, 234, 235, 235, 236, 236, 237, 237, 238, 238,
239, 240, 240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246, 246, 247,
247, 248, 248, 249, 249, 250, 250, 251, 251, 252, 252, 253, 253, 254, 254, 255
};
#endif /* SIMULATOR */

/* Prototypes */
static inline void _deferred_update(void) __attribute__ ((always_inline));
static int exp_s16p16(int x);
static int log_s16p16(int x);
static void grey_screendump_hook(int fd);
static void fill_gvalues(void);
#ifdef SIMULATOR
static unsigned long _grey_get_pixel(int x, int y);
#else
static void _timer_isr(void);
#endif


#if defined(HAVE_BACKLIGHT_INVERSION) && !defined(SIMULATOR)
static void invert_gvalues(void)
{
    unsigned char *val, *end;
    unsigned char rev_tab[256];
    unsigned i;
    unsigned last_i = 0;
    unsigned x = 0;
    unsigned last_x;
    
    if (_grey_info.flags & GREY_BUFFERED)
    {
        fill_gvalues();
        grey_update();
    }
    else /* Unbuffered - need crude reconstruction */
    {
        /* Step 1: Calculate a transposed table for undoing the old mapping */
        for (i = 0; i < 256; i++)
        {
            last_x = x;
            x = _grey_info.gvalue[i];
            if (x > last_x)
            {
                rev_tab[last_x++] = (last_i + i) / 2;
                while (x > last_x)
                    rev_tab[last_x++] = i;
                last_i = i;
            }
        }
        rev_tab[last_x++] = (last_i + 255) / 2;
        while (256 > last_x)
            rev_tab[last_x++] = 255;

        /* Step 2: Calculate new mapping */
        fill_gvalues();

        /* Step 3: Transpose all pixel values */
        val = _grey_info.values;
        end = val + _GREY_MULUQ(_grey_info.width, _grey_info.height);

        do
            *val = _grey_info.gvalue[rev_tab[*val]];
        while (++val < end);
    }
}
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
#if defined(HAVE_BACKLIGHT_INVERSION) && !defined(SIMULATOR)
    unsigned long check = _grey_info.rb->is_backlight_on(true) 
                        ? 0 : _GREY_BACKLIGHT_ON;
    
    if ((_grey_info.flags & (_GREY_BACKLIGHT_ON|GREY_RAWMAPPED)) == check)
    {
        _grey_info.flags ^= _GREY_BACKLIGHT_ON;
        invert_gvalues();
        return; /* don't overload this timer slot */
    }
#endif
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

static void fill_gvalues(void)
{
    int i;
    unsigned data;

#if defined(HAVE_BACKLIGHT_INVERSION) && !defined(SIMULATOR)
    unsigned imask = (_grey_info.flags & _GREY_BACKLIGHT_ON) ? 0xff : 0;
#else
    const unsigned imask = 0;
#endif
    for (i = 0; i < 256; i++)
    {
        data = exp_s16p16((_GREY_GAMMA * log_s16p16(i * 257 + 1)) >> 8) + 128;
        data = (data - (data >> 8)) >> 8; /* approx. data /= 257 */
        data = ((lcdlinear[data ^ imask] ^ imask) << 7) + 127;
        _grey_info.gvalue[i] = (data + (data >> 8)) >> 8;
                                      /* approx. data / 255 */
    }
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
    _grey_info.flags = features & 0xff;
    _grey_info.fg_brightness = 0;
    _grey_info.bg_brightness = 255;
    _grey_info.drawmode = DRMODE_SOLID;
    _grey_info.curfont = FONT_SYSFIXED;

    /* precalculate the value -> pattern index conversion table, taking
       linearisation and gamma correction into account */
    if (features & GREY_RAWMAPPED)
    {
        for (i = 0; i < 256; i++)
        {
            data = i << 7;
            _grey_info.gvalue[i] = (data + (data >> 8)) >> 8;
        }
    }
    else
    {
#if defined(HAVE_BACKLIGHT_INVERSION) && !defined(SIMULATOR)
        if (_grey_info.rb->is_backlight_on(true))
            _grey_info.flags |= _GREY_BACKLIGHT_ON;
#endif
        fill_gvalues();
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
        _grey_info.rb->timer_register(1, NULL, TIMER_FREQ / LCD_SCANRATE, 1,
                                      _timer_isr IF_COP(, CPU));
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
