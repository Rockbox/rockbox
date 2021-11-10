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
#include "system.h"
#include "kernel.h"
#include "action.h"
#include "lcd.h"
#include "font.h"
#include "cpu.h"
#include "pl180.h"
#include "ascodec.h"
#include "adc.h"
#include "storage.h"

#define DEBUG_CANCEL       ACTION_STD_CANCEL
#define DEBUG_NEXT         ACTION_STD_NEXT
#define DEBUG_LEFT_JUSTIFY ACTION_STD_OK
#define DEBUG_LEFT_SCROLL  ACTION_STD_MENU

/* if the possiblity exists to divide by zero protect with this macro */
#define DIV_FINITE(dividend, divisor) ((divisor == 0)? divisor : dividend/divisor)

#define ON "Enabled"
#define OFF "Disabled"

#define CP15_MMU (1<<0)            /* mmu off/on */
#define CP15_DC  (1<<2)            /* dcache off/on */
#define CP15_IC  (1<<12)           /* icache off/on */

#define CLK_MAIN         24000000     /* 24 MHz  */
#define D_MHZ            1000000
#define D_KHZ            1000

enum dbg_clocks
{
    CLK_PLLA = 0x0,
    CLK_PLLB,
    CLK_PROC,
    CLK_FCLK,
    CLK_EXTMEM,
    CLK_PCLK,
    CLK_IDE,
    CLK_I2C,
    CLK_I2SI,
    CLK_I2SO,
    CLK_DBOP,
    CLK_SD_MCLK_NAND,
    CLK_SD_MCLK_MSD,
    CLK_SD_SLOT,
    CLK_USB,
    CLK_SSP,
};

#define MCI_NAND        *((volatile unsigned long *)(NAND_FLASH_BASE + 0x04))
#define MCI_SD          *((volatile unsigned long *)(SD_MCI_BASE + 0x04))

#if defined(SANSA_FUZE) || defined(SANSA_E200V2) || defined(SANSA_C200V2)
#define DEBUG_DBOP
#include "dbop-as3525.h"
#endif

static inline unsigned read_cp15 (void)
{
    unsigned cp15_value;
    asm volatile (
        "mrc p15, 0, %0, c1, c0, 0   @ read control reg\n" : "=r"(cp15_value));
    return (cp15_value);
}

