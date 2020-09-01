/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2018 by Solomon Peachy
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

#include "config.h"
#include "system.h"
#include "cpu.h"
#include <stdarg.h>
#include <stdio.h>
#include "lcd.h"
#include "kernel.h"
#include "font.h"
#include "button.h"
#include "timefuncs.h"

#ifndef BOOTLOADER
#include "action.h"
#include "list.h"

static bool is_done = false;
static int line = 0;
static int cur_line = 0;
static int maxlines = 0;

/*
 * Clock Generation Module
 */
#define TO_MHZ(x) ((x)/1000000), ((x)%1000000)/10000
#define TO_KHZ(x) ((x)/1000), ((x)%1000)/10

#define DEBUG_CANCEL       ACTION_STD_CANCEL
#define DEBUG_NEXT         ACTION_STD_NEXT
#define DEBUG_LEFT_JUSTIFY ACTION_STD_OK
#define DEBUG_LEFT_SCROLL  ACTION_STD_MENU

/* if the possiblity exists to divide by zero protect with this macro */
#define DIV_FINITE(dividend, divisor) ((divisor == 0)? divisor : dividend/divisor)

#define ON "Enabled"
#define OFF "Disabled"

static int hw_info_debug_menu(void);

static bool dbg_btn(bool *done, int *x)
{
    bool cont = !*done;
    if (cont)
    {
        int button = get_action(CONTEXT_STD,HZ/10);
        switch(button)
        {
            case DEBUG_CANCEL:
                *done = true;

            case DEBUG_NEXT:
                cont = false;
            case DEBUG_LEFT_JUSTIFY:
                if (x){(*x) = 0;}
                sleep(HZ/5);
                break;
            case DEBUG_LEFT_SCROLL:
                if(x){(*x)--;}
                break;
        }
    }
    return cont;
}

static bool dbg_btn_update(bool *done, int *x)
{
    bool cont = !*done;
    if (cont)
    {
        lcd_update();
        cont = dbg_btn(done, x);
    }
    lcd_clear_display();
    return cont;
}

static inline unsigned int read_cp0_15 (void)
{
    /* CPUID Cp0 Register 15 Select 0 */
    unsigned int cp_val;
    asm volatile("mfc0 %0, $15\n" : "=r" (cp_val));
    return (cp_val);
}

#define DBG_PUTSF(n, x, ln, ...) do {     \
        if(--lines_left && cur_line < n){\
            lcd_putsf(x, ln, __VA_ARGS__);\
        }                                 \
} while(0)

