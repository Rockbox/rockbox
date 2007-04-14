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
struct mutex boostctrl_mtx NOCACHEBSS_ATTR;
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
    if (cpu_boost_calls_count)
    {
        cpu_boost_track_message = 1;
        return cpu_boost_calls[cpu_boost_first];
    }
    else return NULL;
}
char * cpu_boost_log_getlog_next(void)
{
    int message = (cpu_boost_track_message+cpu_boost_first)%MAX_BOOST_LOG;
    if (cpu_boost_track_message < cpu_boost_calls_count)
    {
        cpu_boost_track_message++;
        return cpu_boost_calls[message];
    }
    else return NULL;
}
void cpu_boost_(bool on_off, char* location, int line)
{
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
#endif
    if(on_off)
    {
        /* Boost the frequency if not already boosted */
        if(boost_counter++ == 0)
            set_cpu_frequency(CPUFREQ_MAX);
    }
    else
    {
        /* Lower the frequency if the counter reaches 0 */
        if(--boost_counter == 0)
        {
            if(cpu_idle)
                set_cpu_frequency(CPUFREQ_DEFAULT);
            else
                set_cpu_frequency(CPUFREQ_NORMAL);
        }

        /* Safety measure */
        if(boost_counter < 0)
            boost_counter = 0;
    }
}

void cpu_idle_mode(bool on_off)
{
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
    "Undefined instruction", "Prefetch abort", "Data abort"
};

/* Unexpected Interrupt or Exception handler. Currently only deals with
   exceptions, but will deal with interrupts later.
 */
void UIE(unsigned int pc, unsigned int num)
{
    char str[32];

    lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
    lcd_setfont(FONT_SYSFIXED);
#endif
    lcd_puts(0, 0, uiename[num]);
    snprintf(str, sizeof(str), "at %08x", pc);
    lcd_puts(0, 1, str);
    lcd_update();

    while (1)
    {
        /* TODO: perhaps add button handling in here when we get a polling
           driver some day.
         */
    }
}

#if CONFIG_CPU==PP5020 || CONFIG_CPU==PP5024

unsigned int ipod_hw_rev;

#ifndef BOOTLOADER
extern void TIMER1(void);
extern void TIMER2(void);

#if defined(IPOD_MINI) /* mini 1 only, mini 2G uses iPod 4G code */
extern void ipod_mini_button_int(void);

void irq(void)
{
    if(CURRENT_CORE == CPU)
    {
        if (CPU_INT_STAT & TIMER1_MASK)
            TIMER1();
        else if (CPU_INT_STAT & TIMER2_MASK)
            TIMER2();
        else if (CPU_HI_INT_STAT & GPIO_MASK)
            ipod_mini_button_int();
    } else {
        if (COP_INT_STAT & TIMER1_MASK)
            TIMER1();
        else if (COP_INT_STAT & TIMER2_MASK)
            TIMER2();
        else if (COP_HI_INT_STAT & GPIO_MASK)
            ipod_mini_button_int();
    }
}
#elif (defined IRIVER_H10) || (defined IRIVER_H10_5GB) || defined(ELIO_TPJ1022) \
    || (defined SANSA_E200)
/* TODO: this should really be in the target tree, but moving it there caused
   crt0.S not to find it while linking */
/* TODO: Even if it isn't in the target tree, this should be the default case */
void irq(void)
{
    if(CURRENT_CORE == CPU)
    {
        if (CPU_INT_STAT & TIMER1_MASK)
            TIMER1();
        else if (CPU_INT_STAT & TIMER2_MASK)
            TIMER2();
    } else {
        if (COP_INT_STAT & TIMER1_MASK)
            TIMER1();
        else if (COP_INT_STAT & TIMER2_MASK)
            TIMER2();
    }
}
#else
extern void ipod_4g_button_int(void);

void irq(void)
{
    if(CURRENT_CORE == CPU)
    {
        if (CPU_INT_STAT & TIMER1_MASK)
            TIMER1();
        else if (CPU_INT_STAT & TIMER2_MASK)
            TIMER2();
        else if (CPU_HI_INT_STAT & I2C_MASK)
            ipod_4g_button_int();
    } else {
        if (COP_INT_STAT & TIMER1_MASK)
            TIMER1();
        else if (COP_INT_STAT & TIMER2_MASK)
            TIMER2();
        else if (COP_HI_INT_STAT & I2C_MASK)
            ipod_4g_button_int();
    }
}
#endif
#endif /* BOOTLOADER */

/* TODO: The following two function have been lifted straight from IPL, and
   hence have a lot of numeric addresses used straight. I'd like to use
   #defines for these, but don't know what most of them are for or even what
   they should be named. Because of this I also have no way of knowing how
   to extend the funtions to do alternate cache configurations and/or
   some other CPU frequency scaling. */

