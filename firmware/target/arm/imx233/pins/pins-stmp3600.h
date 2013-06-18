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
#ifndef __PINS_STMP3600__
#define __PINS_STMP3600__

#define VPIN_PWM(channel)   VPIN_PACK(3, 10 + (channel), MAIN)

#define VPIN_I2C_SCL        VPIN_PACK(3, 17, MAIN)
#define VPIN_I2C_SDA        VPIN_PACK(3, 18, MAIN)

#define VPIN_SSP1_DET       VPIN_PACK(0, 25, MAIN)
#define VPIN_SSP1_CMD       VPIN_PACK(0, 26, MAIN)
#define VPIN_SSP1_SCK       VPIN_PACK(0, 27, MAIN)
#define VPIN_SSP1_D0        VPIN_PACK(0, 28, MAIN)
#define VPIN_SSP1_D1        VPIN_PACK(0, 29, MAIN)
#define VPIN_SSP1_D2        VPIN_PACK(0, 30, MAIN)
#define VPIN_SSP1_D3        VPIN_PACK(0, 31, MAIN)

#define VPIN_UARTDBG_TX     VPIN_PACK(3, 11, ALT2)
#define VPIN_UARTDBG_RX     VPIN_PACK(3, 10, ALT2)

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
#define VPIN_LCD_RESET      VPIN_PACK(1, 16, MAIN)
#define VPIN_LCD_RS         VPIN_PACK(1, 17, MAIN)
#define VPIN_LCD_WR         VPIN_PACK(1, 18, MAIN)
#define VPIN_LCD_CS         VPIN_PACK(1, 19, MAIN)

#define VPIN_GPMI_D0        VPIN_PACK(0, 0, MAIN)
#define VPIN_GPMI_D1        VPIN_PACK(0, 1, MAIN)
#define VPIN_GPMI_D2        VPIN_PACK(0, 2, MAIN)
#define VPIN_GPMI_D3        VPIN_PACK(0, 3, MAIN)
#define VPIN_GPMI_D4        VPIN_PACK(0, 4, MAIN)
#define VPIN_GPMI_D5        VPIN_PACK(0, 5, MAIN)
#define VPIN_GPMI_D6        VPIN_PACK(0, 6, MAIN)
#define VPIN_GPMI_D7        VPIN_PACK(0, 7, MAIN)
#define VPIN_GPMI_D8        VPIN_PACK(0, 8, MAIN)
#define VPIN_GPMI_D9        VPIN_PACK(0, 9, MAIN)
#define VPIN_GPMI_D10       VPIN_PACK(0, 10, MAIN)
#define VPIN_GPMI_D11       VPIN_PACK(0, 11, MAIN)
#define VPIN_GPMI_D12       VPIN_PACK(0, 12, MAIN)
#define VPIN_GPMI_D13       VPIN_PACK(0, 13, MAIN)
#define VPIN_GPMI_D14       VPIN_PACK(0, 14, MAIN)
#define VPIN_GPMI_D15       VPIN_PACK(0, 15, MAIN)
#define VPIN_GPMI_IRQ       VPIN_PACK(0, 16, MAIN)
#define VPIN_GPMI_RDn       VPIN_PACK(0, 17, MAIN)
#define VPIN_GPMI_RDY       VPIN_PACK(0, 18, MAIN)
#define VPIN_GPMI_RDY3      VPIN_PACK(0, 19, MAIN)
#define VPIN_GPMI_RDY2      VPIN_PACK(0, 20, MAIN)
#define VPIN_GPMI_WRn       VPIN_PACK(0, 21, MAIN)
#define VPIN_GPMI_A0        VPIN_PACK(0, 22, MAIN)
#define VPIN_GPMI_A1        VPIN_PACK(0, 23, MAIN)
#define VPIN_GPMI_A2        VPIN_PACK(0, 24, MAIN)
#define VPIN_GPMI_RESETn    VPIN_PACK(1, 20, MAIN)
#define VPIN_GPMI_CE0n      VPIN_PACK(3, 0, ALT1)
#define VPIN_GPMI_CE1n      VPIN_PACK(3, 1, ALT1)

#endif /* __PINS_STMP3600__ */