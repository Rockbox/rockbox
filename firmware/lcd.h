/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __LCD_H__
#define __LCD_H__

#include "sh7034.h"
#include "types.h"
#include "config.h"

#define LCDR (PBDR+1)

/* PA14 : /LCD-BL --- backlight */
#define LCD_BL (14-8)

#ifdef HAVE_LCD_CHARCELLS
 /* JukeBox MP3 Player - AJB6K, AJBS20 */
#  define LCD_DS  +1 // PB0 = 1 --- 0001 ---  LCD-DS
#  define LCD_CS  +2 // PB1 = 1 --- 0010 --- /LCD-CS
#  define LCD_SD  +4 // PB2 = 1 --- 0100 ---  LCD-SD
#  define LCD_SC  +8 // PB3 = 1 --- 1000 ---  LCD-SC
#  ifndef JBP_OLD
#    define LCD_CONTRAST_SET ((char)0x50)
#    define LCD_CRAM         ((char)0x80) /* Characters */
#    define LCD_PRAM         ((char)0xC0) /*  Patterns  */
#    define LCD_IRAM         ((char)0x40) /*    Icons   */
#  else
#    define LCD_CONTRAST_SET ((char)0xA8)
#    define LCD_CRAM         ((char)0xB0) /* Characters */
#    define LCD_PRAM         ((char)0x80) /*  Patterns  */
#    define LCD_IRAM         ((char)0xE0) /*    Icons   */
#  endif
#  define LCD_ASCII(c)       (lcd_ascii[(c)&255])
#  define LCD_CURSOR(x,y)    ((char)(LCD_CRAM+((y)*16+(x))))
#  define LCD_ICON(i)        ((char)(LCD_IRAM+i))          
#    define LCD_ICON_BATTERY         0
#      define LCD_BATTERY_FRAME   0x02
#      define LCD_BATTERY_BAR1    0x08
#      define LCD_BATTERY_BAR2    0x04
#      define LCD_BATTERY_BAR3    0x10
#    define LCD_ICON_USB             2
#      define LCD_USB_LOGO        0xFF
#    define LCD_ICON_PLAY            3
#      define LCD_PLAY_ICON       0xFF
#    define LCD_ICON_RECORD          4
#      define LCD_RECORD_ICON     0x10
#    define LCD_ICON_STOP            5
#      define LCD_STOP_ICON       0x0F
#    define LCD_ICON_AUDIO           5
#      define LCD_AUDIO_ICON      0xF0
#    define LCD_ICON_REVERSE         6
#      define LCD_REVERSE_ICON    0xFF
#    define LCD_ICON_SINGLE          7
#      define LCD_SINGLE_ICON     0xFF
#    define LCD_ICON_VOLUME0         9
#      define LCD_VOLUME_ICON     0x04   
#      define LCD_VOLUME_BAR1     0x02
#      define LCD_VOLUME_BAR2     0x01
#    define LCD_ICON_VOLUME1        10
#      define LCD_VOLUME_BAR3     0x08
#      define LCD_VOLUME_BAR4     0x04
#      define LCD_VOLUME_BAR5     0x01
#    define LCD_ICON_PARAM          10
#      define LCD_PARAM_SYMBOL    0xF0
#endif

#ifdef HAVE_LCD_BITMAP
/* JukeBox MP3 Recorder - AJBR --- FIXME */

/* Defines from Alan on IRC, April 11th 2002 */
#define LCD_SD  +1 // PB0 = 1 --- 0001
#define LCD_SC  +2 // PB1 = 1 --- 0010
#define LCD_RS  +4 // PB2 = 1 --- 0100
#define LCD_CS  +8 // PB3 = 1 --- 1000

#define LCD_DS LCD_RS

#define LCD_SET_LOWER_COLUMN_ADDRESS              ((char)0x00)
#define LCD_SET_HIGHER_COLUMN_ADDRESS             ((char)0x10)
#define LCD_SET_INTERNAL_REGULATOR_RESISTOR_RATIO ((char)0x20)
#define LCD_SET_POWER_CONTROL_REGISTER            ((char)0x28)
#define LCD_SET_DISPLAY_START_LINE                ((char)0x40)
#define LCD_SET_CONTRAST_CONTROL_REGISTER         ((char)0x81)
#define LCD_SET_SEGMENT_REMAP                     ((char)0xA0)
#define LCD_SET_LCD_BIAS                          ((char)0xA2)
#define LCD_SET_ENTIRE_DISPLAY_OFF                ((char)0xA4)
#define LCD_SET_ENTIRE_DISPLAY_ON                 ((char)0xA5)
#define LCD_SET_NORMAL_DISPLAY                    ((char)0xA6)
#define LCD_SET_REVERSE_DISPLAY                   ((char)0xA7)
#define LCD_SET_INDICATOR_OFF                     ((char)0xAC)
#define LCD_SET_INDICATOR_ON                      ((char)0xAD)
#define LCD_SET_DISPLAY_OFF                       ((char)0xAE)
#define LCD_SET_DISPLAY_ON                        ((char)0xAF)
#define LCD_SET_PAGE_ADDRESS                      ((char)0xB0)
#define LCD_SET_COM_OUTPUT_SCAN_DIRECTION         ((char)0xC0)
#define LCD_SET_DISPLAY_OFFSET                    ((char)0xD3)
#define LCD_SET_READ_MODIFY_WRITE_MODE            ((char)0xE0)
#define LCD_SOFTWARE_RESET                        ((char)0xE2)
#define LCD_NOP                                   ((char)0xE3)
#define LCD_SET_END_OF_READ_MODIFY_WRITE_MODE     ((char)0xEE)


#define	DISP_X		112
#define	DISP_Y		64

#define LCD_WIDTH       DISP_X   /* Display width in pixels */
#define LCD_HEIGHT      DISP_Y   /* Display height in pixels */

void lcd_init (void);
void lcd_update (void);
void lcd_clear_display (void);
void lcd_position (int x, int y, int size);
void lcd_string (const char *str);
void lcd_bitmap (const unsigned char *src, int x, int y, int nx, int ny,
                 bool clear);
void lcd_clearrect (int x, int y, int nx, int ny);
void lcd_fillrect (int x, int y, int nx, int ny);
void lcd_invertrect (int x, int y, int nx, int ny);
void lcd_drawline( int x1, int y1, int x2, int y2 );
void lcd_drawpixel(int x, int y);
void lcd_clearpixel(int x, int y);
#endif


#ifndef SIMULATOR

extern void lcd_data (int data);
extern void lcd_instruction (int instruction);
extern void lcd_zero (int length);
extern void lcd_fill (int data,int length);
extern void lcd_copy (void *data,int count);

#ifdef HAVE_LCD_CHARCELLS

extern void lcd_puts (char const *string);
extern void lcd_putns (char const *string,int n);
extern void lcd_putc (int character);
extern void lcd_puthex (unsigned int value,int digits);

extern void lcd_pattern (int which,char const *pattern,int count);

#endif  /* HAVE_LCD_CHARCELLS */

#endif /* SIMULATOR */

#endif /* __LCD_H__ */
