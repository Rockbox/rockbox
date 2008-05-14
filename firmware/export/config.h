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
#define LV24020LP  0x04 /* Sanyo */
#define SI4700     0x08 /* Silicon Labs */

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
#define PP5022       5022
#define PP5024       5024
#define PNX0101       101
#define S3C2440      2440
#define DSC25          25
#define DM320         320
#define IMX31L         31
#define TCC771L       771
#define TCC773L       773
#define TCC7801      7801

/* CONFIG_KEYPAD */
#define PLAYER_PAD          1
#define RECORDER_PAD        2
#define ONDIO_PAD           3
#define IRIVER_H100_PAD     4
#define IRIVER_H300_PAD     5
#define IAUDIO_X5M5_PAD     6
#define IPOD_4G_PAD         7
#define IPOD_3G_PAD         8
#define IPOD_1G2G_PAD       9
#define IRIVER_IFP7XX_PAD  10
#define GIGABEAT_PAD       11
#define IRIVER_H10_PAD     12
#define SANSA_E200_PAD     13
#define SANSA_C200_PAD     14
#define ELIO_TPJ1022_PAD   15
#define ARCHOS_AV300_PAD   16
#define MROBE100_PAD       17
#define MROBE500_PAD       18
#define GIGABEAT_S_PAD     19
#define LOGIK_DAX_PAD      20
#define IAUDIO67_PAD       21
#define COWOND2_PAD        22
#define IAUDIO_M3_PAD      23
#define CREATIVEZVM_PAD    24
#define SANSA_M200_PAD     25

/* CONFIG_REMOTE_KEYPAD */
#define H100_REMOTE 1
#define H300_REMOTE 2
#define X5_REMOTE   3

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
#define LCD_C200     17 /* as used by Sandisk Sansa c200 */
#define LCD_MROBE500 18 /* as used by Olympus M:Robe 500i */
#define LCD_MROBE100 19 /* as used by Olympus M:Robe 100 */
#define LCD_LOGIKDAX 20 /* as used by Logik DAX - SSD1815 */
#define LCD_IAUDIO67 21 /* as used by iAudio 6/7 - unknown */
#define LCD_CREATIVEZVM 22 /* as used by Creative Zen Vision:M */
#define LCD_TL0350A  23 /* as used by the iAudio M3 remote, treated as main LCD */
#define LCD_COWOND2  24 /* as used by Cowon D2 - LTV250QV, TCC7801 driver */

/* LCD_PIXELFORMAT */
#define HORIZONTAL_PACKING 1
#define VERTICAL_PACKING 2
#define HORIZONTAL_INTERLEAVED 3
#define VERTICAL_INTERLEAVED 4
#define RGB565 565
#define RGB565SWAPPED 3553

/* CONFIG_ORIENTATION */
#define SCREEN_PORTRAIT     0
#define SCREEN_LANDSCAPE    1
#define SCREEN_SQUARE       2

/* CONFIG_I2C */
#define I2C_PLAYREC  1 /* Archos Player/Recorder style */
#define I2C_ONDIO    2 /* Ondio style */
#define I2C_COLDFIRE 3 /* Coldfire style */
#define I2C_PP5002   4 /* PP5002 style */
#define I2C_PP5020   5 /* PP5020 style */
#define I2C_PNX0101  6 /* PNX0101 style */
#define I2C_S3C2440  7
#define I2C_PP5024   8 /* PP5024 style */
#define I2C_IMX31L   9
#define I2C_TCC77X  10
#define I2C_TCC780X 11
#define I2C_DM320   12 /* DM320 style */

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
#define RTC_IMX31L   8
#define RTC_RX5X348AB 9
#define RTC_TCC77X   10
#define RTC_TCC780X  11
#define RTC_MR100  12
#define RTC_MC13783  13 /* Freescale MC13783 PMIC */