static bool display_clocks(void)
{
    unsigned int cppcr0 = REG_CPM_CPPCR0;  /* PLL Control Register */
    unsigned int cppcr1 = REG_CPM_CPPCR1;  /* PLL Control Register */
    unsigned int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};
    unsigned int od[4] = {1, 2, 4, 8};

    int lines_left;
    cur_line = 0;
    int x = 0;

    while(!is_done)
    {
        lcd_clear_display();
        cur_line += maxlines / 2;
        while(dbg_btn(&is_done, &x))
        {
            lcd_update();
            lcd_clear_display();
            line = 0;
            lines_left = maxlines;
            DBG_PUTSF(0, x, line++, "CPPCR0:0x%08x", cppcr0);
            if (cppcr0 & CPPCR0_PLLEN) {
                DBG_PUTSF(1, x, line++, "PLL0  :%3d.%02d MHz", TO_MHZ(__cpm_get_pllout()));
                DBG_PUTSF(2, x, line++, "m:n:o :%d:%d:%d",
                        __cpm_get_pllm(),
                        __cpm_get_plln(),
                        od[__cpm_get_pllod()]
                    );
            }
            DBG_PUTSF(3, x, line++, "CPPCR1:0x%08x", cppcr1);
            if (cppcr1 & CPPCR1_PLL1EN) {
                DBG_PUTSF(4, x, line++, "PLL1  :%3d.%02d MHz", TO_MHZ(__cpm_get_pll1out()));
                DBG_PUTSF(5, x, line++, "m:n:o :%d:%d:%d",
                    __cpm_get_pll1m(),
                    __cpm_get_pll1n(),
                    od[__cpm_get_pll1od()]
                );
            }
            DBG_PUTSF(6, x, line++, "C:H:M:P:%d:%d:%d:%d",
                    div[__cpm_get_cdiv()],
                    div[__cpm_get_hdiv()],
                    div[__cpm_get_mdiv()],
                    div[__cpm_get_pdiv()]
                );
            DBG_PUTSF(7, x, line++, "I:P:M : %d:%d:%d",
                    __cpm_get_i2sdiv()+1,
                    __cpm_get_pixdiv()+1,
                    __cpm_get_mscdiv()+1
                );
            DBG_PUTSF(8, x, line++, "CCLK  :%3d.%02d MHz", TO_MHZ(__cpm_get_cclk()));
            DBG_PUTSF(9, x, line++, "HCLK  :%3d.%02d MHz", TO_MHZ(__cpm_get_hclk()));
            DBG_PUTSF(10, x, line++, "MCLK  :%3d.%02d MHz", TO_MHZ(__cpm_get_mclk()));
            DBG_PUTSF(11, x, line++, "PCLK  :%3d.%02d MHz", TO_MHZ(__cpm_get_pclk()));
            DBG_PUTSF(12, x, line++, "PIXCLK:%6d.%02d KHz", TO_KHZ(__cpm_get_pixclk()));
            DBG_PUTSF(13, x, line++, "I2SCLK:%3d.%02d MHz", TO_MHZ(__cpm_get_i2sclk()));
            DBG_PUTSF(14, x, line++, "MSCCLK:%3d.%02d MHz", TO_MHZ(__cpm_get_mscclk()));
            DBG_PUTSF(15, x, line++, "EXTALCLK:%3d.%02d MHz", TO_MHZ(__cpm_get_extalclk()));
            DBG_PUTSF(16, x, line++, "RTCCLK:%3d.%02d KHz", TO_KHZ(__cpm_get_rtcclk()));
            if (cur_line + maxlines > 17)
                cur_line = -1;
            lcd_update();
        }
    }
    return true;
}

static bool display_enabled_clocks(void)
{
    unsigned long lcr = REG_CPM_LCR;
    unsigned long clkgr0 = REG_CPM_CLKGR0;

    lcd_putsf(0, line++, "Low Power Mode : %s",
            ((lcr & LCR_LPM_MASK) == LCR_LPM_IDLE) ?
            "IDLE" : (((lcr & LCR_LPM_MASK) == LCR_LPM_SLEEP) ? "SLEEP" : "HIBERNATE")
          );

    lcd_putsf(0, line++, "Doze Mode      : %s",
            (lcr & LCR_DOZE) ? "ON" : "OFF");
    if (lcr & LCR_DOZE)
        lcd_putsf(0, line++, "     duty      : %d", (int)((lcr & LCR_DUTY_MASK) >> LCR_DUTY_LSB));

    lcd_putsf(0, line++, "IPU   : %s",
            (clkgr0 & CLKGR0_IPU) ? "stopped" : "running");
    lcd_putsf(0, line++, "DMAC  : %s",
            (clkgr0 & CLKGR0_DMAC) ? "stopped" : "running");
    lcd_putsf(0, line++, "UHC   : %s",
            (clkgr0 & CLKGR0_UHC) ? "stopped" : "running");
    lcd_putsf(0, line++, "LCD   : %s",
            (clkgr0 & CLKGR0_LCD) ? "stopped" : "running");
    lcd_putsf(0, line++, "CIM   : %s",
            (clkgr0 & CLKGR0_CIM) ? "stopped" : "running");
    lcd_putsf(0, line++, "SADC  : %s",
            (clkgr0 & CLKGR0_SADC) ? "stopped" : "running");
    lcd_putsf(0, line++, "MSC0  : %s",
            (clkgr0 & CLKGR0_MSC0) ? "stopped" : "running");
    lcd_putsf(0, line++, "MSC1  : %s",
            (clkgr0 & CLKGR0_MSC1) ? "stopped" : "running");
    lcd_putsf(0, line++, "MSC2  : %s",
            (clkgr0 & CLKGR0_MSC2) ? "stopped" : "running");
    lcd_putsf(0, line++, "AIC   : %s",
            (clkgr0 & CLKGR0_AIC) ? "stopped" : "running");
    lcd_putsf(0, line++, "SSI1  : %s",
            (clkgr0 & CLKGR0_SSI1) ? "stopped" : "running");
    lcd_putsf(0, line++, "SSI2  : %s",
            (clkgr0 & CLKGR0_SSI2) ? "stopped" : "running");
    lcd_putsf(0, line++, "I2C0  : %s",
            (clkgr0 & CLKGR0_I2C0) ? "stopped" : "running");
    lcd_putsf(0, line++, "I2C1  : %s",
            (clkgr0 & CLKGR0_I2C1) ? "stopped" : "running");
    lcd_putsf(0, line++, "UART1 : %s",
            (clkgr0 & CLKGR0_UART1) ? "stopped" : "running");
    lcd_putsf(0, line++, "UART0 : %s",
            (clkgr0 & CLKGR0_UART0) ? "stopped" : "running");

    return true;
}

