/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2013 by Amaury Pouly
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
#ifndef __PINS_IMX233__
#define __PINS_IMX233__

#define VPIN_PWM(channel)   VPIN_PACK(1, 26 + (channel), MAIN)

#define VPIN_I2C_SCL        VPIN_PACK(0, 30, MAIN)
#define VPIN_I2C_SDA        VPIN_PACK(0, 31, MAIN)

#define VPIN_SSP1_DET       VPIN_PACK(2, 1, MAIN)
#define VPIN_SSP1_CMD       VPIN_PACK(2, 0, MAIN)
#define VPIN_SSP1_SCK       VPIN_PACK(2, 6, MAIN)
#define VPIN_SSP1_D0        VPIN_PACK(2, 2, MAIN)
#define VPIN_SSP1_D1        VPIN_PACK(2, 3, MAIN)
#define VPIN_SSP1_D2        VPIN_PACK(2, 4, MAIN)
#define VPIN_SSP1_D3        VPIN_PACK(2, 5, MAIN)
#define VPIN_SSP1_D4        VPIN_PACK(0, 8, ALT2)
#define VPIN_SSP1_D5        VPIN_PACK(0, 9, ALT2)
#define VPIN_SSP1_D6        VPIN_PACK(0, 10, ALT2)
#define VPIN_SSP1_D7        VPIN_PACK(0, 11, ALT2)
#define VPIN_SSP1_D4_ALT    VPIN_PACK(0, 26, ALT2)
#define VPIN_SSP1_D5_ALT    VPIN_PACK(0, 27, ALT2)
#define VPIN_SSP1_D6_ALT    VPIN_PACK(0, 28, ALT2)
#define VPIN_SSP1_D7_ALT    VPIN_PACK(0, 29, ALT2)

#define VPIN_SSP2_DET       VPIN_PACK(0, 19, ALT2)
#define VPIN_SSP2_CMD       VPIN_PACK(0, 20, ALT2)
#define VPIN_SSP2_SCK       VPIN_PACK(0, 24, ALT2)
#define VPIN_SSP2_D0        VPIN_PACK(0, 0, ALT2)
#define VPIN_SSP2_D1        VPIN_PACK(0, 1, ALT2)
#define VPIN_SSP2_D2        VPIN_PACK(0, 2, ALT2)
#define VPIN_SSP2_D3        VPIN_PACK(0, 3, ALT2)
#define VPIN_SSP2_D4        VPIN_PACK(0, 4, ALT2)
#define VPIN_SSP2_D5        VPIN_PACK(0, 5, ALT2)
#define VPIN_SSP2_D6        VPIN_PACK(0, 6, ALT2)
#define VPIN_SSP2_D7        VPIN_PACK(0, 7, ALT2)

#define VPIN_UARTDBG_TX     VPIN_PACK(1, 27, ALT2)
#define VPIN_UARTDBG_RX     VPIN_PACK(1, 26, ALT2)

#define VPIN_LCD_D0         VPIN_PACK(1, 0, MAIN)
#define VPIN_LCD_D1         VPIN_PACK(1, 1, MAIN)
#define VPIN_LCD_D2         VPIN_PACK(1, 2, MAIN)
#define VPIN_LCD_D3         VPIN_PACK(1, 3, MAIN)
#define VPIN_LCD_D4         VPIN_PACK(1, 4, MAIN)
#define VPIN_LCD_D5         VPIN_PACK(1, 5, MAIN)
#define VPIN_LCD_D6         VPIN_PACK(1, 6, MAIN)
#define VPIN_LCD_D7         VPIN_PACK(1, 7, MAIN)
#define VPIN_LCD_D8         VPIN_PACK(1, 8, MAIN)
#define VPIN_LCD_D9         VPIN_PACK(1, 9, MAIN)
#define VPIN_LCD_D10        VPIN_PACK(1, 10, MAIN)
#define VPIN_LCD_D11        VPIN_PACK(1, 11, MAIN)
#define VPIN_LCD_D12        VPIN_PACK(1, 12, MAIN)
#define VPIN_LCD_D13        VPIN_PACK(1, 13, MAIN)
#define VPIN_LCD_D14        VPIN_PACK(1, 14, MAIN)
#define VPIN_LCD_D15        VPIN_PACK(1, 15, MAIN)
#define VPIN_LCD_D16        VPIN_PACK(1, 16, MAIN)
#define VPIN_LCD_D17        VPIN_PACK(1, 17, MAIN)
#define VPIN_LCD_RESET      VPIN_PACK(1, 18, MAIN)
#define VPIN_LCD_RS         VPIN_PACK(1, 19, MAIN)
#define VPIN_LCD_WR         VPIN_PACK(1, 20, MAIN)
#define VPIN_LCD_CS         VPIN_PACK(1, 21, MAIN)
#define VPIN_LCD_DOTCLK     VPIN_PACK(1, 22, MAIN)
#define VPIN_LCD_ENABLE     VPIN_PACK(1, 23, MAIN)
#define VPIN_LCD_HSYNC      VPIN_PACK(1, 24, MAIN)
#define VPIN_LCD_VSYNC      VPIN_PACK(1, 25, MAIN)

#endif /* __PINS_IMX233__ */