/* USB On-the-go */
#define USBOTG_ISP1362 1362 /* iriver H300 */
#define USBOTG_ISP1583 1583 /* Creative Zen Vision:M */
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
#elif defined(IAUDIO_M3)
#include "config-iaudiom3.h"
#elif defined(IPOD_COLOR)
#include "config-ipodcolor.h"
#elif defined(IPOD_NANO)
#include "config-ipodnano.h"
#elif defined(IPOD_VIDEO)
#include "config-ipodvideo.h"
#elif defined(IPOD_1G2G)
#include "config-ipod1g2g.h"
#elif defined(IPOD_3G)
#include "config-ipod3g.h"
#elif defined(IPOD_4G)
#include "config-ipod4g.h"
#elif defined(IRIVER_IFP7XX)
#include "config-ifp7xx.h"
#elif defined(GIGABEAT_F)
#include "config-gigabeat.h"
#elif defined(GIGABEAT_S)
#include "config-gigabeat-s.h"
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
#elif defined(SANSA_C200)
#include "config-c200.h"
#elif defined(SANSA_M200)
#include "config-m200.h"
#elif defined(ELIO_TPJ1022)
#include "config-tpj1022.h"
#elif defined(MROBE_100)
#include "config-mrobe100.h"
#elif defined(MROBE_500)
#include "config-mrobe500.h"
#elif defined(LOGIK_DAX)
#include "config-logikdax.h"
#elif defined(IAUDIO_7)
#include "config-iaudio7.h"
#elif defined(COWON_D2)
#include "config-cowond2.h"
#elif defined(CREATIVE_ZVM)
#include "config-creativezvm.h"
#elif defined(CREATIVE_ZVM60GB)
#include "config-creativezvm60gb.h"
#elif defined(CREATIVE_ZV)
#include "config-creativezv.h"
#else
/* no known platform */
#endif

/* setup basic macros from capability masks */
#include "config_caps.h"

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

#ifndef CONFIG_CHARGING
#define CONFIG_CHARGING 0
#endif

#ifndef CONFIG_RTC
#define CONFIG_RTC 0
#endif

#ifndef CONFIG_ORIENTATION
#if LCD_HEIGHT > LCD_WIDTH
#define CONFIG_ORIENTATION SCREEN_PORTRAIT
#elif LCD_HEIGHT < LCD_WIDTH
#define CONFIG_ORIENTATION SCREEN_LANDSCAPE
#else
#define CONFIG_ORIENTATION SCREEN_SQUARE
#endif
#endif

/* define this in the target config.h to use a different size */
#ifndef CONFIG_DEFAULT_ICON_HEIGHT
#define CONFIG_DEFAULT_ICON_HEIGHT 8
#endif
#ifndef CONFIG_DEFAULT_ICON_WIDTH
#define CONFIG_DEFAULT_ICON_WIDTH 6
#endif
#ifndef CONFIG_REMOTE_DEFAULT_ICON_HEIGHT
#define CONFIG_REMOTE_DEFAULT_ICON_HEIGHT 8
#endif
#ifndef CONFIG_REMOTE_DEFAULT_ICON_WIDTH
#define CONFIG_REMOTE_DEFAULT_ICON_WIDTH 6
#endif

#if (CONFIG_TUNER & (CONFIG_TUNER - 1)) != 0
/* Multiple possible tuners */
#define CONFIG_TUNER_MULTI
#endif

#if defined(BOOTLOADER) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
/* Bootloaders don't use CPU frequency adjustment */
#undef HAVE_ADJUSTABLE_CPU_FREQ
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

/* Add one HAVE_ define for all mas35xx targets */
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3507D) || (CONFIG_CODEC == MAS3539F)
#define HAVE_MAS35XX
#endif

#if (CONFIG_CODEC == SWCODEC)
#ifndef BOOTLOADER

#define HAVE_EXTENDED_MESSAGING_AND_NAME

#ifndef SIMULATOR
#define HAVE_PRIORITY_SCHEDULING
#define HAVE_SCHEDULER_BOOSTCTRL
#endif /* SIMULATOR */

#define HAVE_SEMAPHORE_OBJECTS
#define HAVE_EVENT_OBJECTS

#ifdef TOSHIBA_GIGABEAT_F
#define HAVE_WAKEUP_OBJECTS
#endif

#endif /* BOOTLOADER */

#ifdef HAVE_USBSTACK
#define HAVE_WAKEUP_OBJECTS
#endif

#endif /*  (CONFIG_CODEC == SWCODEC) */

/* define for all cpus from SH family */
#if (CONFIG_CPU == SH7034)
#define CPU_SH
#endif

/* define for all cpus from coldfire family */
#if (CONFIG_CPU == MCF5249) || (CONFIG_CPU == MCF5250)
#define CPU_COLDFIRE
#endif

/* define for all cpus from PP family */
#if (CONFIG_CPU == PP5002)
#define CPU_PP
#elif (CONFIG_CPU == PP5020) || (CONFIG_CPU == PP5022)  || (CONFIG_CPU == PP5024)
#define CPU_PP
#define CPU_PP502x
#endif

/* define for all cpus from TCC77X family */
#if (CONFIG_CPU == TCC771L) || (CONFIG_CPU == TCC773L)
#define CPU_TCC77X
#endif

/* define for all cpus from TCC780 family */
#if (CONFIG_CPU == TCC7801)
#define CPU_TCC780X
#endif

