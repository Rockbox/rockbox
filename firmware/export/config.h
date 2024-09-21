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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "autoconf.h"

/* symbolic names for multiple choice configurations: */

/* CONFIG_STORAGE (note these are combineable bit-flags) */
#define STORAGE_ATA_NUM     0
#define STORAGE_MMC_NUM     1
#define STORAGE_SD_NUM      2
#define STORAGE_NAND_NUM    3
#define STORAGE_RAMDISK_NUM 4
#define STORAGE_USB_NUM     5
#define STORAGE_HOSTFS_NUM  6
#define STORAGE_NUM_TYPES   7

#define STORAGE_ATA         (1 << STORAGE_ATA_NUM)
#define STORAGE_MMC         (1 << STORAGE_MMC_NUM)
#define STORAGE_SD          (1 << STORAGE_SD_NUM)
#define STORAGE_NAND        (1 << STORAGE_NAND_NUM)
#define STORAGE_RAMDISK     (1 << STORAGE_RAMDISK_NUM)
#define STORAGE_USB         (1 << STORAGE_USB_NUM)
 /* meant for APPLICATION targets (implicit for SIMULATOR) */
#define STORAGE_HOSTFS      (1 << STORAGE_HOSTFS_NUM)

/* CONFIG_TUNER (note these are combineable bit-flags) */
#define TEA5767    0x02 /* Philips */
#define LV24020LP  0x04 /* Sanyo */
#define SI4700     0x08 /* Silicon Labs */
#define TEA5760    0x10 /* Philips */
#define LV240000   0x20 /* Sanyo */
#define IPOD_REMOTE_TUNER   0x40 /* Apple */
#define RDA5802    0x80 /* RDA Microelectronics */
#define STFM1000   0x100 /* Sigmatel */

/* CONFIG_CPU */
#define MCF5249      5249
#define MCF5250      5250
#define PP5002       5002
#define PP5020       5020
#define PP5022       5022
#define PP5024       5024
#define PP6100       6100
#define PNX0101       101
#define S3C2440      2440
#define DSC25          25
#define DM320         320
#define IMX31L         31
#define TCC7801      7801
#define S5L8700      8700
#define S5L8701      8701
#define S5L8702      8702
#define JZ4732       4732
#define JZ4760B     47602
#define AS3525       3525
#define AT91SAM9260  9260
#define AS3525v2    35252
#define IMX233        233
#define RK27XX       2700
#define X1000        1000

/* platforms
 * bit fields to allow PLATFORM_HOSTED to be OR'ed e.g. with a
 * possible future PLATFORM_ANDROID (some OSes might need totally different
 * handling to run on them than a stand-alone application) */
#define PLATFORM_NATIVE  (1<<0)
#define PLATFORM_HOSTED  (1<<1)
#define PLATFORM_ANDROID (1<<2)
#define PLATFORM_SDL     (1<<3)
#define PLATFORM_MAEMO4  (1<<4)
#define PLATFORM_MAEMO5  (1<<5)
#define PLATFORM_MAEMO   (PLATFORM_MAEMO4|PLATFORM_MAEMO5)
#define PLATFORM_PANDORA (1<<6)

/* CONFIG_KEYPAD */
#define IRIVER_H100_PAD     4
#define IRIVER_H300_PAD     5
#define IAUDIO_X5M5_PAD     6
#define IPOD_4G_PAD         7
#define IPOD_3G_PAD         8
#define IPOD_1G2G_PAD       9
#define GIGABEAT_PAD       11
#define IRIVER_H10_PAD     12
#define SANSA_E200_PAD     13
#define SANSA_C200_PAD     14
#define MROBE100_PAD       17
#define MROBE500_PAD       18
#define GIGABEAT_S_PAD     19
#define COWON_D2_PAD        22
#define IAUDIO_M3_PAD      23
#define CREATIVEZVM_PAD    24
#define SANSA_M200_PAD     25
#define CREATIVEZV_PAD     26
#define PHILIPS_SA9200_PAD 27
#define PHILIPS_HDD1630_PAD 29
#define MEIZU_M6SL_PAD     30
#define ONDAVX747_PAD      31
#define ONDAVX767_PAD      32
#define MEIZU_M6SP_PAD     33
#define MEIZU_M3_PAD       34
#define SANSA_CLIP_PAD     35
#define SANSA_FUZE_PAD     36
#define LYRE_PROTO1_PAD    37
#define SAMSUNG_YH820_PAD  38
#define ONDAVX777_PAD      39
#define SAMSUNG_YPS3_PAD   40
#define MINI2440_PAD       41
#define PHILIPS_HDD6330_PAD 42
#define PBELL_VIBE500_PAD 43
#define MPIO_HD200_PAD     44
#define ANDROID_PAD        45
#define SDL_PAD            46
#define MPIO_HD300_PAD     47
#define SANSA_FUZEPLUS_PAD 48
#define RK27XX_GENERIC_PAD 49
#define HM60X_PAD          50
#define HM801_PAD          51
#define SANSA_CONNECT_PAD  52
#define SAMSUNG_YPR0_PAD   53
#define CREATIVE_ZENXFI2_PAD 54
#define CREATIVE_ZENXFI3_PAD 55
#define MA_PAD            56
#define SONY_NWZ_PAD       57
#define CREATIVE_ZEN_PAD   58
#define IHIFI_PAD          60
#define SAMSUNG_YPR1_PAD   61
#define SAMSUNG_YH92X_PAD  62
#define DX50_PAD           63
#define SONY_NWZA860_PAD   64 /* The NWZ-A860 is too different (touchscreen) */
#define AGPTEK_ROCKER_PAD  65
#define XDUOO_X3_PAD       66
#define IHIFI_770_PAD      67
#define IHIFI_800_PAD      68
#define XDUOO_X3II_PAD     69
#define XDUOO_X20_PAD      70
#define FIIO_M3K_LINUX_PAD 71
#define EROSQ_PAD          72
#define FIIO_M3K_PAD       73
#define SHANLING_Q1_PAD    74

/* CONFIG_REMOTE_KEYPAD */
#define H100_REMOTE   1
#define H300_REMOTE   2
#define IAUDIO_REMOTE 3
#define MROBE_REMOTE  4

/* CONFIG_BACKLIGHT_FADING */
/* No fading capabilities at all (yet) */
#define BACKLIGHT_NO_FADING         0x0
/* Backlight fading is controlled using a hardware PWM mechanism */
#define BACKLIGHT_FADING_PWM        0x1
/* Backlight is controlled using a software implementation
 * BACKLIGHT_FADING_SW_SETTING means that backlight is turned on by only setting
 * the brightness (i.e. no real difference between backlight_on and
 * backlight_set_brightness)
 * BACKLIGHT_FADING_SW_HW_REG means that backlight brightness is restored
 * "in hardware", from a hardware register upon backlight_on
 * Both types need to have minor adjustments in the software fading code */
#define BACKLIGHT_FADING_SW_SETTING 0x2
#define BACKLIGHT_FADING_SW_HW_REG  0x4
/* Backlight fading is done in a target specific way
 * for example in hardware, but not controllable*/