bool dbg_ports(void)
{
#if CONFIG_CPU == JZ4760B
    int line, i, j, cur;
    int x = 0;
    const int last_port = 5;
    bool done = false;

    long data, dir;
    long fun, intr;
    long lvl;


    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(!done)
    {
        i = 0;
        while(dbg_btn_update(&done, &x))
        {
            i %= last_port; /*PORT: A B C D E F */
            while(dbg_btn_update(&done, &x))
            {
                line = 0;
                lcd_puts(x, line++, "[GPIO Vals and Dirs]");
                for (j = i; j < i + 2; j++)
                {
                    cur = j % last_port;
                    dir = REG_GPIO_PXDIR(cur);
                    data = REG_GPIO_PXDAT(cur);
                    fun = REG_GPIO_PXFUN(cur);
                    intr = REG_GPIO_PXIM(cur);
                    lvl  = REG_GPIO_PXPIN(cur);

                    lcd_putsf(x, line++, "[%s%c]: %8x", "GPIO", 'A' + cur, lvl);
                    lcd_putsf(x, line++, "DIR: %8x FUN: %8x", dir, fun);
                    lcd_putsf(x, line++, "DAT: %8x INT: %8x", data, intr);
                    line++;
                }
            }
            i++;
        }

    }
    lcd_setfont(FONT_UI);
#endif

    return false;
}

extern uint32_t irqstackend,irqstackbegin;

bool dbg_hw_info(void)
{
    return hw_info_debug_menu() > 0;
    int btn = 0;
#ifdef HAVE_TOUCHSCREEN
    int touch;
#endif
    struct tm *cur_time;

    lcd_setfont(FONT_UI);
    while(btn ^ BUTTON_POWER)
    {
        lcd_clear_display();
        line = 0;
        lcd_putsf(0, line++, "CPUID %X", read_cp0_15);

        uint32_t *ptr = &irqstackbegin;
        for ( ; ptr < &irqstackend && *ptr == 0xDEADBEEF; ptr++) {}

        lcd_putsf(0, line++, "IRQ stack max: %d", (uint32_t)&irqstackend - (uint32_t)ptr);

        display_clocks();
        display_enabled_clocks();
#ifdef HAVE_TOUCHSCREEN
        btn = button_read_device(&touch);
        lcd_putsf(0, line++, "X: %d Y: %d BTN: 0x%X", touch>>16, touch&0xFFFF, btn);
#else
        btn = button_read_device();
#endif
        cur_time = get_time();
        lcd_putsf(0, line++, "%02d/%02d/%04d %02d:%02d:%02d", cur_time->tm_mday,
               cur_time->tm_mon, cur_time->tm_year, cur_time->tm_hour,
               cur_time->tm_min, cur_time->tm_sec);
        lcd_update();
        sleep(HZ/16);
    }
    return true;
}
#endif

#ifdef BOOTLOADER
#define WITH_SERIAL
#endif

#ifdef WITH_SERIAL
void serial_putc (const char c)
{
        volatile u8 *uart_lsr = (volatile u8 *)(CFG_UART_BASE + OFF_LSR);
        volatile u8 *uart_tdr = (volatile u8 *)(CFG_UART_BASE + OFF_TDR);

        if (c == '\n') serial_putc ('\r');

        /* Wait for fifo to shift out some bytes */
        while ( !((*uart_lsr & (UARTLSR_TDRQ | UARTLSR_TEMT)) == 0x60) );

        *uart_tdr = (u8)c;
}