/* define for all cpus from ARM7TDMI family (for specific optimisations) */
#if defined(CPU_PP) || (CONFIG_CPU == PNX0101) || (CONFIG_CPU == DSC25)
#define CPU_ARM7TDMI
#endif

/* define for all cpus from ARM family */
#if (CONFIG_CPU == IMX31L)
#define CPU_ARM
#define ARM_ARCH 6 /* ARMv6 */
#endif

#if defined(CPU_TCC77X) || defined(CPU_TCC780X) || (CONFIG_CPU == DM320)
#define CPU_ARM
#define ARM_ARCH 5 /* ARMv5 */
#endif

#if defined(CPU_PP) || (CONFIG_CPU == PNX0101) || (CONFIG_CPU == S3C2440) \
  || (CONFIG_CPU == DSC25)
#define CPU_ARM
#define ARM_ARCH 4 /* ARMv4 */
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
    defined(CPU_TCC77X) ||  /* Telechips: core, plugins, codecs */ \
    defined(CPU_TCC780X) || /* Telechips: core, plugins, codecs */ \
    (CONFIG_CPU == PNX0101))
#define ICODE_ATTR      __attribute__ ((section(".icode")))
#define ICONST_ATTR     __attribute__ ((section(".irodata")))
#define IDATA_ATTR      __attribute__ ((section(".idata")))
#define IBSS_ATTR       __attribute__ ((section(".ibss")))
#define USE_IRAM
#if CONFIG_CPU != SH7034
#define PLUGIN_USE_IRAM
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

/* Change this if you want to build a single-core firmware for a multicore
 * target for debugging */
#if defined(BOOTLOADER)
#define FORCE_SINGLE_CORE
#endif

/* Core locking types - specifies type of atomic operation */
#define CORELOCK_NONE   0
#define SW_CORELOCK     1 /* Mutual exclusion provided by a software algorithm
                             and not a special semaphore instruction */
#define CORELOCK_SWAP   2 /* A swap (exchange) instruction */

#if defined(CPU_PP)
#define IDLE_STACK_SIZE  0x80
#define IDLE_STACK_WORDS 0x20

/* Attributes to place data in uncached DRAM */
/* These are useful beyond dual-core and ultimately beyond PP since they may
 * be used for DMA buffers and such without cache maintenence calls. */
#define NOCACHEBSS_ATTR     __attribute__((section(".ncbss"),nocommon))
#define NOCACHEDATA_ATTR    __attribute__((section(".ncdata"),nocommon))

#if !defined(FORCE_SINGLE_CORE)

#define NUM_CORES 2
#define CURRENT_CORE current_core()
/* Attributes for core-shared data in DRAM where IRAM is better used for other
 * purposes. */
#define SHAREDBSS_ATTR     NOCACHEBSS_ATTR
#define SHAREDDATA_ATTR    NOCACHEDATA_ATTR

#define IF_COP(...)         __VA_ARGS__
#define IF_COP_VOID(...)    __VA_ARGS__
#define IF_COP_CORE(core)   core

#ifdef CPU_PP
#define CONFIG_CORELOCK SW_CORELOCK /* SWP(B) is broken */
#else
#define CONFIG_CORELOCK CORELOCK_SWAP
#endif

#endif /* !defined(BOOTLOADER) && CONFIG_CPU != PP5002 */

#endif /* CPU_PP */

#ifndef CONFIG_CORELOCK
#define CONFIG_CORELOCK CORELOCK_NONE
#endif

#if CONFIG_CORELOCK == SW_CORELOCK
#define IF_SWCL(...) __VA_ARGS__
#define IFN_SWCL(...)
#else
#define IF_SWCL(...)
#define IFN_SWCL(...) __VA_ARGS__
#endif /* CONFIG_CORELOCK == */

#ifndef NUM_CORES
/* Default to single core */
#define NUM_CORES 1
#define CURRENT_CORE    CPU
/* Attributes for core-shared data in DRAM - no caching considerations */
#define SHAREDBSS_ATTR
#define SHAREDDATA_ATTR
#ifndef NOCACHEBSS_ATTR
#define NOCACHEBSS_ATTR
#define NOCACHEDATA_ATTR
#endif
#define CONFIG_CORELOCK CORELOCK_NONE

#define IF_COP(...)
#define IF_COP_VOID(...)    void
#define IF_COP_CORE(core)   CURRENT_CORE

#endif /* NUM_CORES */

#ifdef HAVE_HEADPHONE_DETECTION
/* Timeout objects required if headphone detection is enabled */
#ifndef INCLUDE_TIMEOUT_API
#define INCLUDE_TIMEOUT_API
#endif
#endif /* HAVE_HEADPHONE_DETECTION */

#endif /* __CONFIG_H__ */
