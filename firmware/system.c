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
#include <stdio.h>
#include "config.h"
#include <stdbool.h>
#include "lcd.h"
#include "font.h"
#include "system.h"
#include "kernel.h"
#include "thread.h"
#include "timer.h"
#include "inttypes.h"
#include "string.h"

#ifndef SIMULATOR
long cpu_frequency NOCACHEBSS_ATTR = CPU_FREQ;
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
static int boost_counter NOCACHEBSS_ATTR = 0;
static bool cpu_idle NOCACHEBSS_ATTR = false;
#if NUM_CORES > 1
struct spinlock boostctrl_spin NOCACHEBSS_ATTR;
void cpu_boost_init(void)
{
    spinlock_init(&boostctrl_spin);
}
#endif

int get_cpu_boost_counter(void)
{
    return boost_counter;
}
#ifdef CPU_BOOST_LOGGING
#define MAX_BOOST_LOG 64
static char cpu_boost_calls[MAX_BOOST_LOG][MAX_PATH];
static int cpu_boost_first = 0;
static int cpu_boost_calls_count = 0;
static int cpu_boost_track_message = 0;
int cpu_boost_log_getcount(void)
{
    return cpu_boost_calls_count;
}
char * cpu_boost_log_getlog_first(void)
{
    char *first;
#if NUM_CORES > 1
    spinlock_lock(&boostctrl_spin);
#endif

    first = NULL;

    if (cpu_boost_calls_count)
    {
        cpu_boost_track_message = 1;
        first = cpu_boost_calls[cpu_boost_first];
    }

#if NUM_CORES > 1
    spinlock_unlock(&boostctrl_spin);
#endif
    
    return first;
}

char * cpu_boost_log_getlog_next(void)
{
    int message;
    char *next;

#if NUM_CORES > 1
    spinlock_lock(&boostctrl_spin);
#endif

    message = (cpu_boost_track_message+cpu_boost_first)%MAX_BOOST_LOG;
    next = NULL;

    if (cpu_boost_track_message < cpu_boost_calls_count)
    {
        cpu_boost_track_message++;
        next = cpu_boost_calls[message];
    }

#if NUM_CORES > 1
    spinlock_unlock(&boostctrl_spin);
#endif
    
    return next;
}

void cpu_boost_(bool on_off, char* location, int line)
{
#if NUM_CORES > 1
    spinlock_lock(&boostctrl_spin);
#endif

    if (cpu_boost_calls_count == MAX_BOOST_LOG)
    {
        cpu_boost_first = (cpu_boost_first+1)%MAX_BOOST_LOG;
        cpu_boost_calls_count--;
        if (cpu_boost_calls_count < 0)
            cpu_boost_calls_count = 0;
    }
    if (cpu_boost_calls_count < MAX_BOOST_LOG)
    {
        int message = (cpu_boost_first+cpu_boost_calls_count)%MAX_BOOST_LOG;
        snprintf(cpu_boost_calls[message], MAX_PATH,
                    "%c %s:%d",on_off==true?'B':'U',location,line);
        cpu_boost_calls_count++;
    }
#else
void cpu_boost(bool on_off)
{
#if NUM_CORES > 1
    spinlock_lock(&boostctrl_spin);
#endif

#endif /* CPU_BOOST_LOGGING */
    if(on_off)
    {
        /* Boost the frequency if not already boosted */
        if(++boost_counter == 1)
            set_cpu_frequency(CPUFREQ_MAX);
    }
    else
    {
        /* Lower the frequency if the counter reaches 0 */
        if(--boost_counter <= 0)
        {
            if(cpu_idle)
                set_cpu_frequency(CPUFREQ_DEFAULT);
            else
                set_cpu_frequency(CPUFREQ_NORMAL);

            /* Safety measure */
            if (boost_counter < 0)
            {
                boost_counter = 0;
            }
        }
    }

#if NUM_CORES > 1
    spinlock_unlock(&boostctrl_spin);
#endif
}

void cpu_idle_mode(bool on_off)
{
#if NUM_CORES > 1
    spinlock_lock(&boostctrl_spin);
#endif

    cpu_idle = on_off;

    /* We need to adjust the frequency immediately if the CPU
       isn't boosted */
    if(boost_counter == 0)
    {
        if(cpu_idle)
            set_cpu_frequency(CPUFREQ_DEFAULT);
        else
            set_cpu_frequency(CPUFREQ_NORMAL);
    }

#if NUM_CORES > 1
    spinlock_unlock(&boostctrl_spin);
#endif
}
#endif /* HAVE_ADJUSTABLE_CPU_FREQ */


#ifdef HAVE_FLASHED_ROCKBOX
static bool detect_flash_header(uint8_t *addr)
{
#ifndef BOOTLOADER
    int oldmode = system_memory_guard(MEMGUARD_NONE);
#endif
    struct flash_header hdr;
    memcpy(&hdr, addr, sizeof(struct flash_header));
#ifndef BOOTLOADER
    system_memory_guard(oldmode);
#endif
    return hdr.magic == FLASH_MAGIC;
}
#endif

bool detect_flashed_romimage(void)
{
#ifdef HAVE_FLASHED_ROCKBOX
    return detect_flash_header((uint8_t *)FLASH_ROMIMAGE_ENTRY);
#else
    return false;
#endif /* HAVE_FLASHED_ROCKBOX */
}

bool detect_flashed_ramimage(void)
{
#ifdef HAVE_FLASHED_ROCKBOX
    return detect_flash_header((uint8_t *)FLASH_RAMIMAGE_ENTRY);
#else
    return false;
#endif /* HAVE_FLASHED_ROCKBOX */
}

bool detect_original_firmware(void)
{
    return !(detect_flashed_ramimage() || detect_flashed_romimage());
}

#if defined(CPU_ARM)

static const char* const uiename[] = {
    "Undefined instruction",
    "Prefetch abort",
    "Data abort",
    "Divide by zero"
};

/* Unexpected Interrupt or Exception handler. Currently only deals with
   exceptions, but will deal with interrupts later.
 */
void UIE(unsigned int pc, unsigned int num) __attribute__((noreturn));
void UIE(unsigned int pc, unsigned int num)
{
    char str[32];

    lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
    lcd_setfont(FONT_SYSFIXED);
#endif
    lcd_puts(0, 0, uiename[num]);
    snprintf(str, sizeof(str), "at %08x" IF_COP(" (%d)"), pc
             IF_COP(, CURRENT_CORE));
    lcd_puts(0, 1, str);
    lcd_update();

    while (1)
    {
        /* TODO: perhaps add button handling in here when we get a polling
           driver some day.
         */
        core_idle();
    }
}

#ifndef STUB
/* Needs to be here or gcc won't find it */
void __div0(void) __attribute__((naked));
void __div0(void)
{
    asm volatile (
        "ldr    r0, [sp] \r\n"
        "mov    r1, #3   \r\n"
        "b      UIE      \r\n"
    );
}
#endif

#endif /* CPU_ARM */

