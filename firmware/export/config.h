/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifndef __PCTOOL__
#include "autoconf.h"
#endif

/* symbolic names for multiple choice configurations: */

/* CONFIG_TUNER (note these are combineable bit-flags) */
#define S1A0903X01 0x01 /* Samsung */
#define TEA5767    0x02 /* Philips */

/* CONFIG_CODEC */
#define MAS3587F 3587
#define MAS3507D 3507
#define MAS3539F 3539
#define SWCODEC  1    /* if codec is done by SW */

/* CONFIG_CPU */
#define SH7034       7034
#define MCF5249      5249
#define MCF5250      5250
#define PP5002       5002
#define PP5020       5020
#define PNX0101       101
#define S3C2440      2440
#define PP5024       5024
#define TMS320DSC25    25

/* CONFIG_KEYPAD */
#define PLAYER_PAD          1
#define RECORDER_PAD        2
#define ONDIO_PAD           3
#define IRIVER_H100_PAD     4
#define IRIVER_H300_PAD     5
#define IAUDIO_X5M5_PAD     6
#define IPOD_4G_PAD         7
#define IPOD_3G_PAD         8
#define IRIVER_IFP7XX_PAD   9
#define GIGABEAT_PAD       10
#define IRIVER_H10_PAD     11
#define SANSA_E200_PAD     12
#define ELIO_TPJ1022_PAD   13
#define ARCHOS_AV300_PAD   14

/* CONFIG_REMOTE_KEYPAD */
#define H100_REMOTE 1
#define H300_REMOTE 2
#define X5_REMOTE   3

/* CONFIG_BATTERY */
#define BATT_LIION2200   2200 /* FM/V2 recorder type */
#define BATT_4AA_NIMH    1500
#define BATT_3AAA        1000 /* Ondio */
#define BATT_IAUDIO_X5M5  950
#define BATT_LIPOL1300   1300 /* the type used in iRiver h1x0 models */
#define BATT_LPCS355385  1550 /* iriver h10 20Gb - SKC LPCS355385 */
#define BATT_BP009        820 /* iriver H10 5/6Gb - iriver BP009 */
#define BATT_LIION830     830 /* Toshiba Gigabeat Fxx and Xxx series MK11-2740 */
#define BATT_1AA          333 /* iRiver iFP: Alkaline, NiHM */

/* CONFIG_CHARGING */
#define CHARGING_SIMPLE  1 /* Simple, hardware controlled charging */
#define CHARGING_MONITOR 2 /* Hardware controlled charging with monitoring */
#define CHARGING_CONTROL 3 /* Software controlled charging */

/* CONFIG_LCD */
#define LCD_SSD1815   1 /* as used by Archos Recorders and Ondios */
#define LCD_SSD1801   2 /* as used by Archos Player/Studio */
#define LCD_S1D15E06  3 /* as used by iRiver H100 series */
#define LCD_H300      4 /* as used by iRiver H300 series, exact model name is
                           unknown at the time of this writing */
#define LCD_X5        5 /* as used by iAudio X5 series, exact model name is
                          unknown at the time of this writing */
#define LCD_IPODCOLOR 6 /* as used by iPod Color/Photo */
#define LCD_IPODNANO  7 /* as used by iPod Nano */
#define LCD_IPODVIDEO 8 /* as used by iPod Video */
#define LCD_IPOD2BPP  9 /* as used by all fullsize greyscale iPods */
#define LCD_IPODMINI 10 /* as used by iPod Mini g1/g2 */
#define LCD_IFP7XX   11 /* as used by iRiver iFP 7xx/8xx */
#define LCD_GIGABEAT 12
#define LCD_H10_20GB 13 /* as used by iriver H10 20Gb */
#define LCD_H10_5GB  14 /* as used by iriver H10 5Gb */
#define LCD_TPJ1022  15 /* as used by Tatung Elio TPJ-1022 */
#define LCD_DSC25    16 /* as used by Archos AV300 */

/* LCD_PIXELFORMAT */
#define HORIZONTAL_PACKING 1
#define VERTICAL_PACKING 2
#define HORIZONTAL_INTERLEAVED 3
#define VERTICAL_INTERLEAVED 4
#define RGB565 565
#define RGB565SWAPPED 3553