void serial_puts (const char *s)
{
        while (*s) {
                serial_putc (*s++);
        }
}

void serial_putsf(const char *format, ...)
{
    static char printfbuf[256];
    int len;
    unsigned char *ptr;
    va_list ap;
    va_start(ap, format);

    ptr = printfbuf;
    len = vsnprintf(ptr, sizeof(printfbuf), format, ap);
    va_end(ap);
    (void)len;

    serial_puts(ptr);
    serial_putc('\n');
}

void serial_put_hex(unsigned int  d)
{
        char c[12];
        int i;
        for(i = 0; i < 8;i++)
        {
                c[i] = (d >> ((7 - i) * 4)) & 0xf;
                if(c[i] < 10)
                        c[i] += 0x30;
                else
                        c[i] += (0x41 - 10);
        }
        c[8] = '\n';
        c[9] = 0;
        serial_puts(c);

}
void serial_put_dec(unsigned int  d)
{
        char c[16];
        int i;
        int j = 0;
        int x = d;

        while (x /= 10)
                j++;

        for (i = j; i >= 0; i--) {
                c[i] = d % 10;
                c[i] += 0x30;
                d /= 10;
        }
        c[j + 1] = '\n';
        c[j + 2] = 0;
        serial_puts(c);
}

void serial_dump_data(unsigned char* data, int len)
{
        int i;
        for(i=0; i<len; i++)
        {
                unsigned char a = ((*data)>>4) & 0xf;
                if(a < 10)
                        a += 0x30;
                else
                        a += (0x41 - 10);
                serial_putc( a );

                a = (*data) & 0xf;
                if(a < 10)
                        a += 0x30;
                else
                        a += (0x41 - 10);
                serial_putc( a );

                serial_putc( ' ' );

                data++;
        }

        serial_putc( '\n' );
}
#endif

#ifndef BOOTLOADER
static const struct {
    unsigned char *desc; /* string or ID */
    bool (*function) (void); /* return true if USB was connected */
} hwinfo_items[] = {
        { "", NULL }, /*CPUID*/
        { "", NULL }, /*IRQSTACKMAX*/
        { "Clocks", display_clocks},
        { "Enabled Clocks", display_enabled_clocks},
};

static int menu_action_callback(int btn, struct gui_synclist *lists)
{
    int selection = gui_synclist_get_sel_pos(lists);
    if (btn == ACTION_STD_OK)
    {
        FOR_NB_SCREENS(i)
           viewportmanager_theme_enable(i, false, NULL);
        if (hwinfo_items[selection].function)
        {
            is_done = false;
            hwinfo_items[selection].function();
        }       
        btn = ACTION_REDRAW;
        FOR_NB_SCREENS(i)
            viewportmanager_theme_undo(i, false);
    }
    else if (btn == ACTION_STD_CONTEXT)
    {
    }
    return btn;
}

static const char* menu_get_name(int item, void * data,
                                    char *buffer, size_t buffer_len)
{
    (void)data;
    uint32_t *stackptr;
    switch(item)
    {
        case 0: /*cpuID*/
            snprintf(buffer, buffer_len, "CPUID %X:%d", read_cp0_15(), (REG_CPM_CLKGR0 & BIT31) >> 31);
            return buffer;
        case 1:
            stackptr = &irqstackbegin;
            for ( ; stackptr < &irqstackend && *stackptr == 0xDEADBEEF; stackptr++) {}
            snprintf(buffer, buffer_len, "IRQ stack max: %lu", (uint32_t)&irqstackend - (uint32_t)stackptr);
            return buffer;
        default:
        return hwinfo_items[item].desc;
    }
    return "???";
}

static int hw_info_debug_menu(void)
{
    int h;
    lcd_setfont(FONT_SYSFIXED);
    lcd_getstringsize((unsigned char *)"A", NULL, &h);
    maxlines = LCD_HEIGHT / h - 1;
    is_done = false;
    struct simplelist_info info;

    simplelist_info_init(&info, "Hw Info", ARRAYLEN(hwinfo_items), NULL);
    info.action_callback = menu_action_callback;
    info.get_name        = menu_get_name;
    return (simplelist_show_list(&info)) ? 1 : 0;
}
#endif
