/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008 Rafaël Carré
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdbool.h>
#include "debug-target.h"
#include "button.h"
#include "lcd.h"
#include "font.h"
#include "system.h"
#include "sprintf.h"
#include "cpu.h"
#include "pl180.h"
#include "ascodec-target.h"
#include "adc.h"
#include "storage.h"

#define ON "Enabled"
#define OFF "Disabled"

#define CP15_MMU (1<<0)            /* mmu off/on */
#define CP15_DC  (1<<2)            /* dcache off/on */
#define CP15_IC  (1<<12)           /* icache off/on */

#define CLK_MAIN         24000000     /* 24 MHz  */

#define CLK_PLLA             0
#define CLK_PLLB             1
#define CLK_922T             2
#define CLK_FCLK             3
#define CLK_EXTMEM           4
#define CLK_PCLK             5
#define CLK_IDE              6
#define CLK_I2C              7
#define CLK_I2SI             8
#define CLK_I2SO             9
#define CLK_DBOP             10
#define CLK_SD_MCLK_NAND     11
#define CLK_SD_MCLK_MSD      12
#define CLK_USB              13

#define I2C2_CPSR0      *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x1C))
#define I2C2_CPSR1      *((volatile unsigned int *)(I2C_AUDIO_BASE + 0x20))
#define MCI_NAND        *((volatile unsigned long *)(NAND_FLASH_BASE + 0x04))
#define MCI_SD          *((volatile unsigned long *)(SD_MCI_BASE + 0x04))


/* FIXME: target tree is including ./debug-target.h rather than the one in
 * sansa-fuze/, even though deps contains the correct one
 * if I put the below into a sansa-fuze/debug-target.h, it doesn't work*/
#if defined(SANSA_FUZE) || defined(SANSA_E200V2) || defined(SANSA_C200V2)
#define DEBUG_DBOP
unsigned short button_dbop_data(void);
#endif

static inline unsigned read_cp15 (void)
{
    unsigned cp15_value;
    asm volatile (
        "mrc p15, 0, %0, c1, c0, 0   @ read control reg\n" : "=r"(cp15_value));
    return (cp15_value);
}

