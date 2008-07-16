/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "config.h"
#include "jz4740.h"
#include "backlight.h"
#include "font.h"
#include "lcd.h"
#include "system.h"
#include "mips.h"
#include "button.h"
#include "timefuncs.h"
#include "rtc.h"
#include "common.h"

static void audiotest(void)
{
    __i2s_internal_codec();
    __aic_enable();
    __aic_reset();
    __aic_select_i2s();
    __aic_enable_loopback();
}

#define JZ_NAND_SELECT(n)     (REG_EMC_NFCSR |=  (EMC_NFCSR_NFCE##n | EMC_NFCSR_NFE##n) )
#define JZ_NAND_DESELECT(n)   (REG_EMC_NFCSR &= ~(EMC_NFCSR_NFCE##n | EMC_NFCSR_NFE##n) )

#define NAND_CMD_READ1_00  0x00
#define NAND_CMD_READ_ID1  0x90
#define NAND_CMD_READ_ID2  0x91

#define NANDFLASH_CLE      0x00008000 //PA[15]
#define NANDFLASH_ALE      0x00010000 //PA[16]

#define NANDFLASH_BASE     0xB8000000
#define REG_NAND_DATA      (*((volatile unsigned char *) NANDFLASH_BASE))
#define REG_NAND_CMD       (*((volatile unsigned char *) (NANDFLASH_BASE + NANDFLASH_CLE)))
#define REG_NAND_ADDR      (*((volatile unsigned char *) (NANDFLASH_BASE + NANDFLASH_ALE)))

static void jz_nand_scan_id(void)
{
    unsigned char cData[5];
    unsigned int dwNandID;
    
    REG_EMC_NFCSR = 0;
    
    JZ_NAND_SELECT(1);
    REG_NAND_CMD = NAND_CMD_READ_ID1;
    REG_NAND_ADDR = NAND_CMD_READ1_00;
    cData[0] = REG_NAND_DATA;
    cData[1] = REG_NAND_DATA;
    cData[2] = REG_NAND_DATA;
    cData[3] = REG_NAND_DATA;
    cData[4] = REG_NAND_DATA;
    JZ_NAND_DESELECT(1);
    
    dwNandID = ((cData[0] & 0xff) << 8) | (cData[1] & 0xff);

    printf("NAND Flash 1: 0x%x is found [0x%x 0x%x 0x%x]", dwNandID, cData[2], cData[3], cData[4]);
    
    JZ_NAND_SELECT(2);
    REG_NAND_CMD = NAND_CMD_READ_ID1;
    REG_NAND_ADDR = NAND_CMD_READ1_00;
    cData[0] = REG_NAND_DATA;
    cData[1] = REG_NAND_DATA;
    cData[2] = REG_NAND_DATA;
    cData[3] = REG_NAND_DATA;
    cData[4] = REG_NAND_DATA;
    JZ_NAND_DESELECT(2);
    
    dwNandID = ((cData[0] & 0xff) << 8) | (cData[1] & 0xff);

    printf("NAND Flash 2: 0x%x is found [0x%x 0x%x 0x%x]", dwNandID, cData[2], cData[3], cData[4]);
    
    
    JZ_NAND_SELECT(3);
    REG_NAND_CMD = NAND_CMD_READ_ID1;
    REG_NAND_ADDR = NAND_CMD_READ1_00;
    cData[0] = REG_NAND_DATA;
    cData[1] = REG_NAND_DATA;
    cData[2] = REG_NAND_DATA;
    cData[3] = REG_NAND_DATA;
    cData[4] = REG_NAND_DATA;
    JZ_NAND_DESELECT(3);
    
    dwNandID = ((cData[0] & 0xff) << 8) | (cData[1] & 0xff);

    printf("NAND Flash 3: 0x%x is found [0x%x 0x%x 0x%x]", dwNandID, cData[2], cData[3], cData[4]);
    
    JZ_NAND_SELECT(4);
    REG_NAND_CMD = NAND_CMD_READ_ID1;
    REG_NAND_ADDR = NAND_CMD_READ1_00;
    cData[0] = REG_NAND_DATA;
    cData[1] = REG_NAND_DATA;
    cData[2] = REG_NAND_DATA;
    cData[3] = REG_NAND_DATA;
    cData[4] = REG_NAND_DATA;
    JZ_NAND_DESELECT(4);
    
    dwNandID = ((cData[0] & 0xff) << 8) | (cData[1] & 0xff);

    printf("NAND Flash 4: 0x%x is found [0x%x 0x%x 0x%x]", dwNandID, cData[2], cData[3], cData[4]);
}

static void jz_store_icache(void)
{
    unsigned long start;
    unsigned long end;

    start = KSEG0BASE;
    end = start + CFG_ICACHE_SIZE;
    while(start < end)
    {
        cache_unroll(start, 8);
        start += CFG_CACHELINE_SIZE;
    }
}

int main(void)
{
    cli();
    
    write_c0_status(0x10000400);
    
    memcpy((void *)A_K0BASE, (void *)0x80E00080, 0x20);
    memcpy((void *)(A_K0BASE + 0x180), (void *)0x80E00080, 0x20);
    memcpy((void *)(A_K0BASE + 0x200), (void *)0x80E00080, 0x20);
    
    jz_flush_dcache();
    jz_store_icache();
    
    sti();
    
    kernel_init();
    lcd_init();
    font_init();
    lcd_setfont(FONT_SYSFIXED);
    button_init();
    rtc_init();
    
    backlight_init();
    
    /* To make the Windows say "ding-dong".. */
    REG8(USB_REG_POWER) &= ~USB_POWER_SOFTCONN;

    int touch, btn;
    lcd_clear_display();
    printf("Rockbox bootloader v0.000001");
    jz_nand_scan_id();
    printf("REG_EMC_SACR0: 0x%x", REG_EMC_SACR0 >> EMC_SACR_BASE_BIT);
    printf("REG_EMC_SACR1: 0x%x", REG_EMC_SACR1 >> EMC_SACR_BASE_BIT);
    printf("REG_EMC_SACR2: 0x%x", REG_EMC_SACR2 >> EMC_SACR_BASE_BIT);
    printf("REG_EMC_SACR3: 0x%x", REG_EMC_SACR3 >> EMC_SACR_BASE_BIT);
    printf("REG_EMC_SACR4: 0x%x", REG_EMC_SACR4 >> EMC_SACR_BASE_BIT);
    printf("REG_EMC_DMAR0: 0x%x", REG_EMC_DMAR0 >> EMC_DMAR_BASE_BIT);
    while(1)
    {
        btn = button_read_device(&touch);
        if(btn & BUTTON_VOL_DOWN)
            printf("BUTTON_VOL_DOWN");
        if(btn & BUTTON_MENU)
            printf("BUTTON_MENU");
        if(btn & BUTTON_VOL_UP)
            printf("BUTTON_VOL_UP");
        if(btn & BUTTON_POWER)
            printf("BUTTON_POWER");
        if(button_hold())
            printf("BUTTON_HOLD");
        if(touch != 0)
        {
            lcd_set_foreground(LCD_RGBPACK(touch & 0xFF, (touch >> 8)&0xFF, (touch >> 16)&0xFF));
            lcd_fillrect((touch>>16)-10, (touch&0xFFFF)-10, 20, 20);
            lcd_update();
        }
        /*_printf("%02d/%02d/%04d %02d:%02d:%02d", get_time()->tm_mday, get_time()->tm_mon, get_time()->tm_year,
                                     get_time()->tm_hour, get_time()->tm_min, get_time()->tm_sec);*/
    }
    
    return 0;
}