#define BACKLIGHT_FADING_TARGET     0x8

/* CONFIG_CHARGING */

/* Generic types */
#define CHARGING_SIMPLE  1 /* Simple, hardware controlled charging
                            * (CPU cannot read charger state but may read
                            *  when power is plugged-in). */
#define CHARGING_MONITOR 2 /* Hardware controlled charging with monitoring
                            * (CPU is able to read HW charging state and
                            *  when power is plugged-in). */

/* Mostly target-specific code in the /target tree */
#define CHARGING_TARGET  3 /* Any algorithm - usually software controlled
                            * charging or specific programming is required to
                            * use the charging hardware. */

/* CONFIG_BATTERY_MEASURE bits */
#define VOLTAGE_MEASURE     1 /* Target can report battery voltage
                               * Usually native ports */
#define PERCENTAGE_MEASURE  2 /* Target can report remaining capacity in %
                               * Usually application/hosted ports */
#define TIME_MEASURE        4 /* Target can report remaining time estimation
                                 Usually application ports, and only
                                 if the estimation is better that ours
                                 (which it probably is) */
#define CURRENT_MEASURE     8 /* Target can report battery charge and/or
                               * discharge current */
/* CONFIG_LCD */
#define LCD_SSD1815   1 /* as used by Sansa M200 and others */
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
#define LCD_GIGABEAT 12
#define LCD_H10_20GB 13 /* as used by iriver H10 20Gb */
#define LCD_H10_5GB  14 /* as used by iriver H10 5Gb */
#define LCD_C200     17 /* as used by Sandisk Sansa c200 */
#define LCD_MROBE500 18 /* as used by Olympus M:Robe 500i */
#define LCD_MROBE100 19 /* as used by Olympus M:Robe 100 */
#define LCD_CREATIVEZVM 22 /* as used by Creative Zen Vision:M */
#define LCD_TL0350A  23 /* as used by the iAudio M3 remote, treated as main LCD */
#define LCD_COWOND2  24 /* as used by Cowon D2 - LTV250QV, TCC7801 driver */
#define LCD_SA9200   25 /* as used by the Philips SA9200 */
#define LCD_S6B33B2  26 /* as used by the Samsumg YH820 */
#define LCD_HDD1630  27 /* as used by the Philips HDD1630 */
#define LCD_MEIZUM6  28 /* as used by the Meizu M6SP and M6SL (various models) */
#define LCD_ONDAVX747 29 /* as used by the Onda VX747 */
#define LCD_ONDAVX767 30 /* as used by the Onda VX767 */
#define LCD_SSD1303   31 /* as used by the Sansa Clip */
#define LCD_FUZE      32 /* as used by the Sansa Fuze */
#define LCD_LYRE_PROTO1      33 /* as used by the Lyre prototype 1 */
#define LCD_YH925     34 /* as used by Samsung YH-925 (similar to the H10 20GB) */
#define LCD_VIEW      35 /* as used by the Sansa View */
#define LCD_NANO2G    36 /* as used by the iPod Nano 2nd Generation */
#define LCD_MINI2440  37 /* as used by the Mini2440 */
#define LCD_HDD6330   38 /* as used by the Philips HDD6330 */
#define LCD_VIBE500   39 /* as used by the Packard Bell Vibe 500 */
#define LCD_IPOD6G    40 /* as used by the iPod Nano 2nd Generation */
#define LCD_FUZEPLUS  41
#define LCD_SPFD5420A 42 /* rk27xx */
#define LCD_CLIPZIP   43 /* as used by the Sandisk Sansa Clip Zip */
#define LCD_HX8340B   44 /* as used by the HiFiMAN HM-601/HM-602/HM-801 */
#define LCD_CONNECT   45 /* as used by the Sandisk Sansa Connect */
#define LCD_GIGABEATS 46
#define LCD_YPR0      47
#define LCD_CREATIVEZXFI2 48 /* as used by the Creative Zen X-Fi2 */
#define LCD_CREATIVEZXFI3 49 /* as used by the Creative Zen X-Fi3 */
#define LCD_ILI9342   50 /* as used by HiFi E.T MA9/MA8 */
#define LCD_NWZE370   51 /* as used by Sony NWZ-E370 series */
#define LCD_NWZE360   52 /* as used by Sony NWZ-E360 series */
#define LCD_CREATIVEZEN  55 /* as used by the Creative ZEN (X-Fi) (LMS250GF03-001(S6D0139)) */
#define LCD_CREATIVEZENMOZAIC 56 /* as used by the Creative ZEN Mozaic (FGD0801) */
#define LCD_ILI9342C   57 /* another type of lcd used by HiFi E.T MA9/MA8 */
#define LCD_CREATIVEZENV  58 /* as used by the Creative Zen V (Plus) */
#define LCD_IHIFI         60 /* as used by IHIFI 760/960 */
#define LCD_CREATIVEZENXFISTYLE 61 /* as used by Creative Zen X-Fi Style */
#define LCD_SAMSUNGYPR1   62 /* as used by Samsung YP-R1 */
#define LCD_NWZ_LINUX   63 /* as used in the Linux-based NWZ series */
#define LCD_INGENIC_LINUX 64
#define LCD_XDUOOX3       65 /* as used by the xDuoo X3 */
#define LCD_IHIFI770      66 /* as used by IHIFI 770 */
#define LCD_IHIFI770C     67 /* as used by IHIFI 770C */
#define LCD_IHIFI800      68 /* as used by IHIFI 800 */
#define LCD_FIIOM3K       69 /* as used by the FiiO M3K */
#define LCD_SHANLING_Q1   70 /* as used by the Shanling Q1 */
#define LCD_EROSQ         71 /* as used by the ErosQ (native) */

/* LCD_PIXELFORMAT */
#define HORIZONTAL_PACKING 1
#define VERTICAL_PACKING 2
#define HORIZONTAL_INTERLEAVED 3
#define VERTICAL_INTERLEAVED 4
#define RGB565 565
#define RGB565SWAPPED 3553
#define RGB888 888
#define XRGB8888 8888

/* LCD_STRIDEFORMAT */
#define VERTICAL_STRIDE     1
#define HORIZONTAL_STRIDE   2

/* CONFIG_ORIENTATION */
#define SCREEN_PORTRAIT     0
#define SCREEN_LANDSCAPE    1
#define SCREEN_SQUARE       2

/* CONFIG_I2C */
#define I2C_NONE     0 /* For targets that do not use I2C - as the
Lyre prototype 1 */
#define I2C_COLDFIRE 3 /* Coldfire style */
#define I2C_PP5002   4 /* PP5002 style */
#define I2C_PP5020   5 /* PP5020 style */
#define I2C_PNX0101  6 /* PNX0101 style */
#define I2C_S3C2440  7
#define I2C_PP5024   8 /* PP5024 style */
#define I2C_IMX31L   9
#define I2C_TCC780X 11
#define I2C_DM320   12 /* DM320 style */
#define I2C_S5L8700 13
#define I2C_JZ47XX  14 /* Ingenic Jz47XX style */
#define I2C_AS3525  15
#define I2C_S5L8702 16 /* Same as S5L8700, but with two channels */
#define I2C_IMX233  17
#define I2C_RK27XX  18
#define I2C_X1000   19

