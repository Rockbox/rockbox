/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr, speedup by Jörg Hohensohn
 * Further speedup and reorganization by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdbool.h>
#include "system.h"

#define LCDR (PBDR_ADDR+1)

#ifdef HAVE_LCD_CHARCELLS
#define LCD_DS  1 /* PB0 = 1 --- 0001 ---  LCD-DS */
#define LCD_CS  2 /* PB1 = 1 --- 0010 --- /LCD-CS */
#define LCD_SD  4 /* PB2 = 1 --- 0100 ---  LCD-SD */
#define LCD_SC  8 /* PB3 = 1 --- 1000 ---  LCD-SC */
#else
#define LCD_SD  1 /* PB0 = 1 --- 0001 */
#define LCD_SC  2 /* PB1 = 1 --- 0010 */
#define LCD_RS  4 /* PB2 = 1 --- 0100 */
#define LCD_CS  8 /* PB3 = 1 --- 1000 */
#define LCD_DS LCD_RS
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

void lcd_write_command(int byte) __attribute__ ((section (".icode")));
void lcd_write_command(int byte)
{
    asm (
        "and.b   %0, @(r0,gbr)"
        : /* outputs */
        : /* inputs */
        /* %0 */ "I"(~(LCD_CS|LCD_DS|LCD_SD|LCD_SC)),
        /* %1 = r0 */ "z"(LCDR)
    );

    asm (
        "0:                      \n"
        "and.b   %2,@(r0,gbr)    \n"
        "shll    %0              \n"
        "bf      1f              \n"
        "or.b    %3,@(r0,gbr)    \n"
        "1:                      \n"
        "or.b    %4,@(r0,gbr)    \n"
        "add     #-1,%1          \n"
        "cmp/pl  %1              \n"
        "bt      0b              \n"
        : /* outputs */
        : /* inputs */
        /* %0 */ "r"(((unsigned)byte)<<24),
        /* %1 */ "r"(8),
        /* %2 */ "I"(~(LCD_SC|LCD_SD|LCD_DS)),
        /* %3 */ "I"(LCD_SD),
        /* %4 */ "I"(LCD_SC),
        /* %5 = r0 */ "z"(LCDR)
    );

    asm (
        "or.b    %0, @(r0,gbr)"
        : /* outputs */
        : /* inputs */
        /* %0 */ "I"(LCD_CS|LCD_DS|LCD_SD|LCD_SC),
        /* %1 = r0 */ "z"(LCDR)
    );
}


/* A high performance function to write data to the display, 
   one or multiple bytes. */
void lcd_write_data(unsigned char* p_bytes, int count) __attribute__ ((section (".icode")));

#ifdef HAVE_LCD_CHARCELLS
/* This version works for both Player and Recorder models */
void lcd_write_data(unsigned char* p_bytes, int count)
{
    do
    {
        unsigned int byte;
        unsigned int sda1;     /* precalculated SC=low,SD=1 */

        byte = *p_bytes++ << 24; /* fetch to MSB position */

        /* This code will fail if an interrupt changes the contents of PBDRL.
           If so, we must disable the interrupt here. */

        /* precalculate the values for later bit toggling, init data write */
        asm (
            "mov.b   @%1,r0  \n" /* r0 = PBDRL */
            "or      %3,r0   \n" /* r0 |= LCD_DS | LCD_SD    DS and SD high */
            "and     %2,r0   \n" /* r0 &= ~(LCD_CS | LCD_SC) CS and SC low */
            "mov.b   r0,@%1  \n" /* PBDRL = r0 */
            "mov     r0,%0   \n" /* sda1 = r0 */
            : /* outputs */
            /* %0 */ "=r"(sda1)
            : /* inputs */
            /* %1 */ "r"(LCDR),
            /* %2 */ "I"(~(LCD_CS | LCD_SC)),
            /* %3 */ "I"(LCD_DS | LCD_SD)
            : /* trashed */
            "r0"
        );
        
        /* unrolled loop to serialize the byte */
        asm (
            "shll    %0      \n" /* shift the msb into carry */
            ".align  2       \n"
            "mov     %1,r0   \n" /* copy precalculated port value */
            "bt      1f      \n" /* data bit = 1? */
            "and     %5,r0   \n" /* no: r0 &= ~LCD_SD */
            "1:              \n"
            "shll    %0      \n" /* next shift here for alignment */
            "mov.b   r0,@%3  \n" /* set data to port */
            "or      %2,r0   \n" /* rise SC (independent of SD level) */
            "mov.b   r0,@%3  \n" /* set to port */
            
            "mov     %1,r0   \n"
            "bt      1f      \n"
            "and     %5,r0   \n"
            "1:              \n"
            "mov.b   r0,@%3  \n"
            "or      %2,r0   \n"
            "mov.b   r0,@%3  \n"
            
            "shll    %0      \n"
            "mov     %1,r0   \n"
            "bt      1f      \n"
            "and     %5,r0   \n"
            "1:              \n"
            "shll    %0      \n"
            "mov.b   r0,@%3  \n"
            "or      %2,r0   \n"
            "mov.b   r0,@%3  \n"
            
            "mov     %1,r0   \n"
            "bt      1f      \n"
            "and     %5,r0   \n"
            "1:              \n"
            "mov.b   r0,@%3  \n"
            "or      %2,r0   \n"
            "mov.b   r0,@%3  \n"
            
            "shll    %0      \n"
            "mov     %1,r0   \n"
            "bt      1f      \n"
            "and     %5,r0   \n"
            "1:              \n"
            "shll    %0      \n"
            "mov.b   r0,@%3  \n"
            "or      %2,r0   \n"
            "mov.b   r0,@%3  \n"
            
            "mov     %1,r0   \n"
            "bt      1f      \n"
            "and     %5,r0   \n"
            "1:              \n"
            "mov.b   r0,@%3  \n"
            "or      %2,r0   \n"
            "mov.b   r0,@%3  \n"
            
            "shll    %0      \n"
            "mov     %1,r0   \n"
            "bt      1f      \n"
            "and     %5,r0   \n"
            "1:              \n"
            "shll    %0      \n"
            "mov.b   r0,@%3  \n"
            "or      %2,r0   \n"
            "mov.b   r0,@%3  \n"
            
            "mov     %1,r0   \n"
            "bt      1f      \n"
            "and     %5,r0   \n"
            "1:              \n"
            "mov.b   r0,@%3  \n"
            "or      %2,r0   \n"
            "mov.b   r0,@%3  \n"
            
            "or      %4,r0   \n" /* restore port */
            "mov.b   r0,@%3  \n"
            : /* outputs */
            : /* inputs */
            /* %0 */ "r"(byte),
            /* %1 */ "r"(sda1),
            /* %2 */ "I"(LCD_SC),
            /* %3 */ "r"(LCDR),
            /* %4 */ "I"(LCD_CS | LCD_DS | LCD_SD | LCD_SC),
            /* %5 */ "I"(~(LCD_SD))
            : /* trashed */
            "r0"
        );

        /* This is the place to reenable the interrupts, if we have disabled
           them. See above. */

    } while (--count); /* tail loop is faster */
}

