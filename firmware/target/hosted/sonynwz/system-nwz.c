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
#include <stdio.h>

#include "system.h"
#include "lcd.h"
#include "font.h"
#include "system.h"
#include "backlight-target.h"
#include "button.h"
#include "adc.h"
#include "power.h"
#include "mv.h"
#include "power-nwz.h"
#include <backtrace.h>

#include "logf.h"

static const char **kern_mod_list;
bool os_file_exists(const char *ospath);

void power_off(void)
{
    exit(0);
}

static void compute_kern_mod_list(void)
{
    /* create empty list */
    kern_mod_list = malloc(sizeof(const char **));
    kern_mod_list[0] = NULL;
    /* read from proc file system */
    FILE *f = fopen("/proc/modules", "re");
    if(f == NULL)
    {
        printf("Cannot open /proc/modules");
        return;
    }
    for(int i = 0;; i++)
    {
        /* the last entry of the list points to NULL so getline() will allocate
         * some memory */
        size_t n;
        if(getline((char **)&kern_mod_list[i], &n, f) < 0)
        {
            /* make sure last entry is NULL and stop */
            kern_mod_list[i] = NULL;
            break;
        }
        /* grow array */
        kern_mod_list = realloc(kern_mod_list, (i + 2) * sizeof(const char **));
        /* and fill last entry with NULL */
        kern_mod_list[i + 1] = NULL;
        /* parse line to only keep module name */
        char *p = strchr(kern_mod_list[i], ' ');
        if(p != NULL)
            *p = 0; /* stop at first blank */
    }
    fclose(f);
}

static void print_kern_mod_list(void)
{
    printf("Kernel modules:\n");
    const char **p = kern_mod_list;
    while(*p)
        printf("  %s\n", *p++);
}

/* to make thread-internal.h happy */
uintptr_t *stackbegin;
uintptr_t *stackend;

static void dump_proc_map(void)
{
    const char *file = "/proc/self/maps";
    printf("Dumping %s...\n", file);
    FILE *f = fopen(file, "re");
    if(f == NULL)
    {
        perror("Cannot open file");
        return;
    }
    while(true)
    {
        char *line = NULL;
        size_t n;
        if(getline(&line, &n, f) < 0)
            break;
        printf("> %s", line);
        free(line);
    }
    fclose(f);
}

static void nwz_sig_handler(int sig, siginfo_t *siginfo, void *context)
{
    /* safe guard variable - we call backtrace() only on first
     * UIE call. This prevent endless loop if backtrace() touches
     * memory regions which cause abort
     */
    static bool triggered = false;

    /* dump process maps to log file to ease debugging
     * will also print crash info to the log */
    dump_proc_map();

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

    printf("%s at %08x\n", strsignal(sig), (unsigned int)pc);
    lcd_putsf(0, line++, "%s at %08x", strsignal(sig), pc);

    if(sig == SIGILL || sig == SIGFPE || sig == SIGSEGV || sig == SIGBUS || sig == SIGTRAP)
    {
        printf("address 0x%08x\n", (unsigned int)siginfo->si_addr);
        lcd_putsf(0, line++, "address 0x%08x", siginfo->si_addr);
    }

    if(!triggered)
    {
        triggered = true;
        rb_backtrace(pc, sp, &line);
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
    compute_kern_mod_list();
    print_kern_mod_list();
    /* some init not done on hosted targets */
    adc_init();
    power_init();
}


void system_reboot(void)
{
    power_off();
}

#ifdef HAVE_BUTTON_DATA
#define IF_DATA(data) data
#else
#define IF_DATA(data)
#endif
void system_exception_wait(void)
{
    backlight_hw_on();
    backlight_hw_brightness(DEFAULT_BRIGHTNESS_SETTING);
    /* wait until button press and release */
    IF_DATA(int data);
    while(button_read_device(IF_DATA(&data)) != 0) {}
    while(button_read_device(IF_DATA(&data)) == 0) {}
    while(button_read_device(IF_DATA(&data)) != 0) {}
    while(button_read_device(IF_DATA(&data)) == 0) {}
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

const char **nwz_get_kernel_module_list(void)
{
    return kern_mod_list;
}

bool nwz_is_kernel_module_loaded(const char *name)
{
    const char **p = kern_mod_list;
    while(*p)
        if(strcmp(*p++, name) == 0)
            return true;
    return false;
}

#ifdef CONFIG_STORAGE_MULTI
int hostfs_driver_type(int drive)
{
    return drive > 0 ? STORAGE_SD_NUM : STORAGE_HOSTFS_NUM;
}
#endif /* CONFIG_STORAGE_MULTI */

#ifdef HAVE_HOTSWAP
bool hostfs_removable(IF_MD_NONVOID(int volume))
{
#ifdef HAVE_MULTIDRIVE
    if (volume > 0)
        return true;
    else
#endif
        return false; /* internal: always present */
}

bool hostfs_present(int volume)
{
#ifdef HAVE_MULTIDRIVE
    if (volume > 0)
#if defined(MULTIDRIVE_DEV)
        return os_file_exists(MULTIDRIVE_DEV);
#else
        return true; // FIXME?
#endif
    else
#endif
        return true; /* internal: always present */
}
#endif /* HAVE_HOTSWAP */

#ifdef HAVE_MULTIDRIVE
int volume_drive(int drive)
{
    return drive;
}
#endif /* HAVE_MULTIDRIVE */

#ifdef HAVE_HOTSWAP
bool volume_removable(IF_MV_NONVOID(int volume))
{
    /* don't support more than one partition yet, so volume == drive */
    return hostfs_removable(volume);
}

bool volume_present(int volume)
{
    /* don't support more than one partition yet, so volume == drive */
    return hostfs_present(volume);
}
#endif /* HAVE_HOTSWAP */