/* CONFIG_BACKLIGHT */
#define BL_PA14_LO       1 /* Player, PA14 low active */
#define BL_RTC           2 /* Recorder, RTC square wave output */
#define BL_PA14_HI       3 /* Ondio, PA14 high active */
#define BL_IRIVER_H100   4 /* IRiver GPIO */
#define BL_IPOD4G        5 /* Apple iPod 4G */
#define BL_IPODNANO      6 /* Apple iPod Nano and iPod Video*/
#define BL_IPOD3G        7 /* Apple iPod 3g with dock */
#define BL_IRIVER_H300   8 /* IRiver PWM */
#define BL_IRIVER_IFP7XX 9 /* IRiver GPIO */
#define BL_IPODMINI      10 /* Apple iPod Mini */
#define BL_GIGABEAT      11 /* Toshiba Gigabeat */
#define BL_H10           12 /* iriver H10 */
#define BL_X5            13 /* iAudio X5 PCF50606 I2C */
#define BL_H10_5GB       14 /* iriver H10 5/6GB */
#define BL_TPJ1022       15 /* Tatung Elio TPJ-1022 */
#define BL_M5            16 /* iAudio M5 PCF50606 I2C */

/* CONFIG_I2C */
#define I2C_PLAYREC  1 /* Archos Player/Recorder style */
#define I2C_ONDIO    2 /* Ondio style */
#define I2C_COLDFIRE 3 /* Coldfire style */
#define I2C_PP5002   4 /* PP5002 style */
#define I2C_PP5020   5 /* PP5020 style */
#define I2C_PNX0101  6 /* PNX0101 style */
#define I2C_S3C2440  7

/* CONFIG_LED */
#define LED_REAL     1 /* SW controlled LED (Archos recorders, player) */
#define LED_VIRTUAL  2 /* Virtual LED (icon) (Archos Ondio) */
/* else                   HW controlled LED (iRiver H1x0) */

/* CONFIG_FLASH */
#define FLASH_IFP7XX 1

/* CONFIG_RTC */
#define RTC_M41ST84W 1 /* Archos Recorder */
#define RTC_PCF50605 2 /* iPod 3G, 4G & Mini */
#define RTC_PCF50606 3 /* iriver H300 */
#define RTC_S3C2440  4
#define RTC_E8564    5 /* iriver H10 */
#define RTC_AS3514   6 /* Sandisk Sansa e200 series */
#define RTC_DS1339_DS3231   7 /* h1x0 RTC mod */

/* USB On-the-go */
#define USBOTG_ISP1362 1362 /* iriver H300 */
#define USBOTG_M5636   5636 /* iAudio X5 */
#define USBOTG_ARC     5020 /* PortalPlayer 502x */

/* Multiple cores */
#define CPU 0
#define COP 1

/* now go and pick yours */
#if defined(ARCHOS_PLAYER)
#include "config-player.h"
#elif defined(ARCHOS_RECORDER)
#include "config-recorder.h"
#elif defined(ARCHOS_FMRECORDER)
#include "config-fmrecorder.h"
#elif defined(ARCHOS_RECORDERV2)
#include "config-recorderv2.h"
#elif defined(ARCHOS_ONDIOSP)
#include "config-ondiosp.h"
#elif defined(ARCHOS_ONDIOFM)
#include "config-ondiofm.h"
#elif defined(ARCHOS_AV300)
#include "config-av300.h"
#elif defined(IRIVER_H100)
#include "config-h100.h"
#elif defined(IRIVER_H120)
#include "config-h120.h"
#elif defined(IRIVER_H300)
#include "config-h300.h"
#elif defined(IAUDIO_X5)
#include "config-iaudiox5.h"
#elif defined(IAUDIO_M5)
#include "config-iaudiom5.h"
#elif defined(IPOD_COLOR)
#include "config-ipodcolor.h"
#elif defined(IPOD_NANO)
#include "config-ipodnano.h"
#elif defined(IPOD_VIDEO)
#include "config-ipodvideo.h"
#elif defined(IPOD_3G)
#include "config-ipod3g.h"
#elif defined(IPOD_4G)
#include "config-ipod4g.h"
#elif defined(IRIVER_IFP7XX)
#include "config-ifp7xx.h"
#elif defined(GIGABEAT_F)
#include "config-gigabeat.h"
#elif defined(IPOD_MINI)
#include "config-ipodmini.h"
#elif defined(IPOD_MINI2G)
#include "config-ipodmini2g.h"
#elif defined(IRIVER_H10)
#include "config-h10.h"
#elif defined(IRIVER_H10_5GB)
#include "config-h10_5gb.h"
#elif defined(SANSA_E200)
#include "config-e200.h"
#elif defined(ELIO_TPJ1022)
#include "config-tpj1022.h"
#else
/* no known platform */
#endif

/* now set any CONFIG_ defines correctly if they are not used,
   No need to do this on CONFIG_'s which are compulsary (e.g CONFIG_CODEC ) */
#ifndef CONFIG_TUNER
#define CONFIG_TUNER 0
#endif

#ifndef CONFIG_USBOTG
#define CONFIG_USBOTG 0
#endif

#ifndef CONFIG_LED
#define CONFIG_LED 0
#endif

