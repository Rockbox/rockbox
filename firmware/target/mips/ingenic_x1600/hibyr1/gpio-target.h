/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

/* Pins from the vendor GPIO labels, functions from the PM. I2C0 uses
 * function 2; microSD uses MSC1. */

/*              Name            Port    Pins            Function */

/* The DPU drives the top six bits of each lane, leaving lcd_d0/d1 for SPI */
DEFINE_PINGROUP(LCD_DATA,       GPIO_A, 0x00fcfcfc,     GPIOF_DEVICE(0))
DEFINE_PINGROUP(LCD_CONTROL,    GPIO_A,    0xf << 24,   GPIOF_DEVICE(0))
/* i2c0: AXP2101 PMU @0x34 and CW2015 fuel gauge @0x62, 400 kHz */
DEFINE_PINGROUP(I2C0,           GPIO_A,      3 << 28,   GPIOF_DEVICE(2))
DEFINE_PINGROUP(MSC0,           GPIO_B,   0x3f << 12,   GPIOF_DEVICE(0))
/* i2c1: CST8xx touch controller @0x15, 400 kHz */
DEFINE_PINGROUP(I2C1,           GPIO_B,      3 << 19,   GPIOF_DEVICE(0))
/* AIC transmit side only */
DEFINE_PINGROUP(I2S,            GPIO_B,    0xf << 25,   GPIOF_DEVICE(0))
/* pwm0_o -> LCD backlight */
DEFINE_PINGROUP(PWM0,           GPIO_C,      1 <<  0,   GPIOF_DEVICE(0))
/* SFC: Winbond W25N01GV SPI-NAND boot flash */
DEFINE_PINGROUP(SFC,            GPIO_C,   0x3f << 17,   GPIOF_DEVICE(0))
DEFINE_PINGROUP(MSC1,           GPIO_D,   0x3f <<  0,   GPIOF_DEVICE(0))

/*          Name                Pin             Function */

/* Panel config SPI: 3-wire, 9-bit, MSB first, SCK and CS idling high */
DEFINE_GPIO(LCD_SPI_SCK,        GPIO_PA(0),     GPIOF_OUTPUT(1))
DEFINE_GPIO(LCD_SPI_MOSI,       GPIO_PA(1),     GPIOF_OUTPUT(0))

/* Labelled "wifi host wake" by the vendor; direction inferred, unverified */
DEFINE_GPIO(WIFI_HOST_WAKE,     GPIO_PA(8),     GPIOF_INPUT)
/* The vendor labels PA09 as both TCS1421 CFG0 and CW2015 interrupt. The
 * gauge driver does not use its interrupt; the TCS1421 strap function is
 * unknown. */
DEFINE_GPIO(TCS1421_CFG0,       GPIO_PA(9),     GPIOF_OUTPUT(0))

DEFINE_GPIO(CST8XX_INTERRUPT,   GPIO_PA(16),    GPIOF_INPUT)
DEFINE_GPIO(CST8XX_RESET,       GPIO_PA(17),    GPIOF_OUTPUT(0))

DEFINE_GPIO(LCD_SPI_CS,         GPIO_PA(30),    GPIOF_OUTPUT(1))
DEFINE_GPIO(LCD_RST,            GPIO_PA(31),    GPIOF_OUTPUT(0))

/* CS43131 DAC power and reset, both active high */
DEFINE_GPIO(CS43131_POWER,      GPIO_PB(2),     GPIOF_OUTPUT(0))
DEFINE_GPIO(WLAN_REG_ON,        GPIO_PB(3),     GPIOF_OUTPUT(0))
DEFINE_GPIO(BT_REG_ON,          GPIO_PB(4),     GPIOF_OUTPUT(0))
DEFINE_GPIO(HOST_WAKE_BT,       GPIO_PB(5),     GPIOF_OUTPUT(0))
DEFINE_GPIO(CS43131_RESET,      GPIO_PB(21),    GPIOF_OUTPUT(0))

/* microSD card detect: cd-inverted, enable level 0 => card present = low */
DEFINE_GPIO(MSC1_CD,            GPIO_PB(22),    GPIOF_INPUT)
DEFINE_GPIO(AXP_IRQ,            GPIO_PB(23),    GPIOF_INPUT)
/* See TCS1421_CFG0 above */
DEFINE_GPIO(TCS1421_CFG1,       GPIO_PB(24),    GPIOF_OUTPUT(0))

/* Software I2C for the DAC. I2C0 is already assigned to PA28/PA29 for the
 * PMU and gauge; both DAC bus lines idle high. */
DEFINE_GPIO(CODEC_I2C_SCL,      GPIO_PB(30),    GPIOF_OUTPUT(1))
DEFINE_GPIO(CODEC_I2C_SDA,      GPIO_PB(31),    GPIOF_OUTPUT(1))

DEFINE_GPIO(LED_RED,            GPIO_PC(1),     GPIOF_OUTPUT(0))
DEFINE_GPIO(LED_BLUE,           GPIO_PC(2),     GPIOF_OUTPUT(0))
DEFINE_GPIO(USB_DRVVBUS,        GPIO_PC(24),    GPIOF_OUTPUT(0))
/* microSD card power, active high (msc1_pwr_enable_level=1) */
DEFINE_GPIO(MSC1_POWER,         GPIO_PC(25),    GPIOF_OUTPUT(1))

/* PC28 doubles as the BOOT_SEL1 strap. Holding it while plugging USB
 * therefore drops the SoC into BootROM USB mode. Active *high*. */
DEFINE_GPIO(BTN_NEXT,           GPIO_PC(28),    GPIOF_INPUT)
/* PC31 is the SoC's WKUP_ pad. Active low. */
DEFINE_GPIO(BTN_POWER,          GPIO_PC(31),    GPIOF_INPUT)