#ifndef BOOTLOADER
static void ipod_init_cache(void)
{
/* Initialising the cache in the iPod bootloader prevents Rockbox from starting */
    unsigned i;

    /* cache init mode? */
    CACHE_CTL = CACHE_INIT;

    /* PP5002 has 8KB cache */
    for (i = 0xf0004000; i < 0xf0006000; i += 16) {
        outl(0x0, i);
    }

    outl(0x0, 0xf000f040);
    outl(0x3fc0, 0xf000f044);

    /* enable cache */
    CACHE_CTL = CACHE_ENABLE;

    for (i = 0x10000000; i < 0x10002000; i += 16)
        inb(i);
}
#endif

/* Not all iPod targets support CPU freq. boosting yet */
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void set_cpu_frequency(long frequency)
{
    unsigned long postmult;

# if NUM_CORES > 1
    /* Using mutex or spinlock isn't safe here. */
    while (test_and_set(&boostctrl_mtx.locked, 1)) ;
# endif
    
    if (frequency == CPUFREQ_NORMAL)
        postmult = CPUFREQ_NORMAL_MULT;
    else if (frequency == CPUFREQ_MAX)
        postmult = CPUFREQ_MAX_MULT;
    else
        postmult = CPUFREQ_DEFAULT_MULT;
    cpu_frequency = frequency;

    /* Enable PLL? */
    outl(inl(0x70000020) | (1<<30), 0x70000020);
    
    /* Select 24MHz crystal as clock source? */
    outl((inl(0x60006020) & 0x0fffff0f) | 0x20000020, 0x60006020);
    
    /* Clock frequency = (24/8)*postmult */
    outl(0xaa020000 | 8 | (postmult << 8), 0x60006034);
    
    /* Wait for PLL relock? */
    udelay(2000);
    
    /* Select PLL as clock source? */
    outl((inl(0x60006020) & 0x0fffff0f) | 0x20000070, 0x60006020);
    
# if defined(IPOD_COLOR) || defined(IPOD_4G) || defined(IPOD_MINI) || defined(IRIVER_H10) || defined(IRIVER_H10_5GB)
    /* We don't know why the timer interrupt gets disabled on the PP5020
     based ipods, but without the following line, the 4Gs will freeze
     when CPU frequency changing is enabled.

     Note also that a simple "CPU_INT_EN = TIMER1_MASK;" (as used
     elsewhere to enable interrupts) doesn't work, we need "|=".

     It's not needed on the PP5021 and PP5022 ipods.
     */

    /* unmask interrupt source */
    CPU_INT_EN |= TIMER1_MASK;
    COP_INT_EN |= TIMER1_MASK;
# endif
    
# if NUM_CORES > 1
    boostctrl_mtx.locked = 0;
# endif
}
#elif !defined(BOOTLOADER)
void ipod_set_cpu_frequency(void)
{
    /* Enable PLL? */
    outl(inl(0x70000020) | (1<<30), 0x70000020);

    /* Select 24MHz crystal as clock source? */
    outl((inl(0x60006020) & 0x0fffff0f) | 0x20000020, 0x60006020);

    /* Clock frequency = (24/8)*25 = 75MHz */
    outl(0xaa020000 | 8 | (25 << 8), 0x60006034);
    /* Wait for PLL relock? */
    udelay(2000);

    /* Select PLL as clock source? */
    outl((inl(0x60006020) & 0x0fffff0f) | 0x20000070, 0x60006020);
}
#endif

void system_init(void)
{
#ifndef BOOTLOADER
    if (CURRENT_CORE == CPU)
    {
        /* Remap the flash ROM from 0x00000000 to 0x20000000. */
        MMAP3_LOGICAL  = 0x20000000 | 0x3a00;
        MMAP3_PHYSICAL = 0x00000000 | 0x3f84;

        /* The hw revision is written to the last 4 bytes of SDRAM by the
           bootloader - we save it before Rockbox overwrites it. */
        ipod_hw_rev = (*((volatile unsigned long*)(0x01fffffc)));

        /* disable all irqs */
        outl(-1, 0x60001138);
        outl(-1, 0x60001128);
        outl(-1, 0x6000111c);

        outl(-1, 0x60001038);
        outl(-1, 0x60001028);
        outl(-1, 0x6000101c);
        
# if NUM_CORES > 1 && defined(HAVE_ADJUSTABLE_CPU_FREQ)
        spinlock_init(&boostctrl_mtx);
# endif
        
#if (!defined HAVE_ADJUSTABLE_CPU_FREQ) && (NUM_CORES == 1)
        ipod_set_cpu_frequency();
#endif
    }
#if (!defined HAVE_ADJUSTABLE_CPU_FREQ) && (NUM_CORES > 1)
    else
    {
        ipod_set_cpu_frequency();
    }
#endif
    ipod_init_cache();
#endif
}

