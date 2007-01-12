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
#include "timer.h"
#include "inttypes.h"
#include "string.h"

#ifndef SIMULATOR
long cpu_frequency = CPU_FREQ;
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
static int boost_counter = 0;
static bool cpu_idle = false;

int get_cpu_boost_counter(void)
{
    return boost_counter;
}

void cpu_boost(bool on_off)
{
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

#if CONFIG_CPU == SH7034
#include "led.h"
#include "system.h"
#include "rolo.h"

static const char* const irqname[] = {
    "", "", "", "", "IllInstr", "", "IllSltIn","","",
    "CPUAdrEr", "DMAAdrEr", "NMI", "UserBrk",
    "","","","","","","","","","","","","","","","","","","",
    "Trap32","Trap33","Trap34","Trap35","Trap36","Trap37","Trap38","Trap39",
    "Trap40","Trap41","Trap42","Trap43","Trap44","Trap45","Trap46","Trap47",
    "Trap48","Trap49","Trap50","Trap51","Trap52","Trap53","Trap54","Trap55",
    "Trap56","Trap57","Trap58","Trap59","Trap60","Trap61","Trap62","Trap63",
    "Irq0","Irq1","Irq2","Irq3","Irq4","Irq5","Irq6","Irq7",
    "Dma0","","Dma1","","Dma2","","Dma3","",
    "IMIA0","IMIB0","OVI0","", "IMIA1","IMIB1","OVI1","",
    "IMIA2","IMIB2","OVI2","", "IMIA3","IMIB3","OVI3","",
    "IMIA4","IMIB4","OVI4","",
    "Ser0Err","Ser0Rx","Ser0Tx","Ser0TE",
    "Ser1Err","Ser1Rx","Ser1Tx","Ser1TE",
    "ParityEr","A/D conv","","","Watchdog","DRAMRefr"
};

#define RESERVE_INTERRUPT(number) "\t.long\t_UIE" #number "\n"
#define DEFAULT_INTERRUPT(name, number)  "\t.weak\t_" #name \
                          "\n\t.set\t_" #name ",_UIE" #number \
                          "\n\t.long\t_" #name "\n"

asm (

/* Vector table.
 * Handled in asm because gcc 4.x doesn't allow weak aliases to symbols
 * defined in an asm block -- silly.
 * Reset vectors (0..3) are handled in crt0.S */

    ".section\t.vectors,\"aw\",@progbits\n"
    DEFAULT_INTERRUPT (GII,      4)
    RESERVE_INTERRUPT (          5)
    DEFAULT_INTERRUPT (ISI,      6)
    RESERVE_INTERRUPT (          7)
    RESERVE_INTERRUPT (          8)
    DEFAULT_INTERRUPT (CPUAE,    9)
    DEFAULT_INTERRUPT (DMAAE,   10)
    DEFAULT_INTERRUPT (NMI,     11)
    DEFAULT_INTERRUPT (UB,      12)
    RESERVE_INTERRUPT (         13)
    RESERVE_INTERRUPT (         14)
    RESERVE_INTERRUPT (         15)
    RESERVE_INTERRUPT (         16) /* TCB #0 */
    RESERVE_INTERRUPT (         17) /* TCB #1 */
    RESERVE_INTERRUPT (         18) /* TCB #2 */
    RESERVE_INTERRUPT (         19) /* TCB #3 */
    RESERVE_INTERRUPT (         20) /* TCB #4 */
    RESERVE_INTERRUPT (         21) /* TCB #5 */
    RESERVE_INTERRUPT (         22) /* TCB #6 */
    RESERVE_INTERRUPT (         23) /* TCB #7 */
    RESERVE_INTERRUPT (         24) /* TCB #8 */
    RESERVE_INTERRUPT (         25) /* TCB #9 */
    RESERVE_INTERRUPT (         26) /* TCB #10 */
    RESERVE_INTERRUPT (         27) /* TCB #11 */
    RESERVE_INTERRUPT (         28) /* TCB #12 */
    RESERVE_INTERRUPT (         29) /* TCB #13 */
    RESERVE_INTERRUPT (         30) /* TCB #14 */
    RESERVE_INTERRUPT (         31) /* TCB #15 */
    DEFAULT_INTERRUPT (TRAPA32, 32)
    DEFAULT_INTERRUPT (TRAPA33, 33)
    DEFAULT_INTERRUPT (TRAPA34, 34)
    DEFAULT_INTERRUPT (TRAPA35, 35)
    DEFAULT_INTERRUPT (TRAPA36, 36)
    DEFAULT_INTERRUPT (TRAPA37, 37)
    DEFAULT_INTERRUPT (TRAPA38, 38)
    DEFAULT_INTERRUPT (TRAPA39, 39)
    DEFAULT_INTERRUPT (TRAPA40, 40)
    DEFAULT_INTERRUPT (TRAPA41, 41)
    DEFAULT_INTERRUPT (TRAPA42, 42)
    DEFAULT_INTERRUPT (TRAPA43, 43)
    DEFAULT_INTERRUPT (TRAPA44, 44)
    DEFAULT_INTERRUPT (TRAPA45, 45)
    DEFAULT_INTERRUPT (TRAPA46, 46)
    DEFAULT_INTERRUPT (TRAPA47, 47)
    DEFAULT_INTERRUPT (TRAPA48, 48)
    DEFAULT_INTERRUPT (TRAPA49, 49)
    DEFAULT_INTERRUPT (TRAPA50, 50)
    DEFAULT_INTERRUPT (TRAPA51, 51)
    DEFAULT_INTERRUPT (TRAPA52, 52)
    DEFAULT_INTERRUPT (TRAPA53, 53)
    DEFAULT_INTERRUPT (TRAPA54, 54)
    DEFAULT_INTERRUPT (TRAPA55, 55)
    DEFAULT_INTERRUPT (TRAPA56, 56)
    DEFAULT_INTERRUPT (TRAPA57, 57)
    DEFAULT_INTERRUPT (TRAPA58, 58)
    DEFAULT_INTERRUPT (TRAPA59, 59)
    DEFAULT_INTERRUPT (TRAPA60, 60)
    DEFAULT_INTERRUPT (TRAPA61, 61)
    DEFAULT_INTERRUPT (TRAPA62, 62)
    DEFAULT_INTERRUPT (TRAPA63, 63)
    DEFAULT_INTERRUPT (IRQ0,    64)
    DEFAULT_INTERRUPT (IRQ1,    65)
    DEFAULT_INTERRUPT (IRQ2,    66)
    DEFAULT_INTERRUPT (IRQ3,    67)
    DEFAULT_INTERRUPT (IRQ4,    68)
    DEFAULT_INTERRUPT (IRQ5,    69)
    DEFAULT_INTERRUPT (IRQ6,    70)
    DEFAULT_INTERRUPT (IRQ7,    71)
    DEFAULT_INTERRUPT (DEI0,    72)
    RESERVE_INTERRUPT (         73)
    DEFAULT_INTERRUPT (DEI1,    74)
    RESERVE_INTERRUPT (         75)
    DEFAULT_INTERRUPT (DEI2,    76)
    RESERVE_INTERRUPT (         77)
    DEFAULT_INTERRUPT (DEI3,    78)
    RESERVE_INTERRUPT (         79)
    DEFAULT_INTERRUPT (IMIA0,   80)
    DEFAULT_INTERRUPT (IMIB0,   81)
    DEFAULT_INTERRUPT (OVI0,    82)
    RESERVE_INTERRUPT (         83)
    DEFAULT_INTERRUPT (IMIA1,   84)
    DEFAULT_INTERRUPT (IMIB1,   85)
    DEFAULT_INTERRUPT (OVI1,    86)
    RESERVE_INTERRUPT (         87)
    DEFAULT_INTERRUPT (IMIA2,   88)
    DEFAULT_INTERRUPT (IMIB2,   89)
    DEFAULT_INTERRUPT (OVI2,    90)
    RESERVE_INTERRUPT (         91)
    DEFAULT_INTERRUPT (IMIA3,   92)
    DEFAULT_INTERRUPT (IMIB3,   93)
    DEFAULT_INTERRUPT (OVI3,    94)
    RESERVE_INTERRUPT (         95)
    DEFAULT_INTERRUPT (IMIA4,   96)
    DEFAULT_INTERRUPT (IMIB4,   97)
    DEFAULT_INTERRUPT (OVI4,    98)
    RESERVE_INTERRUPT (         99)
    DEFAULT_INTERRUPT (REI0,   100)
    DEFAULT_INTERRUPT (RXI0,   101)
    DEFAULT_INTERRUPT (TXI0,   102)
    DEFAULT_INTERRUPT (TEI0,   103)
    DEFAULT_INTERRUPT (REI1,   104)
    DEFAULT_INTERRUPT (RXI1,   105)
    DEFAULT_INTERRUPT (TXI1,   106)
    DEFAULT_INTERRUPT (TEI1,   107)
    RESERVE_INTERRUPT (        108)
    DEFAULT_INTERRUPT (ADITI,  109)

/* UIE# block.
 * Must go into the same section as the UIE() handler */

    "\t.text\n"
    "_UIE4:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE5:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE6:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE7:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE8:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE9:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE10:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE11:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE12:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE13:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE14:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE15:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE16:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE17:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE18:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE19:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE20:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE21:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE22:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE23:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE24:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE25:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE26:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE27:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE28:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE29:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE30:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE31:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE32:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE33:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE34:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE35:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE36:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE37:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE38:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE39:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE40:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE41:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE42:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE43:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE44:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE45:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE46:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE47:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE48:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE49:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE50:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE51:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE52:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE53:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE54:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE55:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE56:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE57:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE58:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE59:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE60:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE61:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE62:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE63:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE64:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE65:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE66:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE67:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE68:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE69:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE70:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE71:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE72:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE73:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE74:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE75:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE76:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE77:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE78:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE79:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE80:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE81:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE82:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE83:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE84:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE85:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE86:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE87:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE88:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE89:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE90:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE91:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE92:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE93:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE94:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE95:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE96:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE97:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE98:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE99:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE100:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE101:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE102:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE103:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE104:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE105:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE106:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE107:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE108:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"
    "_UIE109:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\n"

);

extern void UIE4(void); /* needed for calculating the UIE number */

void UIE (unsigned int pc) __attribute__((section(".text")));
void UIE (unsigned int pc) /* Unexpected Interrupt or Exception */
{
#if CONFIG_LED == LED_REAL
    bool state = false;
    int i = 0;
#endif
    unsigned int n;
    char str[32];

    asm volatile ("sts\tpr,%0" : "=r"(n));

    /* clear screen */
    lcd_clear_display ();
#ifdef HAVE_LCD_BITMAP
    lcd_setfont(FONT_SYSFIXED);
#endif
    /* output exception */
    n = (n - (unsigned)UIE4 + 12)>>2; /* get exception or interrupt number */
    snprintf(str,sizeof(str),"I%02x:%s",n,irqname[n]);
    lcd_puts(0,0,str);
    snprintf(str,sizeof(str),"at %08x",pc);
    lcd_puts(0,1,str);

#ifdef HAVE_LCD_BITMAP
    lcd_update ();
#endif

    while (1)
    {
#if CONFIG_LED == LED_REAL
        if (--i <= 0)
        {
            state = !state;
            led(state);
            i = 240000;
        }
#endif

        /* try to restart firmware if ON is pressed */
#if CONFIG_KEYPAD == PLAYER_PAD
        if (!(PADRL & 0x20))
#elif CONFIG_KEYPAD == RECORDER_PAD
#ifdef HAVE_FMADC
        if (!(PCDR & 0x0008))
#else
        if (!(PBDRH & 0x01))
#endif
#elif CONFIG_KEYPAD == ONDIO_PAD
        if (!(PCDR & 0x0008))
#endif
        {
            /* enable the watchguard timer, but don't service it */
            RSTCSR_W = 0x5a40; /* Reset enabled, power-on reset */
            TCSR_W = 0xa560; /* Watchdog timer mode, timer enabled, sysclk/2 */
        }
    }
}

void system_init(void)
{
    /* Disable all interrupts */
    IPRA = 0;
    IPRB = 0;
    IPRC = 0;
    IPRD = 0;
    IPRE = 0;

    /* NMI level low, falling edge on all interrupts */
    ICR = 0;

    /* Enable burst and RAS down mode on DRAM */
    DCR |= 0x5000;

    /* Activate Warp mode (simultaneous internal and external mem access) */
    BCR |= 0x2000;

    /* Bus state controller initializations. These are only necessary when
       running from flash. */
    WCR1 = 0x40FD; /* Long wait states for CS6 (ATA), short for the rest. */
    WCR3 = 0x8000; /* WAIT is pulled up, 1 state inserted for CS6 */
}

void system_reboot (void)
{
    set_irq_level(HIGHEST_IRQ_LEVEL);

    asm volatile ("ldc\t%0,vbr" : : "r"(0));

    PACR2 |= 0x4000; /* for coldstart detection */
    IPRA = 0;
    IPRB = 0;
    IPRC = 0;
    IPRD = 0;
    IPRE = 0;
    ICR = 0;

    asm volatile ("jmp @%0; mov.l @%1,r15" : :
                  "r"(*(int*)0),"r"(4));
}

/* Utilise the user break controller to catch invalid memory accesses. */
int system_memory_guard(int newmode)
{
    static const struct {
        unsigned long  addr;
        unsigned long  mask;
        unsigned short bbr;
    } modes[MAXMEMGUARD] = {
        /* catch nothing */
        { 0x00000000, 0x00000000, 0x0000 },
        /* catch writes to area 02 (flash ROM) */
        { 0x02000000, 0x00FFFFFF, 0x00F8 },
        /* catch all accesses to areas 00 (internal ROM) and 01 (free) */
        { 0x00000000, 0x01FFFFFF, 0x00FC }
    };

    int oldmode = MEMGUARD_NONE;
    int i;

    /* figure out the old mode from what is in the UBC regs. If the register
       values don't match any mode, assume MEMGUARD_NONE */
    for (i = MEMGUARD_NONE; i < MAXMEMGUARD; i++)
    {
        if (BAR == modes[i].addr && BAMR == modes[i].mask &&
            BBR == modes[i].bbr)
        {
            oldmode = i;
            break;
        }
    }

    if (newmode == MEMGUARD_KEEP)
        newmode = oldmode;

    BBR = 0;  /* switch off everything first */

    /* always set the UBC according to the mode, in case the old settings
       didn't match any valid mode */
    BAR  = modes[newmode].addr;
    BAMR = modes[newmode].mask;
    BBR  = modes[newmode].bbr;

    return oldmode;
}
#elif defined(CPU_ARM)

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
    if (CPU_INT_STAT & TIMER1_MASK)
        TIMER1();
    else if (CPU_INT_STAT & TIMER2_MASK)
        TIMER2();
    else if (CPU_HI_INT_STAT & GPIO_MASK)
        ipod_mini_button_int();
}
#elif (defined IRIVER_H10) || (defined IRIVER_H10_5GB) || defined(ELIO_TPJ1022) \
    || (defined SANSA_E200)