int calc_freq(int clk)
{
    int out_div;
    unsigned int prediv = ((unsigned int)CGU_PROC>>2) & 0x3;
    unsigned int postdiv = ((unsigned int)CGU_PROC>>4) & 0xf;

    switch(clk) {
        /* clk_main = clk_int = 24MHz oscillator */
        case CLK_PLLA:
            if(CGU_PLLASUP & (1<<3))
                return 0;

                 /*assume 24MHz oscillator only input available */
            out_div = ((CGU_PLLA>>13) & 0x3);         /* bits 13:14 */
            if (out_div == 3)                         /* for 11 NO=4  */
                out_div=4;
            if(out_div)                               /*  NO = 0 not allowed */
                return ((2 * (CGU_PLLA & 0xff))*CLK_MAIN)/
                                               (((CGU_PLLA>>8) & 0x1f)*out_div);
            return 0;
       case CLK_PLLB:
            if(CGU_PLLBSUP & (1<<3))
                return 0;

                 /*assume 24MHz oscillator only input available */
            out_div = ((CGU_PLLB>>13) & 0x3);         /* bits 13:14 */
            if (out_div == 3)                         /* for 11 NO=4  */
                out_div=4;
            if(out_div)                               /*  NO = 0 not allowed */
                return ((2 * (CGU_PLLB & 0xff))*CLK_MAIN)/
                                               (((CGU_PLLB>>8) & 0x1f)*out_div);
            return 0;
        case CLK_922T:
            if (!(read_cp15()>>30))                 /* fastbus */
                return calc_freq(CLK_PCLK);
            else                                    /* Synch or Asynch bus*/
                return calc_freq(CLK_FCLK);
        case CLK_FCLK:
            switch(CGU_PROC & 3) {
                case 0:
                    return (CLK_MAIN * (8 - prediv)) / (8 * (postdiv + 1));
                case 1:
                    return (calc_freq(CLK_PLLA) * (8 - prediv)) /
                                                            (8 * (postdiv + 1));
                case 2:
                    return (calc_freq(CLK_PLLB) * (8 - prediv)) /
                                                            (8 * (postdiv + 1));
                default:
                    return 0;
            }
        case CLK_EXTMEM:
            switch(CGU_PERI & 3) {
                case 0:
                    return CLK_MAIN/(((CGU_PERI>>2)& 0xf)+1);
                case 1:
                    return calc_freq(CLK_PLLA)/(((CGU_PERI>>2)& 0xf)+1);
                case 2:
                    return calc_freq(CLK_PLLB)/(((CGU_PERI>>2)& 0xf)+1);
                case 3:
                    return calc_freq(CLK_FCLK)/(((CGU_PERI>>2)& 0xf)+1);
                default:
                    return 0;
            }
        case CLK_PCLK:
            return calc_freq(CLK_EXTMEM)/(((CGU_PERI>>6)& 0x1)+1);
        case CLK_IDE:
            switch(CGU_IDE & 3) {
                case 0:
                    return CLK_MAIN/(((CGU_IDE>>2)& 0xf)+1);
                case 1:
                    return calc_freq(CLK_PLLA)/(((CGU_IDE>>2)& 0xf)+1);
                case 2:
                    return calc_freq(CLK_PLLB)/(((CGU_IDE>>2)& 0xf)+1);
                default:
                    return 0;
            }
        case CLK_I2C:
            return calc_freq(CLK_PCLK)/(I2C2_CPSR1<<8 | I2C2_CPSR0);
        case CLK_I2SI:
            switch((CGU_AUDIO>>12) & 3) {
                case 0:
                    return CLK_MAIN/(((CGU_AUDIO>>14) & 0x1ff)+1);
                case 1:
                    return calc_freq(CLK_PLLA)/(((CGU_AUDIO>>14) & 0x1ff)+1);
                case 2:
                    return calc_freq(CLK_PLLB)/(((CGU_AUDIO>>14) & 0x1ff)+1);
                default:
                    return 0;
            }
        case CLK_I2SO:
            switch(CGU_AUDIO & 3) {
                case 0:
                    return CLK_MAIN/(((CGU_AUDIO>>2) & 0x1ff)+1);
                case 1:
                    return calc_freq(CLK_PLLA)/(((CGU_AUDIO>>2) & 0x1ff)+1);
                case 2:
                    return calc_freq(CLK_PLLB)/(((CGU_AUDIO>>2) & 0x1ff)+1);
                default:
                    return 0;
            }
        case CLK_DBOP:
            return calc_freq(CLK_PCLK)/((CGU_DBOP & 7)+1);
        case CLK_SD_MCLK_NAND:
            if(!(MCI_NAND & (1<<8)))
                return 0;
            else if(MCI_NAND & (1<<10))
                return calc_freq(CLK_PCLK);
            else
                return calc_freq(CLK_PCLK)/(((MCI_NAND & 0xff)+1)*2);
        case CLK_SD_MCLK_MSD:
            if(!(MCI_SD & (1<<8)))
                return 0;
            else if(MCI_SD & (1<<10))
                return calc_freq(CLK_PCLK);
            else
            return calc_freq(CLK_PCLK)/(((MCI_SD & 0xff)+1)*2);
        case CLK_USB:
            switch(CGU_USB & 3) {     /* 0-> div=1  other->div=1/(2*n)  */
                case 0:
                    if (!((CGU_USB>>2) & 0xf))
                        return CLK_MAIN;
                    else
                        return CLK_MAIN/(2*((CGU_USB>>2) & 0xf));
                case 1:
                    if (!((CGU_USB>>2) & 0xf))
                        return calc_freq(CLK_PLLA);
                    else
                        return calc_freq(CLK_PLLA)/(2*((CGU_USB>>2) & 0xf));
                case 2:
                    if (!((CGU_USB>>2) & 0xf))
                        return calc_freq(CLK_PLLB);
                    else
                        return calc_freq(CLK_PLLB)/(2*((CGU_USB>>2) & 0xf));
                default:
                    return 0;
            }
        default:
            return 0;
        }
}