void system_reboot(void)
{
    /* Reboot */
    DEV_RS |= DEV_SYSTEM;
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}
#elif CONFIG_CPU==PP5002
unsigned int ipod_hw_rev;
#ifndef BOOTLOADER
extern void TIMER1(void);
extern void TIMER2(void);

void irq(void)
{
    if(CURRENT_CORE == CPU)
    {
        if (CPU_INT_STAT & TIMER1_MASK)
            TIMER1();
        else if (CPU_INT_STAT & TIMER2_MASK)
            TIMER2();
    } else {
        if (COP_INT_STAT & TIMER1_MASK)
            TIMER1();
        else if (COP_INT_STAT & TIMER2_MASK)
            TIMER2();
    }
}

#endif

unsigned int current_core(void)
{
    if(((*(volatile unsigned long *)(0xc4000000)) & 0xff) == 0x55)
    {
        return CPU;
    }
    return COP;
}


/* TODO: The following two function have been lifted straight from IPL, and
   hence have a lot of numeric addresses used straight. I'd like to use
   #defines for these, but don't know what most of them are for or even what
   they should be named. Because of this I also have no way of knowing how
   to extend the funtions to do alternate cache configurations and/or
   some other CPU frequency scaling. */

#ifndef BOOTLOADER
static void ipod_init_cache(void)
{
    int i =0;
/* Initialising the cache in the iPod bootloader prevents Rockbox from starting */
    outl(inl(0xcf004050) & ~0x700, 0xcf004050);
    outl(0x4000, 0xcf004020);

    outl(0x2, 0xcf004024);

    /* PP5002 has 8KB cache */
    for (i = 0xf0004000; i < (int)(0xf0006000); i += 16) {
        outl(0x0, i);
    }

    outl(0x0, 0xf000f020);
    outl(0x3fc0, 0xf000f024);

    outl(0x3, 0xcf004024);
}
    
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void set_cpu_frequency(long frequency)
{
    unsigned long postmult;

    if (CURRENT_CORE == CPU)
    {
        if (frequency == CPUFREQ_NORMAL)
            postmult = CPUFREQ_NORMAL_MULT;
        else if (frequency == CPUFREQ_MAX)
            postmult = CPUFREQ_MAX_MULT;
        else
            postmult = CPUFREQ_DEFAULT_MULT;
        cpu_frequency = frequency;

        outl(0x02, 0xcf005008);
        outl(0x55, 0xcf00500c);
        outl(0x6000, 0xcf005010);

        /* Clock frequency = (24/8)*postmult */
        outl(8, 0xcf005018);
        outl(postmult, 0xcf00501c);

        outl(0xe000, 0xcf005010);

        /* Wait for PLL relock? */
        udelay(2000);

        /* Select PLL as clock source? */
        outl(0xa8, 0xcf00500c);
    }
}
#elif !defined(BOOTLOADER)
static void ipod_set_cpu_speed(void)
{
    outl(0x02, 0xcf005008);
    outl(0x55, 0xcf00500c);
    outl(0x6000, 0xcf005010);
#if 1
    // 75  MHz (24/24 * 75) (default)
    outl(24, 0xcf005018);
    outl(75, 0xcf00501c);
#endif

#if 0
    // 66 MHz (24/3 * 8)
    outl(3, 0xcf005018);
    outl(8, 0xcf00501c);
#endif

    outl(0xe000, 0xcf005010);

    udelay(2000);

    outl(0xa8, 0xcf00500c);
}
#endif

void system_init(void)
{
#ifndef BOOTLOADER
    if (CURRENT_CORE == CPU)
    {
        /* Remap the flash ROM from 0x00000000 to 0x20000000. */
        MMAP3_LOGICAL  = 0x20000000 | 0x3a00;
        MMAP3_PHYSICAL = 0x00000000 | 0x3f84;

        ipod_hw_rev = (*((volatile unsigned long*)(0x01fffffc)));
        outl(-1, 0xcf00101c);
        outl(-1, 0xcf001028);
        outl(-1, 0xcf001038);
#ifndef HAVE_ADJUSTABLE_CPU_FREQ
        ipod_set_cpu_speed();
#endif
    }
    ipod_init_cache();
#endif
}

void system_reboot(void)
{
    outl(inl(0xcf005030) | 0x4, 0xcf005030);
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

#endif /* CPU_ARM */
#endif /* CONFIG_CPU */