/* TODO: this should really be in the target tree, but moving it there caused
   crt0.S not to find it while linking */
/* TODO: Even if it isn't in the target tree, this should be the default case */
void irq(void)
{
    if (CPU_INT_STAT & TIMER1_MASK)
        TIMER1();
    else if (CPU_INT_STAT & TIMER2_MASK)
        TIMER2();
}
#else
extern void ipod_4g_button_int(void);

void irq(void)
{
    if (CPU_INT_STAT & TIMER1_MASK)
        TIMER1();
    else if (CPU_INT_STAT & TIMER2_MASK)
        TIMER2();
    else if (CPU_HI_INT_STAT & I2C_MASK)
        ipod_4g_button_int();
}
#endif
#endif /* BOOTLOADER */

unsigned int current_core(void)
{
    if(((*(volatile unsigned long *)(0x60000000)) & 0xff) == 0x55)
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
/* Initialising the cache in the iPod bootloader prevents Rockbox from starting */
    unsigned i;

    /* cache init mode? */
    outl(0x4, 0x6000C000);

    /* PP5002 has 8KB cache */
    for (i = 0xf0004000; i < 0xf0006000; i += 16) {
        outl(0x0, i);
    }

    outl(0x0, 0xf000f040);
    outl(0x3fc0, 0xf000f044);

    /* enable cache */
    outl(0x1, 0x6000C000);

    for (i = 0x10000000; i < 0x10002000; i += 16)
        inb(i);
}
#endif