/* CONFIG_LED */
#define LED_REAL     1 /* SW controlled LED (Archos recorders, player) */
#define LED_VIRTUAL  2 /* Virtual LED (icon) (Archos Ondio) */
/* else                   HW controlled LED (iRiver H1x0) */

/* CONFIG_NAND */
#define NAND_TCC     2
#define NAND_SAMSUNG 3
#define NAND_CC      4 /* ChinaChip */
#define NAND_RK27XX  5
#define NAND_IMX233  6

/* CONFIG_RTC */
#define RTC_PCF50605 2 /* iPod 3G, 4G & Mini */
#define RTC_PCF50606 3 /* iriver H300 */
#define RTC_S3C2440  4
#define RTC_E8564    5 /* iriver H10 */
#define RTC_AS3514   6 /* Sandisk Sansa series */
#define RTC_DS1339_DS3231   7 /* h1x0 RTC mod */
#define RTC_IMX31L   8
#define RTC_RX5X348AB 9
#define RTC_TCC780X  11
#define RTC_MR100  12
#define RTC_MC13783  13 /* Freescale MC13783 PMIC */
#define RTC_S5L8700  14
#define RTC_S35390A  15
#define RTC_JZ4740   16 /* Ingenic Jz4740 */
#define RTC_NANO2G   17 /* This seems to be a PCF5063x */
#define RTC_D2       18 /* Either PCF50606 or PCF50635 */
#define RTC_S35380A  19
#define RTC_IMX233   20
#define RTC_STM41T62 21 /* ST M41T62 */
#define RTC_JZ4760   22 /* Ingenic Jz4760 */
#define RTC_X1000    23 /* Ingenic X1000 */
#define RTC_CONNECT  24 /* Sansa Connect AVR */

/* USB On-the-go */
#define USBOTG_M66591   6591 /* M:Robe 500 */
#define USBOTG_ISP1362  1362 /* iriver H300 */
#define USBOTG_ISP1583  1583 /* Creative Zen Vision:M */
#define USBOTG_M5636    5636 /* iAudio X5 */
#define USBOTG_ARC      5020 /* PortalPlayer 502x and IMX233 */
#define USBOTG_JZ4740   4740 /* Ingenic Jz4740/Jz4732 */
#define USBOTG_JZ4760   4760 /* Ingenic Jz4760/Jz4760B */
#define USBOTG_AS3525   3525 /* AMS AS3525 */
#define USBOTG_S3C6400X 6400 /* Samsung S3C6400X, also used in the S5L8701/S5L8702/S5L8720 */
#define USBOTG_DESIGNWARE 6401 /* Synopsys DesignWare OTG, used in S5L8701/S5L8702/S5L8720/AS3252v2 */
#define USBOTG_RK27XX   2700 /* Rockchip rk27xx */
#define USBOTG_TNETV105 105  /* TI TNETV105 */

/* Multiple cores */
#define CPU 0
#define COP 1

/* imx233 specific: IMX233_PACKAGE */
#define IMX233_BGA100   0
#define IMX233_BGA169   1
#define IMX233_TQFP100  2
#define IMX233_TQFP128  3
#define IMX233_LQFP100  4

/* IMX233_PARTITIONS */
#define IMX233_FREESCALE    (1 << 0) /* Freescale I.MX233 nonstandard two-level MBR */
#define IMX233_CREATIVE     (1 << 1) /* Creative MBLK windowing */

/* CONFIG_BUFLIB_BACKEND */
#define BUFLIB_BACKEND_MEMPOOL      0 /* Default memory pool backed buflib */
#define BUFLIB_BACKEND_MALLOC       1 /* malloc() buflib (for debugging) */