static int calc_freq(int clk)
{
    unsigned int prediv = ((unsigned int)CGU_PROC>>2) & 0x3;
    unsigned int postdiv = ((unsigned int)CGU_PROC>>4) & 0xf;
    unsigned int u_out_div;
#if CONFIG_CPU == AS3525

    switch(clk) {
        /* clk_main = clk_int = 24MHz oscillator */
        case CLK_PLLA:
            if(CGU_PLLASUP & (1<<3))
                return 0;

                 /*assume 24MHz oscillator only input available */
            u_out_div = ((CGU_PLLA>>13) & 0x3);         /* bits 13:14 */
            if (u_out_div == 3)                         /* for 11 NO=4  */
                u_out_div=4;
            /*  NO = 0 not allowed */
            return DIV_FINITE(((2 * (CGU_PLLA & 0xff))*CLK_MAIN),
                                  (((CGU_PLLA>>8) & 0x1f)*u_out_div));
       case CLK_PLLB:
            if(CGU_PLLBSUP & (1<<3))
                return 0;

                 /*assume 24MHz oscillator only input available */
            u_out_div = ((CGU_PLLB>>13) & 0x3);         /* bits 13:14 */
            if (u_out_div == 3)                         /* for 11 NO=4  */
                u_out_div=4;
            /*  NO = 0 not allowed */
            return DIV_FINITE(((2 * (CGU_PLLB & 0xff))*CLK_MAIN),
                                  (((CGU_PLLB>>8) & 0x1f)*u_out_div));
#else /* AS3525v2 */
    int od, f, r;

    /* AS3525v2  */
    switch(clk) {
        case CLK_PLLA:
            if(CGU_PLLASUP & (1<<3))
                return 0;

            f = (CGU_PLLA & 0x7F) + 1;
            r = ((CGU_PLLA >> 7) & 0x7) + 1;
            od = (CGU_PLLA >> 10) & 1 ? 2 : 1;
            return (CLK_MAIN / 2) * f / (r * od);

        case CLK_PLLB:
            if(CGU_PLLBSUP & (1<<3))
                return 0;

            f = (CGU_PLLB & 0x7F) + 1;
            r = ((CGU_PLLB >> 7) & 0x7) + 1;
            od = (CGU_PLLB >> 10) & 1 ? 2 : 1;
            return (CLK_MAIN / 2) * f / (r * od);

        case CLK_SD_SLOT:
            switch(CGU_SDSLOT & 3) {
                case 0:
                    return CLK_MAIN/(((CGU_SDSLOT>>2)& 0xf)+1);
                case 1:
                    return calc_freq(CLK_PLLA)/(((CGU_SDSLOT>>2)& 0xf)+1);
                case 2:
                    return calc_freq(CLK_PLLB)/(((CGU_SDSLOT>>2)& 0xf)+1);
                default:
                    return 0;
            }
#endif /* CONFIG_CPU == AS3525 */

        case CLK_PROC:
#if CONFIG_CPU == AS3525 /* not in arm926-ejs */
            if (!(read_cp15()>>30))                 /* fastbus */
                return calc_freq(CLK_PCLK);
            else                                    /* Synch or Asynch bus*/
#endif /* CONFIG_CPU == AS3525 */
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
#if CONFIG_CPU == AS3525
            switch(CGU_PERI & 3) {
#else /* as3525v2 */
            /* bits 1:0 of CGU_PERI always read as 0 and source = FCLK */
            switch(3) {
#endif /* CONFIG_CPU == AS3525 */
                case 0:
                    return CLK_MAIN/(((CGU_PERI>>2)& 0xf)+1);
                case 1:
                    return calc_freq(CLK_PLLA)/(((CGU_PERI>>2)& 0xf)+1);
                case 2:
                    return calc_freq(CLK_PLLB)/(((CGU_PERI>>2)& 0xf)+1);
                case 3:
                default:
                    return calc_freq(CLK_FCLK)/(((CGU_PERI>>2)& 0xf)+1);
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
            ams_i2c_get_debug_cpsr(&u_out_div);
            /*  NO = 0 not allowed */
            return DIV_FINITE(calc_freq(CLK_PCLK), (u_out_div));
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
#if CONFIG_CPU == AS3525
        case CLK_SD_MCLK_NAND:
            if(!(MCI_NAND & (1<<8)))
                return 0;
            else if(MCI_NAND & (1<<10))
                return calc_freq(CLK_IDE);
            else
                return calc_freq(CLK_IDE)/(((MCI_NAND & 0xff)+1)*2);
        case CLK_SD_MCLK_MSD:
            if(!(MCI_SD & (1<<8)))
                return 0;
            else if(MCI_SD & (1<<10))
                return calc_freq(CLK_PCLK);
            else
            return calc_freq(CLK_PCLK)/(((MCI_SD & 0xff)+1)*2);
#else /* as3525v2 */
        case CLK_SSP:
            /*  NO = 0 not allowed */
            return DIV_FINITE(calc_freq(CLK_PCLK), SSP_CPSR);
#endif /* CONFIG_CPU == AS3525 */

        case CLK_USB:
            switch(CGU_USB & 3) {     /* 0-> div=1  other->div=1/(2*n)  */
                case 0:
                    if (!((CGU_USB>>2) & 0x7))
                        return CLK_MAIN;
                    else
                        return CLK_MAIN/(2*((CGU_USB>>2) & 0x7));
                case 1:
                    if (!((CGU_USB>>2) & 0x7))
                        return calc_freq(CLK_PLLA);
                    else
                        return calc_freq(CLK_PLLA)/(2*((CGU_USB>>2) & 0x7));
                case 2:
                    if (!((CGU_USB>>2) & 0x7))
                        return calc_freq(CLK_PLLB);
                    else
                        return calc_freq(CLK_PLLB)/(2*((CGU_USB>>2) & 0x7));
                default:
                    return 0;
            }
        default:
            return 0;
        }
}

static bool dbg_btn(bool *done, int *x)
{
    bool cont = !*done;
    if (cont)
    {
        lcd_update();
        int button = get_action(CONTEXT_STD,HZ/10);
        switch(button)
        {
            case DEBUG_CANCEL:
                *done = true;
                /*fallthrough*/
            case DEBUG_NEXT:
                cont = false;
                /*fallthrough*/
            case DEBUG_LEFT_JUSTIFY:
                (*x) = 0;
                sleep(HZ/5);
                break;
            case DEBUG_LEFT_SCROLL:
                (*x)--;
                break;
            default:
                break;
        }
    }
    lcd_clear_display();
    return cont;
}

bool dbg_hw_info(void)
{
    int line;
    int x = 0;
    bool done = false;

    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(!done)
    {
        while(dbg_btn(&done, &x))
        {
#if defined(SANSA_C200V2) || defined(SANSA_FUZEV2) || \
    defined(SANSA_CLIPPLUS) || defined(SANSA_CLIPZIP)

        line = 0;
        lcd_puts(x, line++, "[Submodel:]");
#if defined(SANSA_C200V2)
        lcd_putsf(x, line++, "C200v2 variant %d", c200v2_variant);
#elif defined(SANSA_FUZEV2) || defined(SANSA_CLIPPLUS) || \
      defined(SANSA_CLIPZIP)
        lcd_putsf(x, line++, "AMSv2 variant %d", amsv2_variant);
#endif
        }
        while(dbg_btn(&done, &x))
        {
#endif
        line = 0;
        lcd_puts(x, line++, "[Clock Frequencies:]");
        lcd_puts(x, line++, "     SET       ACTUAL");
#if CONFIG_CPU == AS3525
        lcd_putsf(x, line++, "922T:%s     %3dMHz",
                                        (!(read_cp15()>>30)) ? "FAST " :
                                        (read_cp15()>>31) ? "ASYNC" : "SYNC ",
#else /* as3525v2 */
        lcd_putsf(x, line++, "926ejs:        %3dMHz",
#endif
                                         calc_freq(CLK_PROC)/D_MHZ);
        lcd_putsf(x, line++, "%s:%3dMHz    %3dMHz", "PLLA", AS3525_PLLA_FREQ/D_MHZ,
                                                   calc_freq(CLK_PLLA)/D_MHZ);
        lcd_putsf(x, line++, "PLLB:          %3dMHz", calc_freq(CLK_PLLB)/D_MHZ);
        lcd_putsf(x, line++, "FCLK:          %3dMHz", calc_freq(CLK_FCLK)/D_MHZ);
        lcd_putsf(x, line++, "%s:%3dMHz    %3dMHz", "DRAM", AS3525_PCLK_FREQ/D_MHZ,
                                                 calc_freq(CLK_EXTMEM)/D_MHZ);
        lcd_putsf(x, line++, "%s:%3dMHz    %3dMHz", "PCLK", AS3525_PCLK_FREQ/D_MHZ,
                                                   calc_freq(CLK_PCLK)/D_MHZ);

#if LCD_HEIGHT < 176  /* clip  */
        }
        while(dbg_btn(&done, &x))
        {
        line = 0;
#endif  /*  LCD_HEIGHT < 176 */

        lcd_putsf(x, line++, "%s:%3dMHz    %3dMHz", "IDE ", AS3525_IDE_FREQ/D_MHZ,
                                                    calc_freq(CLK_IDE)/D_MHZ);
        lcd_putsf(x, line++, "%s:%3dMHz    %3dMHz", "DBOP", AS3525_DBOP_FREQ/D_MHZ,
                                                   calc_freq(CLK_DBOP)/D_MHZ);
        lcd_putsf(x, line++, "%s:%3dkHz    %3dkHz", "I2C ", AS3525_I2C_FREQ/D_KHZ,
                                                       calc_freq(CLK_I2C)/D_KHZ);
        lcd_putsf(x, line++, "I2SI: %s      %3dMHz", (CGU_AUDIO & (1<<23)) ?
                                   "on " : "off" , calc_freq(CLK_I2SI)/D_MHZ);
        lcd_putsf(x, line++, "I2SO: %s      %3dMHz", (CGU_AUDIO & (1<<11)) ?
                                    "on " : "off", calc_freq(CLK_I2SO)/D_MHZ);
#if CONFIG_CPU == AS3525
        struct ams_sd_debug_info dbg;
        ams_sd_get_debug_info(&dbg);

        lcd_putsf(x, line++, "%s:%3dMHz    %3dMHz", "SD  ",
            ((AS3525_IDE_FREQ/ D_MHZ) /
            ((dbg.mci_nand & MCI_CLOCK_BYPASS)? 1:(((dbg.mci_nand & 0xff)+1) * 2))),
            calc_freq(CLK_SD_MCLK_NAND)/D_MHZ);
#ifdef HAVE_MULTIDRIVE
        lcd_putsf(x, line++, "%s:%3dMHz    %3dMHz", "uSD ",
            ((AS3525_PCLK_FREQ/ D_MHZ) /
            ((dbg.mci_sd & MCI_CLOCK_BYPASS) ? 1: (((dbg.mci_sd & 0xff) + 1) * 2))),
            calc_freq(CLK_SD_MCLK_MSD)/D_MHZ);
#endif

#else /*AS3525v2*/
        lcd_putsf(x, line++, "%s:%3dMHz    %3dMHz", "SD  ", AS3525_SDSLOT_FREQ/D_MHZ,
                                                        calc_freq(CLK_SD_SLOT)/D_MHZ);
        long freq_ssp = calc_freq(CLK_SSP);
        if (freq_ssp >= D_MHZ)
            lcd_putsf(x, line++, "%s:%3dMHz    %3dMHz", "SSP ", AS3525_SSP_FREQ/D_MHZ,
                                                    calc_freq(CLK_SSP)/D_MHZ);
        else
            lcd_putsf(x, line++, "SSP :%3dMHz    %3dkHz", AS3525_SSP_FREQ/D_MHZ,
                                                    calc_freq(CLK_SSP)/D_KHZ);
#endif  /* CONFIG_CPU == AS3525 */
        lcd_putsf(x, line++, "USB :          %3dMHz", calc_freq(CLK_USB)/D_MHZ);

#if LCD_HEIGHT < 176  /* clip  */
        }
        while(dbg_btn(&done, &x))
        {
        lcd_clear_display();
        line = 0;
#endif  /*  LCD_HEIGHT < 176 */

        lcd_putsf(x, line++, "MMU :  %s CVDDP:%4d", (read_cp15() & CP15_MMU) ?
                                        " on" : "off", adc_read(ADC_CVDD) * 25);
        lcd_putsf(x, line++, "Icache:%s Dcache:%s",
                                      (read_cp15() & CP15_IC)  ? " on" : "off",
                                      (read_cp15() & CP15_DC)  ? " on" : "off");
        }
        while(dbg_btn(&done, &x))
        {
        line = 0;

        lcd_putsf(x, line++, "CGU_PLLA  :%8x", (unsigned int)(CGU_PLLA));
        lcd_putsf(x, line++, "CGU_PLLB  :%8x", (unsigned int)(CGU_PLLB));
        lcd_putsf(x, line++, "CGU_PROC  :%8x", (unsigned int)(CGU_PROC));
        lcd_putsf(x, line++, "CGU_PERI  :%8x", (unsigned int)(CGU_PERI));
        lcd_putsf(x, line++, "CGU_IDE   :%8x", (unsigned int)(CGU_IDE));
        lcd_putsf(x, line++, "CGU_DBOP  :%8x", (unsigned int)(CGU_DBOP));
        lcd_putsf(x, line++, "CGU_AUDIO :%8x", (unsigned int)(CGU_AUDIO));
        lcd_putsf(x, line++, "CGU_USB   :%8x", (unsigned int)(CGU_USB));

#if LCD_HEIGHT < 176  /* clip  */
        }
        while(dbg_btn(&done, &x))
        {
        line = 0;
#endif  /*  LCD_HEIGHT < 176 */

        unsigned int i2c_cpsr;
        ams_i2c_get_debug_cpsr(&i2c_cpsr);
        lcd_putsf(x, line++, "I2C2_CPSR :%8x", i2c_cpsr);
#if CONFIG_CPU == AS3525
        lcd_putsf(x, line++, "MCI_NAND  :%8x", (unsigned int)(MCI_NAND));
        lcd_putsf(x, line++, "MCI_SD    :%8x", (unsigned int)(MCI_SD));
#else /* as3525v2 */
        lcd_putsf(x, line++, "CGU_MEMSTK:%8x", (unsigned int)(CGU_MEMSTICK));
        lcd_putsf(x, line++, "CGU_SDSLOT:%8x", (unsigned int)(CGU_SDSLOT));
#endif
        lcd_putsf(x, line++, "SSP_CPSR  :%8x", (unsigned int)(SSP_CPSR));
        }
    }

    lcd_setfont(FONT_UI);
    return false;
}

#if CONFIG_CPU == AS3525v2
void adc_set_voltage_mux(int channel)
{
    ascodec_lock();
    /*this register also controls which subregister is subsequently written, so be careful*/
    ascodec_write(AS3543_PMU_ENABLE, 8 | channel << 4 );
    ascodec_unlock();
}
#endif

bool dbg_ports(void)
{
    int line, i;
    int x = 0;
    bool done = false;
    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(!done)
    {
        while(dbg_btn(&done, &x))
        {
            line = 0;
            lcd_puts(x, line++, "[GPIO Vals and Dirs]");
            lcd_putsf(x, line++, "%s: %2x DIR: %2x", "GPIOA", GPIOA_DATA, GPIOA_DIR);
            lcd_putsf(x, line++, "%s: %2x DIR: %2x", "GPIOB", GPIOB_DATA, GPIOB_DIR);
            lcd_putsf(x, line++, "%s: %2x DIR: %2x", "GPIOC", GPIOC_DATA, GPIOC_DIR);
            lcd_putsf(x, line++, "%s: %2x DIR: %2x", "GPIOD", GPIOD_DATA, GPIOD_DIR);
            lcd_putsf(x, line++, "CCU_IO:%8x", CCU_IO);
#ifdef DEBUG_DBOP
            lcd_puts(x, line++, "[DBOP_DIN]");
            lcd_putsf(x, line++, "DBOP_DIN: %4x", dbop_debug());
#endif
            lcd_puts(x, line++, "[CP15]");
            lcd_putsf(x, line++, "CP15: 0x%8x", read_cp15());
        }

#if CONFIG_CPU == AS3525 /* as3525v2 channels are different */
#define BATTEMP_UNIT 5/2 /* 2.5mV */
        static const char *adc_name[13] = {
            "CHG_OUT  ",
            "RTCSUP   ",
            "VBUS     ",
            "CHG_IN   ",
            "CVDD     ",
            "BatTemp  ",
            "MicSup1  ",
            "MicSup2  ",
            "VBE1     ",
            "VBE2     ",
            "I_MicSup1",
            "I_MicSup2",
            "VBAT     ",
        };
#elif CONFIG_CPU == AS3525v2
#define BATTEMP_UNIT 2 /* 2mV */
        static const char *adc_name[16] = {
            "BVDD    ",
            "BVDDR   ",
            "CHGIN   ",
            "CHGOUT  ",
            "VBUS    ",
            NULL,
            "BatTemp ",
            NULL,
            "MicSup  ",
            NULL,
            "I_MiSsup",
            NULL,
            "VBE_1uA ",
            "VBE_2uA ",
            "I_CHGact",
            "I_CHGref",
        };

        static const char *adc_mux_name[10] = {
            NULL,
            "AVDD27  ",
            "AVDD17  ",
            "PVDD1   ",
            "PVDD2   ",
            "CVDD1   ",
            "CVDD2   ",
            "RVDD    ",
            "FVDD    ",
            "PWGD    ",
        };
#endif

        lcd_clear_display();
        while(dbg_btn(&done, &x))
        {
            line = 0;

            for(i=0; i<5; i++)
                lcd_putsf(x, line++, "%s: %d mV", adc_name[i], adc_read(i) * 5);
            for(; i<8; i++)
                if(adc_name[i])
                    lcd_putsf(x, line++, "%s: %d mV", adc_name[i],
                              adc_read(i) * BATTEMP_UNIT);
#if LCD_HEIGHT < 176  /* clip  */
        }
        while(dbg_btn(&done, &x))
        {
            line = 0;
#endif /* LCD_HEIGHT < 176 */
            for(i=8; i<10; i++)
                if(adc_name[i])
                    lcd_putsf(x, line++, "%s: %d mV", adc_name[i], adc_read(i));
            for(; i<12; i++)
                if(adc_name[i])
                    lcd_putsf(x, line++, "%s: %d uA", adc_name[i], adc_read(i));
#if CONFIG_CPU == AS3525 /* different units */
            lcd_putsf(x, line++, "%s: %d mV", adc_name[i], adc_read(i)*5/2);
#elif CONFIG_CPU == AS3525v2
            for(; i<16; i++)
                lcd_putsf(x, line++, "%s: %d mV", adc_name[i], adc_read(i));
#endif
        }
#if CONFIG_CPU == AS3525v2  /*extend AS3543 voltage registers*/
        while(dbg_btn(&done, &x))
        {
            line = 0;
            for(i=1; i<9; i++){
                adc_set_voltage_mux(i); /*change the voltage mux to a new channel*/
                lcd_putsf(x, line++, "%s: %d mV", adc_mux_name[i], adc_read(5) * 5);
            }
        }
#endif
     }

    lcd_setfont(FONT_UI);
    return false;
}

#ifdef HAVE_ADJUSTABLE_CPU_VOLTAGE
/* Return CPU voltage setting in millivolts */
int get_cpu_voltage_setting(void)
{
    int value;

#if CONFIG_CPU == AS3525
    value = ascodec_read(AS3514_CVDD_DCDC3) & 0x3;
    value = 1200 - value * 50;
#else /* as3525v2 */
    value = ascodec_read_pmu(0x17, 1) & 0x7f;

    /* Calculate in 0.1mV steps */
    if (value == 0)
        /* 0 volts */;
    else if (value <= 0x40)
        value = 6000 + value * 125;
    else if (value <= 0x70)
        value = 14000 + (value - 0x40) * 250;
    else if (value <= 0x7f)
        value = 26000 + (value - 0x70) * 500;

    /* Return voltage setting in millivolts */
    value = (value + 5) / 10;
#endif /* CONFIG_CPU */

    return value;
}
#endif /* HAVE_ADJUSTABLE_CPU_VOLTAGE */