/* Not all iPod targets support CPU freq. boosting yet */
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void set_cpu_frequency(long frequency)
{
    unsigned long postmult;

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

#if defined(IPOD_COLOR) || defined(IPOD_4G) || defined(IPOD_MINI) || defined(IRIVER_H10) || defined(IRIVER_H10_5GB)
    /* We don't know why the timer interrupt gets disabled on the PP5020
       based ipods, but without the following line, the 4Gs will freeze
       when CPU frequency changing is enabled.

       Note also that a simple "CPU_INT_EN = TIMER1_MASK;" (as used
       elsewhere to enable interrupts) doesn't work, we need "|=".

       It's not needed on the PP5021 and PP5022 ipods.
    */

    /* unmask interrupt source */
    CPU_INT_EN |= TIMER1_MASK;
#endif
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
#ifndef HAVE_ADJUSTABLE_CPU_FREQ
    ipod_set_cpu_frequency();
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
    if (CPU_INT_STAT & TIMER1_MASK)
        TIMER1();
    else if (CPU_INT_STAT & TIMER2_MASK)
        TIMER2();
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
    ipod_hw_rev = (*((volatile unsigned long*)(0x01fffffc)));
    outl(-1, 0xcf00101c);
    outl(-1, 0xcf001028);
    outl(-1, 0xcf001038);
#ifndef HAVE_ADJUSTABLE_CPU_FREQ
    ipod_set_cpu_speed();
#endif
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

#elif CONFIG_CPU==PNX0101

interrupt_handler_t interrupt_vector[0x1d]  __attribute__ ((section(".idata")));

#define IRQ_REG(reg) (*(volatile unsigned long *)(0x80300000 + (reg)))

static inline unsigned long irq_read(int reg)
{
    unsigned long v, v2;
    do
    {
        v = IRQ_REG(reg);
        v2 = IRQ_REG(reg);
    } while (v != v2);
    return v;
}

#define IRQ_WRITE_WAIT(reg, val, cond) \
    do { unsigned long v, v2; \
        do { \
            IRQ_REG(reg) = (val); \
            v = IRQ_REG(reg); \
            v2 = IRQ_REG(reg); \
        } while ((v != v2) || !(cond)); \
    } while (0);

static void undefined_int(void)
{
}

void irq(void)
{
    int n = irq_read(0x100) >> 3;
    (*(interrupt_vector[n]))();
}

void fiq(void)
{
}

void irq_enable_int(int n)
{
    IRQ_WRITE_WAIT(0x404 + n * 4, 0x4010000, v & 0x10000);
}

void irq_set_int_handler(int n, interrupt_handler_t handler)
{
    interrupt_vector[n + 1] = handler;
}

void system_init(void)
{
    int i;

    /* turn off watchdog */
    (*(volatile unsigned long *)0x80002804) = 0;

    /*
    IRQ_WRITE_WAIT(0x100, 0, v == 0);
    IRQ_WRITE_WAIT(0x104, 0, v == 0);
    IRQ_WRITE_WAIT(0, 0, v == 0);
    IRQ_WRITE_WAIT(4, 0, v == 0);
    */

    for (i = 0; i < 0x1c; i++)
    {
        IRQ_WRITE_WAIT(0x404 + i * 4, 0x1e000001, (v & 0x3010f) == 1);
        IRQ_WRITE_WAIT(0x404 + i * 4, 0x4000000, (v & 0x10000) == 0);
        IRQ_WRITE_WAIT(0x404 + i * 4, 0x10000001, (v & 0xf) == 1);
        interrupt_vector[i + 1] = undefined_int;
    }
    interrupt_vector[0] = undefined_int;
}


void system_reboot(void)
{
    (*(volatile unsigned long *)0x80002804) = 1;
    while (1);
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

#endif /* CPU_ARM */
#endif /* CONFIG_CPU */