/* now go and pick yours */
#if defined(IRIVER_H100)
#include "config/iriverh100.h"
#elif defined(IRIVER_H120)
#include "config/iriverh120.h"
#elif defined(IRIVER_H300)
#include "config/iriverh300.h"
#elif defined(IAUDIO_X5)
#include "config/iaudiox5.h"
#elif defined(IAUDIO_M5)
#include "config/iaudiom5.h"
#elif defined(IAUDIO_M3)
#include "config/iaudiom3.h"
#elif defined(IPOD_COLOR)
#include "config/ipodcolor.h"
#elif defined(IPOD_NANO)
#include "config/ipodnano1g.h"
#elif defined(IPOD_VIDEO)
#include "config/ipodvideo.h"
#elif defined(IPOD_1G2G)
#include "config/ipod1g2g.h"
#elif defined(IPOD_3G)
#include "config/ipod3g.h"
#elif defined(IPOD_4G)
#include "config/ipod4g.h"
#elif defined(IPOD_NANO2G)
#include "config/ipodnano2g.h"
#elif defined(IPOD_6G)
#include "config/ipod6g.h"
#elif defined(GIGABEAT_F)
#include "config/gigabeatfx.h"
#elif defined(GIGABEAT_S)
#include "config/gigabeats.h"
#elif defined(IPOD_MINI)
#include "config/ipodmini1g.h"
#elif defined(IPOD_MINI2G)
#include "config/ipodmini2g.h"
#elif defined(IRIVER_H10)
#include "config/iriverh10.h"
#elif defined(IRIVER_H10_5GB)
#include "config/iriverh10_5gb.h"
#elif defined(SANSA_E200)
#include "config/sansae200.h"
#elif defined(SANSA_C200)
#include "config/sansac200.h"
#elif defined(MROBE_100)
#include "config/mrobe100.h"
#elif defined(MROBE_500)
#include "config/mrobe500.h"
#elif defined(COWON_D2)
#include "config/cowond2.h"
#elif defined(CREATIVE_ZVM)
#include "config/zenvisionm30gb.h"
#elif defined(CREATIVE_ZVM60GB)
#include "config/zenvisionm60gb.h"
#elif defined(CREATIVE_ZV)
#include "config/zenvision.h"
#elif defined(CREATIVE_ZENXFI2)
#include "config/creativezenxfi2.h"
#elif defined(CREATIVE_ZENXFI3)
#include "config/creativezenxfi3.h"
#elif defined(PHILIPS_SA9200)
#include "config/gogearsa9200.h"
#elif defined(PHILIPS_HDD1630)
#include "config/gogearhdd1630.h"
#elif defined(PHILIPS_HDD6330)
#include "config/gogearhdd6330.h"
#elif defined(MEIZU_M6SL)
#include "config/meizum6sl.h"
#elif defined(MEIZU_M6SP)
#include "config/meizum6sp.h"
#elif defined(MEIZU_M3)
#include "config/meizum3.h"
#elif defined(ONDA_VX747) || defined(ONDA_VX747P)
#include "config/ondavx747.h"
#elif defined(ONDA_VX777)
#include "config/ondavx777.h"
#elif defined(ONDA_VX767)
#include "config/ondavx767.h"
#elif defined(SANSA_CLIP)
#include "config/sansaclip.h"
#elif defined(SANSA_CLIPV2)
#include "config/sansaclipv2.h"
#elif defined(SANSA_CLIPPLUS)
#include "config/sansaclipplus.h"
#elif defined(SANSA_E200V2)
#include "config/sansae200v2.h"
#elif defined(SANSA_M200V4)
#include "config/sansam200v4.h"
#elif defined(SANSA_FUZE)
#include "config/sansafuze.h"
#elif defined(SANSA_FUZEV2)
#include "config/sansafuzev2.h"
#elif defined(SANSA_FUZEPLUS)
#include "config/sansafuzeplus.h"
#elif defined(SANSA_CLIPZIP)
#include "config/sansaclipzip.h"
#elif defined(SANSA_C200V2)
#include "config/sansac200v2.h"
#elif defined(SANSA_VIEW)
#include "config/sansaview.h"
#elif defined(LYRE_PROTO1)
#include "config/lyreproto1.h"
#elif defined(MINI2440)
#include "config/mini2440.h"
#elif defined(SAMSUNG_YH820)
#include "config/samsungyh820.h"
#elif defined(SAMSUNG_YH920)
#include "config/samsungyh920.h"
#elif defined(SAMSUNG_YH925)
#include "config/samsungyh925.h"
#elif defined(SAMSUNG_YPS3)
#include "config/samsungyps3.h"
#elif defined(PBELL_VIBE500)
#include "config/vibe500.h"
#elif defined(MPIO_HD200)
#include "config/mpiohd200.h"
#elif defined(MPIO_HD300)
#include "config/mpiohd300.h"
#elif defined(RK27_GENERIC)
#include "config/rk27generic.h"
#elif defined(HM60X)
#include "config/hifimanhm60x.h"
#elif defined(HM801)
#include "config/hifimanhm801.h"
#elif defined(SANSA_CONNECT)
#include "config/sansaconnect.h"
#elif defined(SDLAPP)
#include "config/sdlapp.h"
#elif defined(ANDROID)
#include "config/android.h"
#elif defined(NOKIAN8XX)
#include "config/nokian8xx.h"
#elif defined(NOKIAN900)
#include "config/nokian900.h"
#elif defined(PANDORA)
#include "config/pandora.h"
#elif defined(SAMSUNG_YPR0)
#include "config/samsungypr0.h"
#elif defined(CREATIVE_ZENXFI)
#include "config/creativezenxfi.h"
#elif defined(CREATIVE_ZENMOZAIC)
#include "config/creativezenmozaic.h"
#elif defined(CREATIVE_ZEN)
#include "config/creativezen.h"
#elif defined(CREATIVE_ZENV)
#include "config/creativezenv.h"
#elif defined(MA9)
#include "config/hifietma9.h"
#elif defined(MA9C)
#include "config/hifietma9c.h"
#elif defined(MA8)
#include "config/hifietma8.h"
#elif defined(MA8C)
#include "config/hifietma8c.h"
#elif defined(SONY_NWZE370)
#include "config/sonynwze370.h"
#elif defined(SONY_NWZE360)
#include "config/sonynwze360.h"
#elif defined(IHIFI760)
#include "config/ihifi760.h"
#elif defined(IHIFI770)
#include "config/ihifi770.h"
#elif defined(IHIFI770C)
#include "config/ihifi770c.h"
#elif defined(IHIFI800)
#include "config/ihifi800.h"
#elif defined(IHIFI960)
#include "config/ihifi960.h"
#elif defined(CREATIVE_ZENXFISTYLE)
#include "config/creativezenxfistyle.h"
#elif defined(SAMSUNG_YPR1)
#include "config/samsungypr1.h"
#elif defined(DX50)
#include "config/ibassodx50.h"
#elif defined(DX90)
#include "config/ibassodx90.h"
#elif defined(SONY_NWZE460)
#include "config/sonynwze460.h"
#elif defined(SONY_NWZE450)
#include "config/sonynwze450.h"
#elif defined(SONY_NWZE580)
#include "config/sonynwze580.h"
#elif defined(SONY_NWZA10)
#include "config/sonynwza10.h"
#elif defined(SONY_NWA20)
#include "config/sonynwa20.h"
#elif defined(SONY_NWZE470)
#include "config/sonynwze470.h"
#elif defined(SONY_NWZA860)
#include "config/sonynwza860.h"
#elif defined(SONY_NWZS750)
#include "config/sonynwzs750.h"
#elif defined(SONY_NWZE350)
#include "config/sonynwze350.h"
#elif defined(AGPTEK_ROCKER)
#include "config/agptekrocker.h"
#elif defined(XDUOO_X3)
#include "config/xduoox3.h"
#elif defined(XDUOO_X3II)
#include "config/xduoox3ii.h"
#elif defined(XDUOO_X20)
#include "config/xduoox20.h"
#elif defined(FIIO_M3K_LINUX)
#include "config/fiiom3klinux.h"
#elif defined(FIIO_M3K)
#include "config/fiiom3k.h"
#elif defined(EROS_Q)
#include "config/aigoerosq.h"
#elif defined(SHANLING_Q1)
#include "config/shanlingq1.h"
#elif defined(EROS_QN)
#include "config/erosqnative.h"
#else
//#error "unknown hwardware platform!"
#endif

#ifndef CONFIG_CPU
#define CONFIG_CPU 0
#endif

// NOTE: should be placed before sim.h (where CONFIG_CPU is undefined)
#if !(CONFIG_CPU >= PP5002 && CONFIG_CPU <= PP5022) && CODEC_SIZE >= 0x80000
#define CODEC_AAC_SBR_DEC
#endif

#ifdef __PCTOOL__
#undef CONFIG_CPU
#define CONFIG_CPU 0
#undef HAVE_MULTIVOLUME
#undef HAVE_MULTIDRIVE
#undef CONFIG_STORAGE_MULTI
#undef CONFIG_STORAGE
#endif

#ifndef CONFIG_BUFLIB_BACKEND
# define CONFIG_BUFLIB_BACKEND BUFLIB_BACKEND_MEMPOOL
#endif

#ifdef APPLICATION
#define CONFIG_CPU 0
#endif

/* keep this include after the target configs */
#ifdef SIMULATOR
#include "config/sim.h"
#ifndef HAVE_POWEROFF_WHILE_CHARGING
    #define HAVE_POWEROFF_WHILE_CHARGING
#endif
#endif

#if defined(__PCTOOL__) || defined(SIMULATOR)
#ifndef CONFIG_PLATFORM
#define CONFIG_PLATFORM PLATFORM_HOSTED
#endif
#endif

#ifndef CONFIG_PLATFORM
#define CONFIG_PLATFORM PLATFORM_NATIVE
#endif

/* setup basic macros from capability masks */
#include "config_caps.h"

/* setup CPU-specific defines */

#ifndef __PCTOOL__

/* define for all cpus from coldfire family */
#if (ARCH == ARCH_M68K) && ((CONFIG_CPU == MCF5249) || (CONFIG_CPU == MCF5250))
#define CPU_COLDFIRE
#endif

