/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "kernel.h"
#include "system.h"
#include "panic.h"

void system_reboot(void)
{
}

/* TODO - these should live in the target-specific directories and
   once we understand what all the GPIO pins do, move the init to the
   specific driver for that hardware.   For now, we just perform the 
   same GPIO init as the original firmware - this makes it easier to
   investigate what the GPIO pins do.
*/

#ifdef LOGIK_DAX
static void gpio_init(void)
{
    /* Do what the original firmware does */
    GPIOD_FUNC = 0;
    GPIOD_DIR = 0x3f0;
    GPIOD = 0xe0;
    GPIOE_FUNC = 0;
    GPIOE_DIR = 0xe0;
    GPIOE = 0;
    GPIOA_FUNC = 0;
    GPIOA_DIR = 0xffff1000;   /* 0 - 0xf000 */
    GPIOA = 0x1080;
    GPIOB_FUNC = 0x16a3;
    GPIOB_DIR = 0x6ffff;
    GPIOB = 0;
    GPIOC_FUNC = 1;
    GPIOC_DIR = 0x03ffffff;  /* mvn     r2, 0xfc000000 */
    GPIOC = 0;
}
#elif defined(IAUDIO_7)
static void gpio_init(void)
{
    /* Do what the original firmware does */
    GPIOA_FUNC = 0;
    GPIOB_FUNC = 0x1623;
    GPIOC_FUNC = 1;
    GPIOD_FUNC = 0;
    GPIOE_FUNC = 0;
    GPIOA = 0x30;
    GPIOB = 0x80000;
    GPIOC = 0;
    GPIOD = 0x180;
    GPIOE = 0;
    GPIOA_DIR = 0x84b0
    GPIOB_DIR = 0x80800;
    GPIOC_DIR = 0x2000000;
    GPIOD_DIR = 0x3e3;
    GPIOE_DIR = 0x88;
}
#endif

/* Second function called in the original firmware's startup code - we just
   set up the clocks in the same way as the original firmware for now. */
static void clock_init(void)
{
    unsigned int i;

    CSCFG3 = (CSCFG3 &~ 0x3fff) | 0x820;

    CLKCTRL = (CLKCTRL & ~0xff) | 0x14;

    if (BMI & 0x20)
      PCLKCFG0 = 0xc82d7000;
    else
      PCLKCFG0 = 0xc8ba7000;

    MCFG |= 0x2000;

#ifdef LOGIK_DAX
    /* Only seen in the Logik DAX original firmware */
    SDCFG = (SDCFG & ~0x7000) | 0x2000;
#endif

    PLL0CFG |= 0x80000000;

    PLL0CFG = 0x0000cf13;

    i = 8000;
    while (--i) {};

    CLKDIV0 = 0x81000000;
    CLKCTRL = 0x80000010;

    asm volatile (
        "nop      \n\t"
        "nop      \n\t"
    );
}


void system_init(void)
{
    /* TODO: cache init - the original firmwares have cache init code which
       is called at the very start of the firmware */
    clock_init();
    gpio_init();
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ

void set_cpu_frequency(long frequency)
{
}

#endif
