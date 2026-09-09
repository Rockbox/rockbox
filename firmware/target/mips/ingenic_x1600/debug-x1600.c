/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 *
 * Based on firmware/target/mips/ingenic_x1000/debug-x1000.c,
 * Copyright (C) 2021 Aidan MacDonald
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

/* Check clock and power gates before reading peripheral registers;
 * accessing a gated block can hang the bus. */

#ifndef BOOTLOADER

#define LOGF_ENABLE

#include "system.h"
#include "viewport.h"
#include "kernel.h"
#include "button.h"
#include "lcd.h"
#include "font.h"
#include "logf.h"
#include "action.h"
#include "misc.h"
#include "list.h"
#include "adc.h"

#include "clk-x1600.h"
#include "gpio-x1600.h"
#include "x1600/cpm.h"
#include "x1600/aic.h"
#include "adc-target.h"
#include "rbversion.h"
#ifdef HAVE_CS43131
#include "cs43131.h"
#endif

extern volatile unsigned aic_tx_underruns;

/* Cache mode */

/* Which image was loaded, and what the silicon is doing with KSEG0 */

static bool dbg_cache_mode(void)
{
    do {
        lcd_clear_display();
        lcd_putsf(0, 0, "version   %s", RBVERSION);

        /* Read the KSEG0 cache mode from CP0 Config.K0; 2 means uncached. */
        {
            uint32_t cfg;
            __asm__ __volatile__("mfc0 %0, $16, 0" : "=r"(cfg));
            unsigned k0 = cfg & 7;
            lcd_putsf(0, 1, "KSEG0     %s (Config.K0=%u)",
                      k0 == 2 ? "UNCACHED" : "cached", k0);
        }

        lcd_putsf(0, 3, "version = which image was LOADED");
        lcd_putsf(0, 4, "K0      = what the silicon is doing");
        lcd_update();
    } while(!action_userabort(HZ));

    return false;
}

/* Stacks */

/* Scan the 0xDEADBEEF stack canaries, as in debug-jz4760.c. The IRQ stack
 * adjoins the main stack and has no guard page. */
static bool dbg_stacks(void)
{
    extern uint32_t stackbegin[], stackend[];
    extern uint32_t _irqstackbegin[], _irqstackend[];

    do {
        lcd_clear_display();
        int line = 0;
        lcd_putsf(0, line++, "%-6s %6s %6s %4s", "stack", "used", "size", "pct");

        struct { const char* name; uint32_t* lo; uint32_t* hi; } s[] = {
            { "main", stackbegin,     stackend     },
            { "IRQ",  _irqstackbegin, _irqstackend },
        };

        for(unsigned i = 0; i < ARRAYLEN(s); ++i) {
            uint32_t* p = s[i].lo;
            while(p < s[i].hi && *p == 0xDEADBEEF)
                ++p;

            unsigned size = (unsigned)((char*)s[i].hi - (char*)s[i].lo);
            unsigned used = (unsigned)((char*)s[i].hi - (char*)p);
            lcd_putsf(0, line++, "%-6s %6u %6u %3u%%",
                      s[i].name, used, size, size ? (used * 100 / size) : 0);
        }

        line++;
        lcd_putsf(0, line++, "no guard page on either");
        lcd_putsf(0, line++, "IRQ overflows INTO main");

        lcd_update();
    } while(!action_userabort(HZ));

    return false;
}

static bool dbg_clocks(void)
{
    do {
        lcd_clear_display();
        int line = 0;
        for(int i = 0; i < X1600_CLK_COUNT; ++i) {
            uint32_t hz = clk_get(i);
            uint32_t khz = hz / 1000;
            uint32_t mhz = khz / 1000;
            lcd_putsf(0, line++, "%9s %4u,%03u,%03u Hz", clk_get_name(i),
                      mhz, (khz - mhz*1000), (hz - khz*1000));
        }

        lcd_update();
    } while(!action_userabort(HZ));

    return false;
}

/* CPM */

