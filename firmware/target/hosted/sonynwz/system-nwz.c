/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 *
 * Copyright (C) 2016 Amaury Pouly
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

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <ucontext.h>
#include <string.h>
#include "system.h"
#include "lcd.h"
#include "font.h"
#include "logf.h"
#include "system.h"
#include "backlight-target.h"
#include "button.h"
#include <backtrace.h>

void power_off(void)
{
    exit(0);
}

/* to make thread-internal.h happy */
uintptr_t *stackbegin;
uintptr_t *stackend;

static void nwz_sig_handler(int sig, siginfo_t *siginfo, void *context)
{
    /* safe guard variable - we call backtrace() only on first
     * UIE call. This prevent endless loop if backtrace() touches
     * memory regions which cause abort
     */
    static bool triggered = false;

    lcd_set_backdrop(NULL);
    lcd_set_drawmode(DRMODE_SOLID);
    lcd_set_foreground(LCD_BLACK);
    lcd_set_background(LCD_WHITE);
    unsigned line = 0;

    lcd_setfont(FONT_SYSFIXED);
    lcd_set_viewport(NULL);
    lcd_clear_display();

    /* get context info */
    ucontext_t *uc = (ucontext_t *)context;
    unsigned long pc = uc->uc_mcontext.arm_pc;
    unsigned long sp = uc->uc_mcontext.arm_sp;

    lcd_putsf(0, line++, "%s at %08x", strsignal(sig), pc);

    if(sig == SIGILL || sig == SIGFPE || sig == SIGSEGV || sig == SIGBUS || sig == SIGTRAP)
        lcd_putsf(0, line++, "address 0x%08x", siginfo->si_addr);

    if(!triggered)
    {
        triggered = true;
        //backtrace(pc, sp, &line);
    }

#ifdef ROCKBOX_HAS_LOGF
    lcd_putsf(0, line++, "logf:");
    logf_panic_dump(&line);
#endif

    lcd_update();

    system_exception_wait(); /* If this returns, try to reboot */
    system_reboot();
    while (1);       /* halt */
}

void system_init(void)
{
    int *s;
    /* fake stack, to make thread-internal.h happy */
    stackbegin = stackend = (uintptr_t*)&s;
    /* catch some signals for easier debugging */
    struct sigaction sa;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = &nwz_sig_handler;
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}


void system_reboot(void)
{
    power_off();
}

void system_exception_wait(void)
{
    backlight_hw_on();
    backlight_hw_brightness(DEFAULT_BRIGHTNESS_SETTING);
    /* wait until button press and release */
    while(button_read_device() != 0) {}
    while(button_read_device() == 0) {}
    while(button_read_device() != 0) {}
    while(button_read_device() == 0) {}
}

int hostfs_init(void)
{
    return 0;
}

int hostfs_flush(void)
{
    sync();
    return 0;
}