#else /* #ifdef HAVE_LCD_CHARCELLS */
/* A further optimized version, exploits that SD is on bit 0 for recorders */
void lcd_write_data(unsigned char* p_bytes, int count)
{
    do
    {
        unsigned byte;
        unsigned sda1;     /* precalculated SC=low,SD=1 */

        /* take inverse data, so I can use the NEGC instruction below, it is
           the only carry add/sub which does not destroy a source register */
        byte = ~(*p_bytes++ << 24); /* fetch to MSB position */

        /* This code will fail if an interrupt changes the contents of PBDRL.
           If so, we must disable the interrupt here. */

        /* precalculate the values for later bit toggling, init data write */
        asm (
            "mov.b   @%1,r0  \n" /* r0 = PBDRL */
            "or      %3,r0   \n" /* r0 |= LCD_DS | LCD_SD     DS and SD high, */
            "and     %2,r0   \n" /* r0 &= ~(LCD_CS | LCD_SC)  CS and SC low   */
            "mov.b   r0,@%1  \n" /* PBDRL = r0 */
            "neg     r0,%0   \n" /* sda1 = 0-r0 */
            : /* outputs: */
            /* %0 */ "=r"(sda1)
            : /* inputs: */
            /* %1 */ "r"(LCDR),
            /* %2 */ "I"(~(LCD_CS | LCD_SC)),
            /* %3 */ "I"(LCD_DS | LCD_SD)
            : /* trashed */
            "r0"
        );

        /* unrolled loop to serialize the byte */
        asm (
            "shll    %0      \n" /* shift the MSB into carry */
            ".align  2       \n"
            "negc    %1, r0  \n" /* carry to SD, SC low */
            "shll    %0      \n" /* next shift here for alignment */
            "mov.b   r0,@%3  \n" /* set data to port */
            "or      %2, r0  \n" /* rise SC (independent of SD level) */
            "mov.b   r0,@%3  \n" /* set to port */

            "negc    %1, r0  \n"
            "mov.b   r0,@%3  \n"
            "or      %2, r0  \n"
            "mov.b   r0,@%3  \n"

            "shll    %0      \n"
            "negc    %1, r0  \n"
            "shll    %0      \n"
            "mov.b   r0,@%3  \n"
            "or      %2, r0  \n"
            "mov.b   r0,@%3  \n"

            "negc    %1, r0  \n"
            "mov.b   r0,@%3  \n"
            "or      %2, r0  \n"
            "mov.b   r0,@%3  \n"

            "shll    %0      \n"
            "negc    %1, r0  \n"
            "shll    %0      \n"
            "mov.b   r0,@%3  \n"
            "or      %2, r0  \n"
            "mov.b   r0,@%3  \n"

            "negc    %1, r0  \n"
            "mov.b   r0,@%3  \n"
            "or      %2, r0  \n"
            "mov.b   r0,@%3  \n"

            "shll    %0      \n"
            "negc    %1, r0  \n"
            "shll    %0      \n"
            "mov.b   r0,@%3  \n"
            "or      %2, r0  \n"
            "mov.b   r0,@%3  \n"

            "negc    %1, r0  \n"
            "mov.b   r0,@%3  \n"
            "or      %2, r0  \n"
            "mov.b   r0,@%3  \n"

            "or      %4, r0  \n" /* restore port */
            "mov.b   r0,@%3  \n"
            : /* outputs: */
            : /* inputs: */
            /* %0 */ "r"(byte),
            /* %1 */ "r"(sda1),
            /* %2 */ "I"(LCD_SC),
            /* %3 */ "r"(LCDR),
            /* %4 */ "I"(LCD_CS|LCD_DS|LCD_SD|LCD_SC)
            :  /* trashed: */
            "r0"
        );

        /* This is the place to reenable the interrupts, if we have disabled
           them. See above. */

    } while (--count); /* tail loop is faster */
}
#endif /* #ifdef HAVE_LCD_CHARCELLS */