static bool dbg_cpm(void)
{
    do {
        lcd_clear_display();
        int line = 0;
        lcd_putsf(0, line++, "CPCCR  %08lx", (unsigned long)REG_CPM_CPCCR);
        lcd_putsf(0, line++, "CPCSR  %08lx", (unsigned long)REG_CPM_CPCSR);
        lcd_putsf(0, line++, "CPAPCR %08lx", (unsigned long)REG_CPM_CPAPCR);
        lcd_putsf(0, line++, "CPMPCR %08lx", (unsigned long)REG_CPM_CPMPCR);
        lcd_putsf(0, line++, "CPEPCR %08lx", (unsigned long)REG_CPM_CPEPCR);
        lcd_putsf(0, line++, "DDRCDR %08lx", (unsigned long)REG_CPM_DDRCDR);
        lcd_putsf(0, line++, "LPCDR  %08lx", (unsigned long)REG_CPM_LPCDR);
        lcd_putsf(0, line++, "MSC0CDR%08lx", (unsigned long)REG_CPM_MSC0CDR);
        lcd_putsf(0, line++, "MSC1CDR%08lx", (unsigned long)REG_CPM_MSC1CDR);
        lcd_putsf(0, line++, "SFCCDR %08lx", (unsigned long)REG_CPM_SFCCDR);
        lcd_putsf(0, line++, "SSICDR %08lx", (unsigned long)REG_CPM_SSICDR);
        lcd_putsf(0, line++, "PWMCDR %08lx", (unsigned long)REG_CPM_PWMCDR);
        lcd_putsf(0, line++, "I2S1CDR%08lx", (unsigned long)REG_CPM_I2S1CDR);
        line++;
        /* 1 = clock stopped.  Boot ROM leaves 0x47FBFFF4 / 0x37FFFFFF. */
        lcd_putsf(0, line++, "CLKGR  %08lx", (unsigned long)REG_CPM_CLKGR);
        lcd_putsf(0, line++, "CLKGR1 %08lx", (unsigned long)REG_CPM_CLKGR1);
        /* 1 = that block's RAM is powered down. */
        lcd_putsf(0, line++, "MEMPD0 %08lx", (unsigned long)REG_CPM_MEMPD0);
        lcd_putsf(0, line++, "MEMPD1 %08lx", (unsigned long)REG_CPM_MEMPD1);
        lcd_putsf(0, line++, "SRBC   %08lx", (unsigned long)REG_CPM_SRBC);
        lcd_putsf(0, line++, "OPCR   %08lx", (unsigned long)REG_CPM_OPCR);
        lcd_update();
    } while(!action_userabort(HZ));

    return false;
}

/* GPIO */

#define GPIO_NUM_PORTS 4    /* PA..PD; PM tables 21-2 .. 21-5 */

static const char gpio_portname[GPIO_NUM_PORTS] = { 'A', 'B', 'C', 'D' };

static void dbg_gpios_show_state(void)
{
    int line = 0;
    for(int i = 0; i < GPIO_NUM_PORTS; ++i)
        lcd_putsf(0, line++, "GPIO %c: %08lx", gpio_portname[i],
                  (unsigned long)REG_GPIO_PIN(i));

    line++;
    lcd_putsf(0, line++, "SFC pins are PC17-22 (mask %08lx)",
              (unsigned long)(0x3fu << 17));
    lcd_putsf(0, line++, "PC = %08lx", (unsigned long)REG_GPIO_PIN(GPIO_C));
}

static void dbg_gpios_show_config(int port)
{
    int line = 0;
    lcd_putsf(0, line++, "GPIO %c", gpio_portname[port]);
    /* X1600 GPIO offsets: INT at 0x10, EDG at 0x70, pull enable at 0x80. */
    lcd_putsf(0, line++, " int  %08lx", (unsigned long)REG_GPIO_INT(port));
    lcd_putsf(0, line++, " msk  %08lx", (unsigned long)REG_GPIO_MSK(port));
    lcd_putsf(0, line++, " pat1 %08lx", (unsigned long)REG_GPIO_PAT1(port));
    lcd_putsf(0, line++, " pat0 %08lx", (unsigned long)REG_GPIO_PAT0(port));
    lcd_putsf(0, line++, " flag %08lx", (unsigned long)REG_GPIO_FLAG(port));
    lcd_putsf(0, line++, " edg  %08lx", (unsigned long)REG_GPIO_EDG(port));
    lcd_putsf(0, line++, " pull %08lx", (unsigned long)REG_GPIO_PULL(port));
    lcd_putsf(0, line++, " pin  %08lx", (unsigned long)REG_GPIO_PIN(port));
    line++;
    lcd_putsf(0, line++, "NEXT changes port");
}