#ifndef CONFIG_BACKLIGHT
#define CONFIG_BACKLIGHT 0
#endif

#ifndef CONFIG_CHARGING
#define CONFIG_CHARGING 0
#endif

#ifndef CONFIG_RTC
#define CONFIG_RTC 0
#endif

/* Enable the directory cache and tagcache in RAM if we have
 * plenty of RAM. Both features can be enabled independently. */
#if ((defined(MEMORYSIZE) && (MEMORYSIZE > 8)) || MEM > 8) && \
 !defined(BOOTLOADER)
#define HAVE_DIRCACHE
#ifdef HAVE_TAGCACHE
#define HAVE_TC_RAMCACHE
#endif
#endif

#if CONFIG_CODEC == SWCODEC && !defined(BOOTLOADER)
#define HAVE_EXTENDED_MESSAGING_AND_NAME
#endif

#if (CONFIG_CODEC == SWCODEC) && !defined(SIMULATOR) && !defined(BOOTLOADER)
#define HAVE_PRIORITY_SCHEDULING
#define HAVE_SCHEDULER_BOOSTCTRL
#endif

/* define for all cpus from SH family */
#if (CONFIG_CPU == SH7034)
#define CPU_SH
#endif

/* define for all cpus from coldfire family */
#if (CONFIG_CPU == MCF5249) || (CONFIG_CPU == MCF5250)
#define CPU_COLDFIRE
#endif

/* define for all cpus from PP family */
#if (CONFIG_CPU == PP5002) || (CONFIG_CPU == PP5020) || (CONFIG_CPU == PP5024)
#define CPU_PP
#endif

/* define for all cpus from ARM family */
#if defined(CPU_PP) || (CONFIG_CPU == PNX0101) || (CONFIG_CPU == S3C2440) \
    || (CONFIG_CPU == TMS320DSC25)
#define CPU_ARM
#endif

/* Determine if accesses should be strictly long aligned. */
#if (CONFIG_CPU == SH7034) || defined(CPU_ARM)
#define ROCKBOX_STRICT_ALIGN 1
#endif

#ifndef CODEC_SIZE
#define CODEC_SIZE 0
#endif

/* This attribute can be used to ensure that certain symbols are never profiled
 * which can be important as profiling a function de-inlines it */
#ifdef RB_PROFILE
#define NO_PROF_ATTR __attribute__ ((no_instrument_function))
#else
#define NO_PROF_ATTR
#endif

/* IRAM usage */
#if !defined(SIMULATOR) &&   /* Not for simulators */ \
    (((CONFIG_CPU == SH7034) && !defined(PLUGIN)) || /* SH1 archos: core only */ \
    defined(CPU_COLDFIRE) || /* Coldfire: core, plugins, codecs */ \
    defined(CPU_PP) ||  /* PortalPlayer: core, plugins, codecs */ \
    (CONFIG_CPU == PNX0101))
#define ICODE_ATTR      __attribute__ ((section(".icode")))
#define ICONST_ATTR     __attribute__ ((section(".irodata")))
#define IDATA_ATTR      __attribute__ ((section(".idata")))
#define IBSS_ATTR       __attribute__ ((section(".ibss")))
#define USE_IRAM
#if CONFIG_CPU != SH7034
#define IRAM_STEAL
#endif
#if defined(CPU_ARM)
/* GCC quirk workaround: arm-elf-gcc treats static functions as short_call
 * when not compiling with -ffunction-sections, even when the function has
 * a section attribute. */
#define STATICIRAM
#else
#define STATICIRAM static
#endif
#else
#define ICODE_ATTR
#define ICONST_ATTR
#define IDATA_ATTR
#define IBSS_ATTR
#define STATICIRAM static
#endif

#ifndef IRAM_LCDFRAMEBUFFER
/* if the LCD framebuffer has not been moved to IRAM, define it empty here */
#define IRAM_LCDFRAMEBUFFER
#endif

/* Dual core support - not yet working on the 3G iPod */
#if defined(CPU_PP) && CONFIG_CPU != PP5002
#define NUM_CORES 2
#define CURRENT_CORE current_core()
/* Hopefully at some point we will learn how to mark areas of main memory as
 * not to be cached.  Until then, use IRAM for variables shared across cores */
#define NOCACHEBSS_ATTR IBSS_ATTR
#define NOCACHEDATA_ATTR IDATA_ATTR

#define IF_COP(empty, x, y) , x, y

/* Defines for inter-core messaging */
#define COP_REBOOT 1

#else
#define NUM_CORES 1
#define CURRENT_CORE CPU
#define NOCACHEBSS_ATTR
#define NOCACHEDATA_ATTR

#define IF_COP(empty, x, y)

#endif /* Processor specific */

#endif