/* define for all cpus from PP family */
#if (CONFIG_CPU == PP5002)
#define CPU_PP
#elif (CONFIG_CPU == PP5020) || (CONFIG_CPU == PP5022) \
    || (CONFIG_CPU == PP5024) || (CONFIG_CPU == PP6100)
#define CPU_PP
#define CPU_PP502x
#endif

/* define for all cpus from S5L870X family */
#if (CONFIG_CPU == S5L8700) || (CONFIG_CPU == S5L8701) || (CONFIG_CPU == S5L8702)
#define CPU_S5L870X
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
#if ARCH == ARCH_ARM
#define CPU_ARM
#define ARM_ARCH ARCH_VERSION /* ARMv{4,5,6,7} */
#endif

#if ARCH == ARCH_MIPS
#define CPU_MIPS ARCH_VERSION /* 32, 64 */
#endif

#endif /*__PCTOOL__*/

/* now set any CONFIG_ defines correctly if they are not used,
   No need to do this on CONFIG_'s which are compulsory (e.g CONFIG_CODEC ) */

#if !defined(CONFIG_BACKLIGHT_FADING)
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_NO_FADING
#endif

#ifndef CONFIG_I2C
#define CONFIG_I2C I2C_NONE
#endif

#ifndef CONFIG_TUNER
#define CONFIG_TUNER 0
#endif

#ifndef CONFIG_USBOTG
#define CONFIG_USBOTG 0
#endif

#ifndef CONFIG_LED
#define CONFIG_LED LED_VIRTUAL
#endif

#ifndef CONFIG_CHARGING
#define CONFIG_CHARGING 0
#endif

#ifndef CONFIG_BATTERY_MEASURE
#define CONFIG_BATTERY_MEASURE 0
#define NO_LOW_BATTERY_SHUTDOWN
#endif

#ifndef CONFIG_RTC
#define CONFIG_RTC 0
#endif

#ifndef BATTERY_TYPES_COUNT
#define BATTERY_TYPES_COUNT 0
#endif

#ifndef BATTERY_CAPACITY_DEFAULT
#define BATTERY_CAPACITY_DEFAULT 0
#endif

#ifndef BATTERY_CAPACITY_MIN
#define BATTERY_CAPACITY_MIN BATTERY_CAPACITY_DEFAULT
#endif

#ifndef BATTERY_CAPACITY_MAX
#define BATTERY_CAPACITY_MAX BATTERY_CAPACITY_DEFAULT
#endif

#ifndef BATTERY_CAPACITY_INC
#define BATTERY_CAPACITY_INC 0
#endif

#ifdef HAVE_RDS_CAP
/* combinable bitflags */
/* 0x01 can be reused, was RDS_CFG_ISR */
#define RDS_CFG_PROCESS 0x2 /* uses raw packet processing */
#define RDS_CFG_PUSH    0x4 /* pushes processed information */
#define RDS_CFG_POLL    0x8 /* tuner driver provides a polling function */
#ifndef CONFIG_RDS
#define CONFIG_RDS  RDS_CFG_PROCESS /* thread processing+raw processing */
#endif /* CONFIG_RDS */
#endif /* HAVE_RDS_CAP */

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
#if CONFIG_RTC == RTC_AS3514
#if CONFIG_CPU == AS3525 || CONFIG_CPU == AS3525v2
#define HAVE_RTC_IRQ
#endif
#elif CONFIG_RTC == RTC_MC13783
#define HAVE_RTC_IRQ
#endif
#endif /* (CONFIG_PLATFORM & PLATFORM_NATIVE) */

#ifndef CONFIG_ORIENTATION
#if LCD_HEIGHT > LCD_WIDTH
#define CONFIG_ORIENTATION SCREEN_PORTRAIT
#elif LCD_HEIGHT < LCD_WIDTH
#define CONFIG_ORIENTATION SCREEN_LANDSCAPE
#else
#define CONFIG_ORIENTATION SCREEN_SQUARE
#endif
#endif

/* Pixel aspect ratio is defined in terms of a multiplier for pixel width and
 * height, and is set to 1:1 if the target does not set a value
 */
#ifndef LCD_PIXEL_ASPECT_HEIGHT
#define LCD_PIXEL_ASPECT_HEIGHT 1
#endif
#ifndef LCD_PIXEL_ASPECT_WIDTH
#define LCD_PIXEL_ASPECT_WIDTH 1
#endif

/* Used for split displays (Sansa Clip). Set to 0 otherwise */
#ifndef LCD_SPLIT_LINES
#define LCD_SPLIT_LINES 0
#endif

/* Most displays have a horizontal stride */
#ifndef LCD_STRIDEFORMAT
# define LCD_STRIDEFORMAT HORIZONTAL_STRIDE
#endif

/* Simulator LCD dimensions. Set to standard dimensions if undefined */
#ifndef SIM_LCD_WIDTH
#define SIM_LCD_WIDTH LCD_WIDTH
#endif
#ifndef SIM_LCD_HEIGHT
#define SIM_LCD_HEIGHT (LCD_HEIGHT + LCD_SPLIT_LINES)
#endif

#ifdef HAVE_REMOTE_LCD
#ifndef SIM_REMOTE_WIDTH
#define SIM_REMOTE_WIDTH LCD_REMOTE_WIDTH
#endif
#ifndef SIM_REMOTE_HEIGHT
#define SIM_REMOTE_HEIGHT LCD_REMOTE_HEIGHT
#endif
#endif /* HAVE_REMOTE_LCD */

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

#if LCD_DEPTH > 1 || defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
#define HAVE_BACKDROP_IMAGE
#endif

#if (CONFIG_TUNER & (CONFIG_TUNER - 1)) != 0
/* Multiple possible tuners */
#define CONFIG_TUNER_MULTI
#endif

/* deactivate fading in bootloader */
#if defined(BOOTLOADER)
#undef CONFIG_BACKLIGHT_FADING
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_NO_FADING
#endif

/* determine which setting/manual text to use */
#if (CONFIG_BACKLIGHT_FADING == BACKLIGHT_FADING_PWM)

/* possibly overridden in target config */
#if !defined(HAVE_BACKLIGHT_FADING_BOOL_SETTING) \
    && !defined(HAVE_BACKLIGHT_FADING_INT_SETTING)
#define HAVE_BACKLIGHT_FADING_INT_SETTING
#endif

#elif  (CONFIG_BACKLIGHT_FADING == BACKLIGHT_FADING_SW_SETTING) \
    || (CONFIG_BACKLIGHT_FADING == BACKLIGHT_FADING_SW_HW_REG) \
    || (CONFIG_BACKLIGHT_FADING == BACKLIGHT_FADING_TARGET)

/* possibly overridden in target config */
#if !defined(HAVE_BACKLIGHT_FADING_BOOL_SETTING) \
    && !defined(HAVE_BACKLIGHT_FADING_INT_SETTING)
#define HAVE_BACKLIGHT_FADING_BOOL_SETTING
#endif

#endif /* CONFIG_BACKLIGHT_FADING */

/* Storage related config handling */

#if (CONFIG_STORAGE & (CONFIG_STORAGE - 1)) != 0
/* Multiple storage drivers */
#define CONFIG_STORAGE_MULTI
#endif

