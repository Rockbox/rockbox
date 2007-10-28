/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __TCC77X_H__
#define __TCC77X_H__

/* General-purpose IO */

#define GPIOA      (*(volatile unsigned long *)0x80000300)
#define GPIOB      (*(volatile unsigned long *)0x80000310)
#define GPIOC      (*(volatile unsigned long *)0x80000320)
#define GPIOD      (*(volatile unsigned long *)0x80000330)
#define GPIOE      (*(volatile unsigned long *)0x80000340)

#define GPIOA_DIR  (*(volatile unsigned long *)0x80000304)
#define GPIOB_DIR  (*(volatile unsigned long *)0x80000314)
#define GPIOC_DIR  (*(volatile unsigned long *)0x80000324)
#define GPIOD_DIR  (*(volatile unsigned long *)0x80000334)
#define GPIOE_DIR  (*(volatile unsigned long *)0x80000344)

#define GPIOA_FUNC (*(volatile unsigned long *)0x80000308)
#define GPIOB_FUNC (*(volatile unsigned long *)0x80000318)
#define GPIOC_FUNC (*(volatile unsigned long *)0x80000328)
#define GPIOD_FUNC (*(volatile unsigned long *)0x80000338)
#define GPIOE_FUNC (*(volatile unsigned long *)0x80000348)

#define BMI        (*(volatile unsigned long *)0x80000364)

/* Clock Generator */

#define CLKCTRL    (*(volatile unsigned long *)0x80000400)
#define PLL0CFG    (*(volatile unsigned long *)0x80000404)
#define CLKDIV0    (*(volatile unsigned long *)0x8000040c)
#define MODECTR    (*(volatile unsigned long *)0x80000410)
#define BCLKCTR    (*(volatile unsigned long *)0x80000414)
#define SWRESET    (*(volatile unsigned long *)0x80000418)
#define PCLKCFG0   (*(volatile unsigned long *)0x8000041c)
#define PCLKCFG1   (*(volatile unsigned long *)0x80000420)
#define PCLKCFG2   (*(volatile unsigned long *)0x80000424)
#define PCLKCFG3   (*(volatile unsigned long *)0x80000428)
#define PCLKCFG4   (*(volatile unsigned long *)0x8000042c)
#define PCLKCFG5   (*(volatile unsigned long *)0x80000430)
#define PCLKCFG6   (*(volatile unsigned long *)0x80000434)

/* ADC */

#define ADCCON     (*(volatile unsigned long *)0x80000a00)
#define ADCDATA    (*(volatile unsigned long *)0x80000a04)
#define ADCCONA    (*(volatile unsigned long *)0x80000a80)
#define ADCSTATUS  (*(volatile unsigned long *)0x80000a84)
#define ADCCFG     (*(volatile unsigned long *)0x80000a88)


/* Memory Controller */
#define SDCFG      (*(volatile unsigned long *)0xf0000000)
#define SDFSM      (*(volatile unsigned long *)0xf0000004)
#define MCFG       (*(volatile unsigned long *)0xf0000008)
#define TST        (*(volatile unsigned long *)0xf000000c)
#define CSCFG0     (*(volatile unsigned long *)0xf0000010)
#define CSCFG1     (*(volatile unsigned long *)0xf0000014)
#define CSCFG2     (*(volatile unsigned long *)0xf0000018)
#define CSCFG3     (*(volatile unsigned long *)0xf000001c)
#define CLKCFG     (*(volatile unsigned long *)0xf0000020)
#define SDCMD      (*(volatile unsigned long *)0xf0000024)

#endif
