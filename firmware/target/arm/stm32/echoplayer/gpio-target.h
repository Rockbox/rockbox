/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Aidan MacDonald
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
#ifndef __ECHOPLAYER_GPIO_TARGET_H__
#define __ECHOPLAYER_GPIO_TARGET_H__

#define GPIO_BUTTON_A               GPIO_PA(0)
#define GPIO_BUTTON_B               GPIO_PA(1)
#define GPIO_BUTTON_X               GPIO_PA(6)
#define GPIO_BUTTON_Y               GPIO_PA(4)
#define GPIO_BUTTON_START           GPIO_PD(12)
#define GPIO_BUTTON_SELECT          GPIO_PB(8)
#define GPIO_BUTTON_UP              GPIO_PB(14)
#define GPIO_BUTTON_DOWN            GPIO_PD(6)
#define GPIO_BUTTON_LEFT            GPIO_PD(7)
#define GPIO_BUTTON_RIGHT           GPIO_PC(6)
#define GPIO_BUTTON_VOL_UP          GPIO_PH(6)
#define GPIO_BUTTON_VOL_DOWN        GPIO_PB(15)
#define GPIO_BUTTON_POWER           GPIO_PF(8)
#define GPIO_BUTTON_HOLD            GPIO_PH(7)

#define GPIO_CPU_POWER_ON           GPIO_PA(2)
#define GPIO_POWER_1V8              GPIO_PD(3)
#define GPIO_CODEC_AVDD_EN          GPIO_PB(9)
#define GPIO_CODEC_DVDD_EN          GPIO_PC(5)
#define GPIO_CODEC_RESET            GPIO_PC(4)
#define GPIO_USBPHY_POWER_EN        GPIO_PD(4)
#define GPIO_USBPHY_RESET           GPIO_PD(5)

#define GPIO_CHARGER_CHARGING       GPIO_PA(10)
#define GPIO_CHARGER_ENABLE         GPIO_PA(15)

#define GPIO_SDMMC_DETECT           GPIO_PC(7)
#define GPIO_LINEOUT_DETECT         GPIO_PG(13)

#define GPIO_LCD_RESET              GPIO_PD(11)
#define GPIO_BACKLIGHT              GPIO_PD(13)

#endif /* __ECHOPLAYER_GPIO_TARGET_H__ */