#if defined(CONFIG_STORAGE_MULTI) && !defined(HAVE_MULTIDRIVE)
#define HAVE_MULTIDRIVE
#endif

#if defined(HAVE_MULTIDRIVE) && !defined(NUM_DRIVES)
#error HAVE_MULTIDRIVE needs to have an explicit NUM_DRIVES
#endif

#ifndef NUM_DRIVES
#define NUM_DRIVES 1
#endif

#if !defined(HAVE_MULTIVOLUME)
#if defined(HAVE_MULTIDRIVE)
/* Multidrive strongly implies multivolume */
#define HAVE_MULTIVOLUME
#elif (CONFIG_STORAGE & STORAGE_SD)
/* SD routinely have multiple partitions */
#elif (CONFIG_STORAGE & STORAGE_ATA) && defined(HAVE_LBA48)
/* ATA routinely haves multiple partitions, but don't bother if we can't do LBA48 */
#define HAVE_MULTIVOLUME
#endif
#endif

/* Bootloaders don't need multivolume awareness */
#if defined(BOOTLOADER) && defined(HAVE_MULTIVOLUME) \
    && !(CONFIG_PLATFORM & PLATFORM_HOSTED) && !defined(BOOT_REDIR)
#undef HAVE_MULTIVOLUME
#endif

/* Number of volumes per drive */
#if defined(HAVE_MULTIVOLUME) && !defined(SIMULATOR) && !(CONFIG_PLATFORM & PLATFORM_HOSTED)
#define NUM_VOLUMES_PER_DRIVE 4
#else
#define NUM_VOLUMES_PER_DRIVE 1
#endif

/* note to remove multi-partition booting this could be changed to MULTIDRIVE */
#if defined(HAVE_BOOTDATA) && defined(BOOT_REDIR) && defined(HAVE_MULTIVOLUME)
#define HAVE_MULTIBOOT
#endif

/* The lowest numbered volume to read a multiboot redirect from; default is to
 * allow any volume but some targets may wish to exclude the internal drive. */
#if defined(HAVE_MULTIBOOT) && !defined(MULTIBOOT_MIN_VOLUME)
# define MULTIBOOT_MIN_VOLUME 0
#endif

#define NUM_VOLUMES (NUM_DRIVES * NUM_VOLUMES_PER_DRIVE)

#if defined(BOOTLOADER) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
/* Bootloaders don't use CPU frequency adjustment */
#undef HAVE_ADJUSTABLE_CPU_FREQ
#endif

#if defined(__PCTOOL__) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
/* PCTOOLs don't use CPU frequency adjustment */
#undef HAVE_ADJUSTABLE_CPU_FREQ
#endif

/* Enable the directory cache and tagcache in RAM if we have
 * plenty of RAM. Both features can be enabled independently. */
#if (MEMORYSIZE >= 8) && !defined(BOOTLOADER) && (defined(CHECKWPS) || !defined(__PCTOOL__)) \
    && !defined(APPLICATION)
#ifndef SIMULATOR
#define HAVE_DIRCACHE
#endif
#ifdef HAVE_TAGCACHE
#define HAVE_TC_RAMCACHE
#endif
#endif

#if defined(HAVE_TAGCACHE)
#define HAVE_PICTUREFLOW_INTEGRATION
#endif

#ifdef BOOTLOADER

#ifdef HAVE_BOOTLOADER_USB_MODE
/* Priority in bootloader is wanted */
#define HAVE_PRIORITY_SCHEDULING

#if (CONFIG_CPU == S5L8702)
#define USB_DRIVER_CLOSE
#else
#define USB_STATUS_BY_EVENT
#define USB_DETECT_BY_REQUEST
#endif

#if defined(HAVE_USBSTACK) && CONFIG_USBOTG == USBOTG_ARC
#define INCLUDE_TIMEOUT_API
#define USB_DRIVER_CLOSE
#endif

#if defined(HAVE_USBSTACK) && CONFIG_USBOTG == USBOTG_TNETV105
#define INCLUDE_TIMEOUT_API
#define USB_DRIVER_CLOSE
#endif

#if CONFIG_CPU == X1000
#define USB_DRIVER_CLOSE
#endif

#endif /* BOOTLOADER_USB_MODE */

#else /* !BOOTLOADER */

#define HAVE_EXTENDED_MESSAGING_AND_NAME
#define HAVE_WAKEUP_EXT_CB

#if defined(ASSEMBLER_THREADS) \
    || defined(HAVE_WIN32_FIBER_THREADS) \
    || defined(HAVE_SIGALTSTACK_THREADS)
#define HAVE_PRIORITY_SCHEDULING
#endif

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
#define HAVE_PRIORITY_SCHEDULING
#define HAVE_SCHEDULER_BOOSTCTRL
#endif /* PLATFORM_NATIVE */


#ifdef HAVE_USBSTACK
#if CONFIG_USBOTG == USBOTG_ARC
#define USB_STATUS_BY_EVENT
#define USB_DETECT_BY_REQUEST
#define INCLUDE_TIMEOUT_API
#elif CONFIG_USBOTG == USBOTG_AS3525
#define USB_STATUS_BY_EVENT
#define USB_DETECT_BY_REQUEST
#elif CONFIG_USBOTG == USBOTG_S3C6400X /* FIXME */ && CONFIG_CPU != S5L8701
#define USB_STATUS_BY_EVENT
#define USB_DETECT_BY_REQUEST
#elif CONFIG_USBOTG == USBOTG_DESIGNWARE /* FIXME */ && CONFIG_CPU != S5L8701
#define USB_STATUS_BY_EVENT
#define USB_DETECT_BY_REQUEST
#elif CONFIG_USBOTG == USBOTG_RK27XX
#define USB_DETECT_BY_REQUEST
#elif CONFIG_USBOTG == USBOTG_TNETV105
#define USB_STATUS_BY_EVENT
#define USB_DETECT_BY_REQUEST
#endif /* CONFIG_USB == */
#endif /* HAVE_USBSTACK */

#endif /* !BOOTLOADER */

#if defined(HAVE_USBSTACK) || (CONFIG_CPU == JZ4732) || (CONFIG_CPU == JZ4760B) \
    || (CONFIG_CPU == AS3525) || (CONFIG_CPU == AS3525v2) \
    || defined(CPU_S5L870X) || (CONFIG_CPU == S3C2440) \
    || defined(APPLICATION) || (CONFIG_CPU == PP5002) \
    || (CONFIG_CPU == RK27XX) || (CONFIG_CPU == IMX233) ||              \
    (defined(HAVE_LCD_COLOR) && (LCD_STRIDEFORMAT == HORIZONTAL_STRIDE))
#define HAVE_SEMAPHORE_OBJECTS
#endif

/*include support for crossfading - requires significant PCM buffer space*/
#if MEMORYSIZE > 2
#define HAVE_CROSSFADE
#endif

/* Determine if accesses should be strictly long aligned. */
#if defined(CPU_ARM) || defined(CPU_MIPS)
#define ROCKBOX_STRICT_ALIGN 1
#endif