bool __dbg_hw_info(void)
{
    int line;
    int last_nand = 0;
#ifdef HAVE_MULTIDRIVE
    int last_sd = 0;
#endif

    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        while(1)
        {
        lcd_clear_display();
        line = 0;
        lcd_puts(0, line++, "[Clock Frequencies:]");
        lcd_puts(0, line++, "      SET       ACTUAL");
        lcd_putsf(0, line++, "922T:%s     %3dMHz",
                                        (!(read_cp15()>>30)) ? "FAST " :
                                        (read_cp15()>>31) ? "ASYNC" : "SYNC ",
                                         calc_freq(CLK_922T)/1000000);
        lcd_putsf(0, line++, "PLLA:%3dMHz    %3dMHz", AS3525_PLLA_FREQ/1000000,
                                                   calc_freq(CLK_PLLA)/1000000);
        lcd_putsf(0, line++, "PLLB:          %3dMHz", calc_freq(CLK_PLLB)/1000000);
        lcd_putsf(0, line++, "FCLK:          %3dMHz", calc_freq(CLK_FCLK)/1000000);

#if LCD_HEIGHT < 176  /* clip  */
        lcd_update();
        int btn = button_get_w_tmo(HZ/10);
        if(btn == (DEBUG_CANCEL|BUTTON_REL))
            goto end;
        else if(btn == (BUTTON_DOWN|BUTTON_REL))
            break;
        }
        while(1)
        {
        lcd_clear_display();
        line = 0;
#endif  /*  LCD_HEIGHT < 176 */

        lcd_putsf(0, line++, "DRAM:%3dMHz    %3dMHz", AS3525_PCLK_FREQ/1000000,
                                                 calc_freq(CLK_EXTMEM)/1000000);
        lcd_putsf(0, line++, "PCLK:%3dMHz    %3dMHz", AS3525_PCLK_FREQ/1000000,
                                                   calc_freq(CLK_PCLK)/1000000);
        lcd_putsf(0, line++, "IDE :%3dMHz    %3dMHz", AS3525_IDE_FREQ/1000000,
                                                    calc_freq(CLK_IDE)/1000000);
        lcd_putsf(0, line++, "DBOP:%3dMHz    %3dMHz", AS3525_DBOP_FREQ/1000000,
                                                   calc_freq(CLK_DBOP)/1000000);
        lcd_putsf(0, line++, "I2C :%3dkHz    %3dkHz", AS3525_I2C_FREQ/1000,
                                                       calc_freq(CLK_I2C)/1000);
        lcd_putsf(0, line++, "I2SI: %s      %3dMHz", (CGU_AUDIO & (1<<23)) ?
                                   "on " : "off" , calc_freq(CLK_I2SI)/1000000);

#if LCD_HEIGHT < 176  /* clip  */
        lcd_update();
        int btn = button_get_w_tmo(HZ/10);
        if(btn == (DEBUG_CANCEL|BUTTON_REL))
            goto end;
        else if(btn == (BUTTON_DOWN|BUTTON_REL))
            break;
        }
        while(1)
        {
        lcd_clear_display();
        line = 0;
#endif  /*  LCD_HEIGHT < 176 */

        lcd_putsf(0, line++, "I2SO: %s      %3dMHz", (CGU_AUDIO & (1<<11)) ?
                                    "on " : "off", calc_freq(CLK_I2SO)/1000000);

        /* Enable SD cards to read the registers */
        sd_enable(true);
        last_nand = MCI_NAND;
#ifdef HAVE_MULTIDRIVE
        last_sd = MCI_SD;
#endif
        sd_enable(false);

        lcd_putsf(0, line++, "SD  :%3dMHz    %3dMHz",
            ((AS3525_PCLK_FREQ/ 1000000) /
            ((last_nand & MCI_CLOCK_BYPASS)? 1:(((last_nand & 0xff)+1) * 2))),
            calc_freq(CLK_SD_MCLK_NAND)/1000000);
#ifdef HAVE_MULTIDRIVE
        lcd_putsf(0, line++, "uSD :%3dMHz    %3dMHz",
            ((AS3525_PCLK_FREQ/ 1000000) /
            ((last_sd & MCI_CLOCK_BYPASS) ? 1: (((last_sd & 0xff) + 1) * 2))),
            calc_freq(CLK_SD_MCLK_MSD)/1000000);
#endif
        lcd_putsf(0, line++, "USB :          %3dMHz", calc_freq(CLK_USB)/1000000);
        lcd_putsf(0, line++, "MMU :  %s CVDDP:%4d", (read_cp15() & CP15_MMU) ?
                                        " on" : "off", adc_read(ADC_CVDD) * 25);
        lcd_putsf(0, line++, "Icache:%s Dcache:%s",
                                      (read_cp15() & CP15_IC)  ? " on" : "off",
                                      (read_cp15() & CP15_DC)  ? " on" : "off");

        lcd_update();
        int btn = button_get_w_tmo(HZ/10);
        if(btn == (DEBUG_CANCEL|BUTTON_REL))
            goto end;
        else if(btn == (BUTTON_DOWN|BUTTON_REL))
            break;
        }
        while(1)
        {
        lcd_clear_display();
        line = 0;

        lcd_putsf(0, line++, "CGU_PLLA  :%8x", (unsigned int)(CGU_PLLA));
        lcd_putsf(0, line++, "CGU_PLLB  :%8x", (unsigned int)(CGU_PLLB));
        lcd_putsf(0, line++, "CGU_PROC  :%8x", (unsigned int)(CGU_PROC));
        lcd_putsf(0, line++, "CGU_PERI  :%8x", (unsigned int)(CGU_PERI));
        lcd_putsf(0, line++, "CGU_IDE   :%8x", (unsigned int)(CGU_IDE));
        lcd_putsf(0, line++, "CGU_DBOP  :%8x", (unsigned int)(CGU_DBOP));

#if LCD_HEIGHT < 176  /* clip  */
        lcd_update();
        int btn = button_get_w_tmo(HZ/10);
        if(btn == (DEBUG_CANCEL|BUTTON_REL))
            goto end;
        else if(btn == (BUTTON_DOWN|BUTTON_REL))
            break;
        }
        while(1)
        {
        lcd_clear_display();
        line = 0;
#endif  /*  LCD_HEIGHT < 176 */

        lcd_putsf(0, line++, "CGU_AUDIO :%8x", (unsigned int)(CGU_AUDIO));
        lcd_putsf(0, line++, "CGU_USB   :%8x", (unsigned int)(CGU_USB));
        lcd_putsf(0, line++, "I2C2_CPSR :%8x", (unsigned int)(I2C2_CPSR1<<8 |
                                                       I2C2_CPSR0));
        lcd_putsf(0, line++, "MCI_NAND  :%8x", (unsigned int)(MCI_NAND));
        lcd_putsf(0, line++, "MCI_SD    :%8x", (unsigned int)(MCI_SD));

        lcd_update();
        int btn = button_get_w_tmo(HZ/10);
        if(btn == (DEBUG_CANCEL|BUTTON_REL))
            goto end;
        else if(btn == (BUTTON_DOWN|BUTTON_REL))
            break;
        }
    }

end:
    lcd_setfont(FONT_UI);
    return false;
}

bool __dbg_ports(void)
{
    int line;

    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        line = 0;
        lcd_puts(0, line++, "[GPIO Values and Directions]");
        lcd_putsf(0, line++, "GPIOA: %2x DIR: %2x", GPIOA_DATA, GPIOA_DIR);
        lcd_putsf(0, line++, "GPIOB: %2x DIR: %2x", GPIOB_DATA, GPIOB_DIR);
        lcd_putsf(0, line++, "GPIOC: %2x DIR: %2x", GPIOC_DATA, GPIOC_DIR);
        lcd_putsf(0, line++, "GPIOD: %2x DIR: %2x", GPIOD_DATA, GPIOD_DIR);
#ifdef DEBUG_DBOP
        line++;
        lcd_puts(0, line++, "[DBOP_DIN]");
        lcd_putsf(0, line++, "DBOP_DIN: %4x", button_dbop_data());
#endif
        line++;
        lcd_puts(0, line++, "[CP15]");
        lcd_putsf(0, line++, "CP15: 0x%8x", read_cp15());
        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
            break;
    }
    lcd_setfont(FONT_UI);
    return false;
}