static bool dbg_gpios(void)
{
    enum { STATE, NUM_STATIC_SCREENS };
    int screen = STATE;

    while(1) {
        lcd_clear_display();
        if(screen == STATE)
            dbg_gpios_show_state();
        else
            dbg_gpios_show_config(screen - NUM_STATIC_SCREENS);

        lcd_update();

        int action = get_action(CONTEXT_STD, screen == STATE ? 1 : HZ);
        switch(action) {
        case ACTION_STD_CANCEL:
            return false;
        case ACTION_STD_PREV:
        case ACTION_STD_PREVREPEAT:
            screen -= 1;
            if(screen < 0)
                screen = NUM_STATIC_SCREENS + GPIO_NUM_PORTS - 1;
            break;
        case ACTION_STD_NEXT:
        case ACTION_STD_NEXTREPEAT:
            screen += 1;
            if(screen >= NUM_STATIC_SCREENS + GPIO_NUM_PORTS)
                screen = 0;
            break;
        default:
            default_event_handler(action);
            break;
        }
    }

    return false;
}

/* SADC */

static bool dbg_adc(void)
{
    /* AUX3 is not sampled by adc_init(). */
    static const char* const adc_names[] = {
        "AUX0 buttons",
        "AUX1 hp det",
        "AUX2 remote",
    };

    do {
        lcd_clear_display();
        int line = 0;

        if(jz_readf(CPM_CLKGR, SADC)) {
            /* Reading SADC registers with the block gated would hang. */
            lcd_putsf(0, line++, "SADC is clock-gated (CLKGR bit 13)");
            lcd_putsf(0, line++, "adc_init() has not run");
        } else {
            for(unsigned i = 0; i < ARRAYLEN(adc_names); ++i) {
                unsigned short raw = adc_read(i);
                lcd_putsf(0, line++, "%-13s %4u  %4d mV",
                          adc_names[i], raw, adc_read_mv(i));
            }

            line++;
            lcd_putsf(0, line++, "key idle %d mV", ADC_KEY_IDLE_MV);
            lcd_putsf(0, line++, "key play %d  vol- %d  vol+ %d",
                      ADC_KEY_PLAY_MV, ADC_KEY_VOLDOWN_MV, ADC_KEY_VOLUP_MV);
            lcd_putsf(0, line++, "hp in-range %d..%d mV",
                      ADC_HP_MIN_MV, ADC_HP_MAX_MV);
        }

        lcd_update();
    } while(!action_userabort(HZ/4));

    return false;
}

/* Audio */

static bool dbg_audio(void)
{
    do {
        lcd_clear_display();
        int line = 0;
        /* Playback underrun count; recording is not implemented. */
        lcd_putsf(0, line++, "TX underruns            %u", aic_tx_underruns);
        lcd_putsf(0, line++, "AUDIO gate (CLKGR.11)   %s",
                  jz_readf(CPM_CLKGR, AUDIO) ? "GATED" : "open");
        lcd_putsf(0, line++, "I2S0 TCLK  (CLKGR1.9)   %s",
                  jz_readf(CPM_CLKGR1, I2S0_TCLK) ? "GATED" : "open");
        lcd_putsf(0, line++, "I2S0 RCLK  (CLKGR1.8)   %s",
                  jz_readf(CPM_CLKGR1, I2S0_RCLK) ? "GATED" : "open");
        lcd_putsf(0, line++, "I2S1CDR   %08lx",
                  (unsigned long)REG_CPM_I2S1CDR);
        lcd_putsf(0, line++, "MCLK      %lu Hz",
                  (unsigned long)clk_get(X1600_CLK_I2S_MCLK));
        lcd_putsf(0, line++, "BCLK      %lu Hz",
                  (unsigned long)clk_get(X1600_CLK_I2S_BCLK));

        /* Show DAC control-bus errors and AIC FIFO state. Read AIC
         * registers only while its clock is enabled. */
        line++;
#ifdef HAVE_CS43131
        {
            unsigned w, n; int e;
            cs43131_bus_status(&w, &n, &e);
            lcd_putsf(0, line++, "DAC i2c   w%u nak%u e%d", w, n, e);
        }
#endif
        if(jz_readf(CPM_CLKGR, AUDIO)) {
            lcd_putsf(0, line++, "AIC_SR    (gated)");
        } else {
            unsigned long sr = REG_AIC_SR;
            lcd_putsf(0, line++, "AIC_SR    %08lx TFL %lu", sr,
                      (sr & BM_AIC_SR_TFL) >> BP_AIC_SR_TFL);
        }
        lcd_update();
    } while(!action_userabort(HZ));

    return false;
}

