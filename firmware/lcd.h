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

#include <sh7034.h>
#include <system.h>

#define LCDR (PBDR+1)

/* PA14 : /LCD-BL --- backlight */
#define LCD_BL (14-8)

#ifdef JBP /* JukeBox MP3 Player - AJB6K, AJBS20 */
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

#ifdef JBR /* JukeBox MP3 Recorder - AJBR --- FIXME */
#  error "JBR : FIX ME"
#endif


/*
 * About /CS,DS,SC,SD
 * ------------------
 *
 * LCD on JBP and JBR uses a SPI protocol to receive orders (SDA and SCK lines)
 *
 * - /CS -> Chip Selection line :
 *            0 : LCD chipset is activated.
 * -  DS -> Data Selection line, latched at the rising edge
 *          of the 8th serial clock (*) :
 *            0 : instruction register,
 *            1 : data register; 
 * -  SC -> Serial Clock line (SDA).
 * -  SD -> Serial Data line (SCK), latched at the rising edge
 *          of each serial clock (*).  
 *
 *    _                                                          _
 * /CS \                                                        /
 *      \______________________________________________________/
 *    _____  ____  ____  ____  ____  ____  ____  ____  ____  _____ 
 *  SD     \/ D7 \/ D6 \/ D5 \/ D4 \/ D3 \/ D2 \/ D1 \/ D0 \/
 *    _____/\____/\____/\____/\____/\____/\____/\____/\____/\_____
 *
 *    _____     _     _     _     _     _     _     _     ________ 
 *  SC     \   * \   * \   * \   * \   * \   * \   * \   *
 *          \_/   \_/   \_/   \_/   \_/   \_/   \_/   \_/
 *    _  _________________________________________________________ 
 *  DS \/                                                  
 *    _/\_________________________________________________________
 *
 */

/*
 * The only way to do logical operations in an atomic way
 * on SH1 is using :
 *
 *   or.b/and.b/tst.b/xor.b #imm,@(r0,gbr)
 *
 * but GCC doesn't generate them at all so some assembly
 * codes are needed here.
 *
 * The Global Base Register gbr is expected to be zero
 * and r0 is the address of one register in the on-chip
 * peripheral module.
 *
 */ 

static inline void lcd_start (void)
 /*
  * Enter a LCD session :
  *
  * QI(LCDR) &= ~(LCD_CS|LCD_DS|LCD_SD|LCD_SC);
  */
  {
    asm
      ("and.b\t%0,@(r0,gbr)"
       :
       : /* %0 */ "I"(~(LCD_CS|LCD_DS|LCD_SD|LCD_SC)),
         /* %1 */ "z"(LCDR));
  }

static inline void lcd_stop (void)
 /*
  * Leave a LCD session :
  *
  * QI(LCDR) |= LCD_CS|LCD_RS|LCD_SD|LCD_SC;
  */
  {
    asm
      ("or.b\t%0,@(r0,gbr)"
       :
       : /* %0 */ "I"(LCD_CS|LCD_DS|LCD_SD|LCD_SC),
         /* %1 */ "z"(LCDR));
  }

static inline void lcd_byte (int byte,int rs)
 /*
  * char j = 0x80;
  * if (rs)
  *   do
  *     {
  *       QI(LCDR) &= ~(LCD_SC|LCD_SD);
  *       if (j & byte)
  *         QI(LCDR) |= LCD_SD;
  *       QI(LCDR) |= LCD_SC|LCD_DS;
  *     }
  *   while ((unsigned char)j >>= 1);
  * else
  *   do
  *     {
  *       QI(LCDR) &= ~(LCD_SC|LCD_SD|LCD_DS);
  *       if (j & byte)
  *         QI(LCDR) |= LCD_SD;
  *       QI(LCDR) |= LCD_SC;
  *     }
  *   while ((unsigned char)j >>= 1);
  */
  {
    if (rs > 0)
      asm
        ("shll8\t%0\n"
         "0:\n\t"
         "and.b\t%2,@(r0,gbr)\n\t"
         "shll\t%0\n\t"  
         "bf\t1f\n\t"
         "or.b\t%3,@(r0,gbr)\n"
         "1:\n\t"
         "or.b\t%4,@(r0,gbr)\n"
         "add\t#-1,%1\n\t"
         "cmp/pl\t%1\n\t"
         "bt\t0b"
         :
         : /* %0 */ "r"(((unsigned)byte)<<16),
           /* %1 */ "r"(8),
           /* %2 */ "I"(~(LCD_SC|LCD_SD)),
           /* %3 */ "I"(LCD_SD),
           /* %4 */ "I"(LCD_SC|LCD_DS),
           /* %5 */ "z"(LCDR));
    else
      asm
        ("shll8\t%0\n"
         "0:\n\t"
         "and.b\t%2,@(r0,gbr)\n\t"
         "shll\t%0\n\t"  
         "bf\t1f\n\t"
         "or.b\t%3,@(r0,gbr)\n"
         "1:\n\t"
         "or.b\t%4,@(r0,gbr)\n"
         "add\t#-1,%1\n\t"
         "cmp/pl\t%1\n\t"
         "bt\t0b"
         :
         : /* %0 */ "r"(((unsigned)byte)<<16),
           /* %1 */ "r"(8),
           /* %2 */ "I"(~(LCD_SC|LCD_DS|LCD_SD)),
           /* %3 */ "I"(LCD_SD),
           /* %4 */ "I"(LCD_SC),
           /* %5 */ "z"(LCDR));
  }

extern void lcd_data (int data);
extern void lcd_instruction (int instruction);
extern void lcd_zero (int length);
extern void lcd_fill (int data,int length);
extern void lcd_copy (void *data,int count);

#ifdef JBP

extern void lcd_puts (char const *string);
extern void lcd_putns (char const *string,int n);
extern void lcd_putc (int character);
extern void lcd_puthex (unsigned int value,int digits);

extern void lcd_pattern (int which,char const *pattern,int count);

static inline void lcd_goto (int x,int y)
  { lcd_instruction (LCD_CURSOR(x,y)); }

#endif

#ifdef JBR
#  error "JBR : FIX ME"
#endif

/*** BACKLIGHT ***/

static inline void lcd_toggle_backlight (void)
  { toggle_bit (LCD_BL,PAIOR); }

static inline void lcd_turn_on_backlight (void)
  { set_bit (LCD_BL,PAIOR); }

static inline void lcd_turn_off_backlight (void)
  { clear_bit (LCD_BL,PAIOR); }

/*** ICONS ***/

#endif