/*
 * These macros are for switching on unified syntax in inline assembly.
 * Older versions of GCC emit assembly in divided syntax with no option
 * to enable unified syntax.
 */
#if (__GNUC__ < 8)
#define BEGIN_ARM_ASM_SYNTAX_UNIFIED ".syntax unified\n"
#define END_ARM_ASM_SYNTAX_UNIFIED   ".syntax divided\n"
#else
#define BEGIN_ARM_ASM_SYNTAX_UNIFIED
#define END_ARM_ASM_SYNTAX_UNIFIED
#endif

#if defined(CPU_ARM) && defined(__ASSEMBLER__)
#if (__GNUC__ < 8)
.syntax unified
#endif
/* ARMv4T doesn't switch the T bit when popping pc directly, we must use BX */
.macro ldmpc cond="", order="ia", regs
#if ARM_ARCH == 4 && defined(USE_THUMB)
    ldm\order\cond sp!, { \regs, lr }
    bx\cond lr
#else
    ldm\order\cond sp!, { \regs, pc }
#endif
.endm
.macro ldrpc cond=""
#if ARM_ARCH == 4 && defined(USE_THUMB)
    ldr\cond lr, [sp], #4
    bx\cond  lr
#else
    ldr\cond pc, [sp], #4
#endif
.endm
#if (__GNUC__ < 8)
.syntax divided
#endif
#endif

#if defined(CPU_COLDFIRE) && defined(__ASSEMBLER__)
/* Assembler doesn't support these as mnemonics but does tpf */
.macro tpf.w
.word 0x51fa
.endm

.macro tpf.l
.word 0x51fb
.endm
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
#if (CONFIG_PLATFORM & PLATFORM_NATIVE) &&   /* Not for hosted environments */ \
    (defined(CPU_COLDFIRE) || /* Coldfire: core, plugins, codecs */ \
    defined(CPU_PP) ||  /* PortalPlayer: core, plugins, codecs */ \
    (CONFIG_CPU == AS3525 && MEMORYSIZE > 2 && !defined(BOOTLOADER)) || /* AS3525 +2MB: core, plugins, codecs */ \
    (CONFIG_CPU == AS3525 && MEMORYSIZE <= 2 && !defined(PLUGIN) && !defined(CODEC) && !defined(BOOTLOADER)) || /* AS3525 2MB: core only */ \
    (CONFIG_CPU == AS3525v2 && !defined(PLUGIN) && !defined(CODEC) && !defined(BOOTLOADER)) || /* AS3525v2: core only */ \
    (CONFIG_CPU == PNX0101) || \
    (CONFIG_CPU == TCC7801) || \
    (CONFIG_CPU == IMX233 && !defined(PLUGIN) && !defined(CODEC)) || /* IMX233: core only */ \
    defined(CPU_S5L870X)) || /* Samsung S5L8700: core, plugins, codecs */ \
    ((CONFIG_CPU == JZ4732 || CONFIG_CPU == JZ4760B) && !defined(PLUGIN) && !defined(CODEC)) /* Jz47XX: core only */
#define ICODE_ATTR      __attribute__ ((section(".icode")))
#define ICONST_ATTR     __attribute__ ((section(".irodata")))
#define IDATA_ATTR      __attribute__ ((section(".idata")))
#define IBSS_ATTR       __attribute__ ((section(".ibss")))
#define USE_IRAM
#if (CONFIG_CPU != AS3525 || MEMORYSIZE > 2) \
    && CONFIG_CPU != JZ4732 && CONFIG_CPU != JZ4760B && CONFIG_CPU != AS3525v2 && CONFIG_CPU != IMX233
#define PLUGIN_USE_IRAM
#endif
#else
#define ICODE_ATTR
#define ICONST_ATTR
#define IDATA_ATTR
#define IBSS_ATTR
#endif

#if (defined(CPU_PP) || (CONFIG_CPU == AS3525) || (CONFIG_CPU == AS3525v2) || \
    (CONFIG_CPU == IMX31L) || (CONFIG_CPU == IMX233) || \
     (CONFIG_CPU == RK27XX) || defined(CPU_MIPS) || defined(CPU_COLDFIRE)) \
    && (CONFIG_PLATFORM & PLATFORM_NATIVE) && !defined(BOOTLOADER)
/* Functions that have INIT_ATTR attached are NOT guaranteed to survive after
 * root_menu() has been called. Their code may be overwritten by other data or
 * code in order to save RAM, and references to them might point into
 * zombie area.
 *
 * It is critical that you make sure these functions are only called before
 * the final call to root_menu() (see apps/main.c) is called (i.e. basically
 * only while main() runs), otherwise things may go wild,
 * from crashes to freezes to exploding daps.
 */


#if defined(__APPLE__) && defined(__MACH__)
    #define INIT_ATTR __attribute__((section ("__INIT,.init")))
    #define INITDATA_ATTR __attribute__((section ("__INITDATA,.initdata")))
#else
    #define INIT_ATTR       __attribute__ ((section(".init")))
    #define INITDATA_ATTR   __attribute__ ((section(".initdata")))
#endif

#define HAVE_INIT_ATTR
#else
#define INIT_ATTR
#define INITDATA_ATTR
#endif

/* We need to call storage_init more than once only if USB storage mode is
 * handled in hardware:
 * Deinit storage -> let hardware handle USB mode -> storage_init() again
 */
#if defined(HAVE_USBSTACK) || defined(USB_NONE)
#define STORAGE_INIT_ATTR INIT_ATTR
#else
#define STORAGE_INIT_ATTR
#endif

#if (CONFIG_PLATFORM & PLATFORM_HOSTED) && defined(__APPLE__)
#define DATA_ATTR       __attribute__ ((section("__DATA, .data")))
#else
#define DATA_ATTR       __attribute__ ((section(".data")))
#endif

#ifndef IRAM_LCDFRAMEBUFFER
/* if the LCD framebuffer has not been moved to IRAM, define it empty here */
#define IRAM_LCDFRAMEBUFFER
#endif

/* Change this if you want to build a single-core firmware for a multicore
 * target for debugging */
#if defined(BOOTLOADER) || (CONFIG_CPU == PP6100)
#define FORCE_SINGLE_CORE
#endif

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
#define HAVE_CORELOCK_OBJECT
#define CURRENT_CORE current_core()
/* Attributes for core-shared data in DRAM where IRAM is better used for other
 * purposes. */
#define SHAREDBSS_ATTR     NOCACHEBSS_ATTR
#define SHAREDDATA_ATTR    NOCACHEDATA_ATTR

#define IF_COP(...)         __VA_ARGS__
#define IF_COP_VOID(...)    __VA_ARGS__
#define IF_COP_CORE(core)   core

#endif /* !defined(FORCE_SINGLE_CORE) */

#endif /* CPU_PP */

#if CONFIG_CPU == IMX31L || CONFIG_CPU == IMX233
#define NOCACHEBSS_ATTR     __attribute__((section(".ncbss"),nocommon))
#define NOCACHEDATA_ATTR    __attribute__((section(".ncdata"),nocommon))
#endif

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