#ifdef INGENIC_CPUIDLE_STATS
static bool dbg_cpuidle(void)
{
    do {
        unsigned freq = clk_get(X1600_CLK_CPU);
        lcd_clear_display();
        lcd_putsf(0, 0, "CPU idle time: %d.%01d%%",
                  __cpu_idle_cur/10, __cpu_idle_cur%10);
        lcd_putsf(0, 1, "CPU frequency: %u.%03u MHz",
                  freq/1000000, (freq%1000000)/1000);
        lcd_update();
    } while(!action_userabort(HZ));

    return false;
}
#endif

/* Menu */

#ifdef HAVE_AXP2101
extern bool axp2101_debug_menu(void);
#endif
#ifdef HAVE_CW2015
extern bool cw2015_debug_menu(void);
#endif

static const struct {
    const char* name;
    bool (*function)(void);
} hw_info_menu[] = {
    {"Cache mode", &dbg_cache_mode},
    {"Stacks", &dbg_stacks},
    {"Clocks", &dbg_clocks},
    {"CPM registers", &dbg_cpm},
    {"GPIOs", &dbg_gpios},
#ifdef INGENIC_CPUIDLE_STATS
    {"CPU idle", &dbg_cpuidle},
#endif
    {"SADC", &dbg_adc},
    {"Audio", &dbg_audio},
#ifdef HAVE_AXP2101
    {"Power stats (AXP2101)", &axp2101_debug_menu},
#endif
#ifdef HAVE_CW2015
    {"CW2015 debug", &cw2015_debug_menu},
#endif
};

static int hw_info_menu_action_cb(int btn, struct gui_synclist* lists)
{
    if(btn == ACTION_STD_OK) {
        int sel = gui_synclist_get_sel_pos(lists);
        FOR_NB_SCREENS(i)
            viewportmanager_theme_enable(i, false, NULL);

        lcd_setfont(FONT_SYSFIXED);
        lcd_set_foreground(LCD_WHITE);
        lcd_set_background(LCD_BLACK);

        if(hw_info_menu[sel].function())
            btn = SYS_USB_CONNECTED;
        else
            btn = ACTION_REDRAW;

        lcd_setfont(FONT_UI);

        FOR_NB_SCREENS(i)
            viewportmanager_theme_undo(i, false);
    }

    return btn;
}

static const char* hw_info_menu_get_name(int item, void* data,
                                         char* buffer, size_t buffer_len)
{
    (void)buffer;
    (void)buffer_len;
    (void)data;
    return hw_info_menu[item].name;
}

bool dbg_hw_info(void)
{
    struct simplelist_info info;
    simplelist_info_init(&info, MODEL_NAME " debug menu",
                         ARRAYLEN(hw_info_menu), NULL);
    info.action_callback = hw_info_menu_action_cb;
    info.get_name = hw_info_menu_get_name;
    return simplelist_show_list(&info);
}

bool dbg_ports(void)
{
    bool usb = false;
    /* Compare the hardware reading with the generic button state. */
    lcd_setfont(FONT_SYSFIXED);

    while(1) {
        int data = 0;
        int raw = button_read_device(&data);
        int st  = button_status();
        static int last_raw = -1;
        if(raw != last_raw) {       /* only on change, or the log fills with noise */
            logf("x1600 button: raw=%08X status=%08X", raw, st);
            last_raw = raw;
        }

        lcd_clear_display();
        lcd_putsf(0, 0, "BUTTON DEBUG");
        lcd_putsf(0, 2, "raw    %08X", raw);
        lcd_putsf(0, 3, "status %08X", st);
        lcd_putsf(0, 5, "POWER  %s", (raw & BUTTON_POWER) ? "DOWN" : "up");
        lcd_putsf(0, 6, "PLAY   %s", (raw & BUTTON_PLAY)  ? "DOWN" : "up");
        lcd_putsf(0, 7, "NEXT   %s", (raw & BUTTON_NEXT)  ? "DOWN" : "up");
        lcd_putsf(0, 8, "VOL+   %s", (raw & BUTTON_VOL_UP)   ? "DOWN" : "up");
        lcd_putsf(0, 9, "VOL-   %s", (raw & BUTTON_VOL_DOWN) ? "DOWN" : "up");
        lcd_putsf(0, 11,"hold PLAY to exit");
        lcd_update();

        /* PLAY, not POWER: POWER is the button under investigation and must
         * stay observable rather than being consumed to leave the screen. */
        int b = button_get_w_tmo(HZ/10);
        if(b == (BUTTON_PLAY | BUTTON_REPEAT))
            break;
        if(default_event_handler(b) == SYS_USB_CONNECTED) {
            usb = true;
            break;
        }
    }

    lcd_setfont(FONT_UI);
    return usb;
}
#endif /* !BOOTLOADER */
