/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Szymon Dziok
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

/*
SANSA VIEW: TESTING CODE
*/


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "config.h"
#include "inttypes.h"
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include "storage.h"
#include "fat.h"
#include "disk.h"
#include "font.h"
#include "backlight.h"
#include "backlight-target.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"

#include "regs/regs-clock.h"
#include "regs/regs-device.h"

asm(
".section .rodata.hwstub,\"ax\",%progbits\n"
".code 32\n"
".global hwstub_start\n"
"hwstub_start:\n"
".incbin \"/home/pamaury/project/rockbox/myrockbox/utils/hwstub/stub/pp/build/hwstub.bin\"\n"
".global hwstub_end\n"
"hwstub_end:\n"
    );

void run_hwstub(void)
{
    extern char hwstub_start[];
    void (*fn)(void) = (void *)&hwstub_start;
    fn();
}

unsigned compute_bogomips(void)
{
#define NOP "nop\n"
#define NOP2 NOP NOP
#define NOP4 NOP2 NOP2
#define NOP8 NOP4 NOP4
#define NOP16 NOP8 NOP8
#define NOP32 NOP16 NOP16
#define NOP64 NOP32 NOP32
#define NOP128 NOP64 NOP64
#define NOP256 NOP128 NOP128
#define NOP512 NOP256 NOP256
#if 0
    unsigned timeout = USEC_TIMER + 1000000;
    unsigned count = 0;
    do
    {
        asm volatile(NOP128);
        count += (128 + 4) * 2;
    }while(USEC_TIMER < timeout);
    return count;
#elif 1
    unsigned long ta = 0, tb = 0, tc = 0;
    asm volatile(
        "ldr    %[ta], [%[reg_usec]]\n"
        NOP512
        "ldr    %[tb], [%[reg_usec]]\n"
        NOP512
        NOP512
        "ldr    %[tc], [%[reg_usec]]\n"
        :[ta]"+r"(ta), [tb]"+r"(tb), [tc]"+r"(tc)
        :[reg_usec]"r"(&USEC_TIMER));
    unsigned long nop512t = (tc - tb) - (tb - ta);
    return 1000000 * 2 * 512 / nop512t;
#else
#define CNT 1024
    unsigned long ta = 0, tb = 0, tc = 0;
    unsigned long cnt = CNT, cnt2 = CNT;
    asm volatile(
        "ldr    %[ta], [%[reg_usec]]\n"
        "0:\n"
        NOP32
        "subs   %[cnt], #1\n"
        "bne    0b\n"
        "ldr    %[tb], [%[reg_usec]]\n"
        "0:\n"
        NOP64
        "subs   %[cnt2], #1\n"
        "bne    0b\n"
        "ldr    %[tc], [%[reg_usec]]\n"
        :[ta]"+r"(ta), [tb]"+r"(tb), [tc]"+r"(tc), [cnt]"+r"(cnt), [cnt2]"+r"(cnt2)
        :[reg_usec]"r"(&USEC_TIMER));
    unsigned long nopt = (tc - tb) - (tb - ta);
    return 1000000LLU * 2 * CNT * 32 / nopt;
#endif
}

void main(void)
{
    system_init();
    kernel_init();
    disable_irq();

    BF_WR(CLOCK_ENABLE, CACHE, 0);
    BF_WR(DEVICE_INIT7, UNK7_0, 0xd5);
    HW_CLOCK_SOURCE = BF_OR2(CLOCK_SOURCE, SELECT_V(SRC1), SRC1_V(24MHz));
    HW_CLOCK_SOURCE2 = BF_OR2(CLOCK_SOURCE2, SELECT_V(SRC1), SRC1_V(24MHz));
    BF_WR(DEVICE_INIT2, PLL, 1);
    BF_WR(DEVICE_INIT4, PLL2, 1);
    BF_WR(CLOCK_PLL_CTRL, RESET, 1);
    BF_WR(CLOCK_PLL_CTRL, RESET, 0);
    BF_WR(CLOCK_PLL_CTRL2, RESET, 1);
    BF_WR(CLOCK_PLL_CTRL2, RESET, 0);
    HW_CLOCK_PLL_CTRL2 = 0x3000007;
    while(BF_RD(CLOCK_PLL_CTRL2, LOCK_STATUS) == 0);
    HW_CLOCK_PLL_CTRL = 0x3010428;
    MLCD_SCLK_DIV |= 0x8888;
    (void)HW_CLOCK_PLL_CTRL;
    HW_CLOCK_SOURCE = BF_OR2(CLOCK_SOURCE, SELECT_V(SRC1), SRC1_V(FAST));
    HW_CLOCK_SOURCE2 = BF_OR2(CLOCK_SOURCE2, SELECT_V(SRC1), SRC1_V(PLL));

    unsigned mips[8];
    for(int i = 0; i < 8; i++)
    {
        HW_CLOCK_SOURCE = BF_OR2(CLOCK_SOURCE, SELECT_V(SRC1), SRC1(i));
        udelay(100);
        mips[i] = compute_bogomips();
    }
    HW_CLOCK_SOURCE = BF_OR2(CLOCK_SOURCE, SELECT_V(SRC1), SRC1_V(FAST));

    lcd_init_device();
    _backlight_init();
    _backlight_on();
    _buttonlight_on();
    lcd_clear_display();
    lcd_putsf(0, 0, "Rockbox on View");
    lcd_set_foreground(LCD_RGBPACK(0xff, 0, 0));
    lcd_putsf(0, 1, "RED");
    lcd_set_foreground(LCD_RGBPACK(0, 0xff, 0));
    lcd_putsf(0, 2, "GREEN");
    lcd_set_foreground(LCD_RGBPACK(0, 0, 0xff));
    lcd_putsf(0, 3, "BLUE");
    lcd_set_foreground(LCD_WHITE);
    for(int i = 0; i < 8; i++)
    {
        lcd_putsf(0, i + 4, "MIPS[%d]: %u KHz", i, mips[i] / 1000);
    }
    lcd_update();
    for(int i = 0; i < 3; i++)
    {
        _buttonlight_on();
        sleep(HZ/2);
        _buttonlight_off();
        sleep(HZ/2);
        lcd_update();
    }
    run_hwstub();
}