#define IF_COP(...)
#define IF_COP_VOID(...)    void
#define IF_COP_CORE(core)   CURRENT_CORE

#endif /* NUM_CORES */

#ifdef HAVE_HEADPHONE_DETECTION
/* Timeout objects required if headphone detection is enabled */
#define INCLUDE_TIMEOUT_API
#endif /* HAVE_HEADPHONE_DETECTION */

#ifdef HAVE_TOUCHSCREEN
/* Timeout objects required for kinetic list scrolling */
#define INCLUDE_TIMEOUT_API
/* Enable skin variable system, may not be the best place for this #define. */
#define HAVE_SKIN_VARIABLES
#endif /* HAVE_TOUCHSCREEN */

#if defined(HAVE_USB_CHARGING_ENABLE) && defined(HAVE_USBSTACK)
/* USB charging support in the USB stack requires timeout objects */
#define INCLUDE_TIMEOUT_API
#endif /* HAVE_USB_CHARGING_ENABLE && HAVE_USBSTACK */

#if defined(USB_STATUS_BY_EVENT) && defined(HAVE_USBSTACK)
/* Status by event requires timeout for debouncing */
# define INCLUDE_TIMEOUT_API
#endif

#ifndef SIMULATOR
#if defined(HAVE_USBSTACK) || (CONFIG_STORAGE & STORAGE_NAND) || (CONFIG_STORAGE & STORAGE_RAMDISK)
#define STORAGE_GET_INFO
#endif
#endif

#if defined(HAVE_SIGALTSTACK_THREADS)
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600   /* For sigaltstack */
#endif
#endif

#ifdef CPU_MIPS
#include <stdbool.h> /* MIPS GCC fix? */
#endif

#if defined(HAVE_USBSTACK)
/* Define the implemented USB transport classes */
#if CONFIG_USBOTG == USBOTG_ISP1583
#define USB_HAS_BULK
#define USB_LEGACY_CONTROL_API
#elif (CONFIG_USBOTG == USBOTG_DESIGNWARE)
#define USB_HAS_BULK
#define USB_HAS_INTERRUPT
#elif (CONFIG_USBOTG == USBOTG_ARC) ||  \
    (CONFIG_USBOTG == USBOTG_JZ4740) || \
    (CONFIG_USBOTG == USBOTG_JZ4760) || \
    (CONFIG_USBOTG == USBOTG_M66591) || \
    (CONFIG_USBOTG == USBOTG_AS3525) || \
    (CONFIG_USBOTG == USBOTG_RK27XX) || \
    (CONFIG_USBOTG == USBOTG_TNETV105)
#define USB_HAS_BULK
#define USB_HAS_INTERRUPT
#define USB_LEGACY_CONTROL_API
#elif defined(CPU_TCC780X)
#define USB_HAS_BULK
#define USB_LEGACY_CONTROL_API
#elif CONFIG_USBOTG == USBOTG_S3C6400X
#define USB_HAS_BULK
#define USB_LEGACY_CONTROL_API
//#define USB_HAS_INTERRUPT -- seems to be broken
#endif /* CONFIG_USBOTG */

#if (CONFIG_USBOTG == USBOTG_ARC) || \
    (CONFIG_USBOTG == USBOTG_AS3525)
#define USB_HAS_ISOCHRONOUS
#endif

/* define the class drivers to enable */
#ifdef BOOTLOADER

/* enable usb storage for targets that do bootloader usb */
#if defined(HAVE_BOOTLOADER_USB_MODE) || \
     defined(CREATIVE_ZVx) || defined(CPU_TCC780X) || \
     CONFIG_USBOTG == USBOTG_JZ4740 || CONFIG_USBOTG == USBOTG_AS3525 || \
     CONFIG_USBOTG == USBOTG_S3C6400X || CONFIG_USBOTG == USBOTG_DESIGNWARE || \
     CONFIG_USBOTG == USBOTG_JZ4760
#define USB_ENABLE_STORAGE
#endif

#else /* BOOTLOADER */

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
#ifdef USB_HAS_BULK
//#define USB_ENABLE_SERIAL
#define USB_ENABLE_STORAGE
#endif /* USB_HAS_BULK */

#ifdef USB_HAS_INTERRUPT
#define USB_ENABLE_HID
#else
#define USB_ENABLE_CHARGING_ONLY
#endif
#endif

#endif /* BOOTLOADER */

#endif /* HAVE_USBSTACK */

/* This attribute can be used to enable to detection of plugin file handles leaks.
 * When enabled, the plugin core will monitor open/close/creat and when the plugin exits
 * will display an error message if the plugin leaked some file handles */
#if (CONFIG_PLATFORM & PLATFORM_NATIVE) || defined (SIMULATOR)
#define HAVE_PLUGIN_CHECK_OPEN_CLOSE
#endif

#if defined(CPU_COLDIRE) || CONFIG_CPU == IMX31L
/* Can record and play simultaneously */
#define HAVE_PCM_FULL_DUPLEX
#endif

#define HAVE_PITCHCONTROL

/* enable logging messages to disk*/
#if !defined(BOOTLOADER) && !defined(__PCTOOL__)
#define ROCKBOX_HAS_LOGDISKF
#endif

#if defined(HAVE_SDL_AUDIO) \
    && !(CONFIG_PLATFORM & PLATFORM_MAEMO5) \
    && !defined(HAVE_SW_VOLUME_CONTROL)
/* SW volume is needed for accurate control and no double buffering should be
 * required. If target uses SW volume, then its definitions are used instead
 * so things are as on target. */
#define HAVE_SW_VOLUME_CONTROL
#define PCM_SW_VOLUME_UNBUFFERED /* pcm driver itself is buffered */
#ifdef SIMULATOR
/* For sim, nice res for ~ -127dB..+36dB that so far covers all targets */
#define PCM_SW_VOLUME_FRACBITS  (24)
#else
/* For app, use fractional-only setup for -79..+0, no large-integer math */
#define PCM_SW_VOLUME_FRACBITS  (16)
#endif /* SIMULATOR */
#endif /* default SDL SW volume conditions */

#if !defined(BOOTLOADER) || defined(HAVE_BOOTLOADER_SCREENDUMP)
# define HAVE_SCREENDUMP
#endif

#if !defined(BOOTLOADER) && MEMORYSIZE > 2
# define HAVE_PERCEPTUAL_VOLUME
#endif

/*
 * Turn off legacy codepage handling in the filesystem code for bootloaders,
 * and support ISO-8859-1 (Latin-1) only. This only affects DOS 8.3 filename
 * parsing when FAT32 long names are unavailable; long names are Unicode and
 * can always be decoded properly regardless of this setting.
 *
 * In reality, bootloaders never supported codepages other than Latin-1 in
 * the first place. They did contain the code to load codepages from disk,
 * but had no way to actually change the codepage away from Latin-1.
 */
#if !defined(BOOTLOADER)
# define HAVE_FILESYSTEM_CODEPAGE
#endif

/* null audiohw setting macro for when codec header is included for reasons
   other than audio support */
#define AUDIOHW_SETTING(name, us, nd, st, minv, maxv, defv, expr...)

#endif /* __CONFIG_H__ */
