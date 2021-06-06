/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __ES9218_H__
#define __ES9218_H__

// TODO: look into LINEOUT_CAP
#define AUDIOHW_CAPS (FILTER_ROLL_OFF_CAP|POWER_MODE_CAP)
#define AUDIOHW_HAVE_ES9218_ROLL_OFF

#define ES9218_MIN_VOLUME (-1270)
#define ES9218_MAX_VOLUME 0

AUDIOHW_SETTING(VOLUME, "dB", 1, 5, ES9218_MIN_VOLUME, ES9218_MAX_VOLUME, -200)
AUDIOHW_SETTING(FILTER_ROLL_OFF, "", 0, 1, 0, 7, 0)
AUDIOHW_SETTING(POWER_MODE, "", 0, 1, 0, 1, 0)

/* Register addresses */
#define ES9218_REG_SYSTEM               0x00
#define ES9218_REG_INPUT_SEL            0x01
#define ES9218_REG_MIX_AUTOMUTE         0x02
#define ES9218_REG_ANALOG_VOL           0x03
#define ES9218_REG_AUTOMUTE_TIME        0x04
#define ES9218_REG_AUTUMUTE_LEVEL       0x05
#define ES9218_REG_DOP_VOLUME_RAMP      0x06
#define ES9218_REG_FILTER_SYS_MUTE      0x07
#define ES9218_REG_GPIO1_2_CONFIG       0x08
#define ES9218_REG_RESERVED_1           0x09
#define ES9218_REG_MASTER_MODE_CONFIG   0x0a
#define ES9218_REG_OVERCURRENT_PROT     0x0b
#define ES9218_REG_ASRC_DPLL_BANDWIDTH  0x0c
#define ES9218_REG_THD_COMP_BYPASS      0x0d
#define ES9218_REG_SOFT_START_CONFIG    0x0e
#define ES9218_REG_VOLUME_LEFT          0x0f
#define ES9218_REG_VOLUME_RIGHT         0x10
#define ES9218_REG_MASTER_TRIM_BIT0_7   0x11
#define ES9218_REG_MASTER_TRIM_BIT8_15  0x12
#define ES9218_REG_MASTER_TRIM_BIT16_23 0x13
#define ES9218_REG_MASTER_TRIM_BIT24_31 0x14
#define ES9218_REG_GPIO_INPUT_SEL       0x15
#define ES9218_REG_THD_COMP_C2_LO       0x16
#define ES9218_REG_THD_COMP_C2_HI       0x17
#define ES9218_REG_THD_COMP_C3_LO       0x18
#define ES9218_REG_THD_COMP_C3_HI       0x19
#define ES9218_REG_CHARGE_PUMP_SS_DELAY 0x1a
#define ES9218_REG_GENERAL_CONFIG       0x1b
#define ES9218_REG_RESERVED_2           0x1c
#define ES9218_REG_GPIO_INV_CLOCK_GEAR  0x1d
#define ES9218_REG_CHARGE_PUMP_CLK_LO   0x1e
#define ES9218_REG_CHARGE_PUMP_CLK_HI   0x1f
#define ES9218_REG_AMP_CONFIG           0x20
#define ES9218_REG_INTERRUPT_MASK       0x21
#define ES9218_REG_PROG_NCO_BIT0_7      0x22
#define ES9218_REG_PROG_NCO_BIT8_15     0x23
#define ES9218_REG_PROG_NCO_BIT16_23    0x24
#define ES9218_REG_PROG_NCO_BIT24_31    0x25
#define ES9218_REG_RESERVED_3           0x27
#define ES9218_REG_FIR_RAM_ADDR         0x28
#define ES9218_REG_FIR_DATA_BIT0_7      0x29
#define ES9218_REG_FIR_DATA_BIT8_15     0x2a
#define ES9218_REG_FIR_DATA_BIT16_23    0x2b
#define ES9218_REG_PROG_FIR_CONFIG      0x2c
#define ES9218_REG_CLKGEAR_CFG_BIT0_7   0x31
#define ES9218_REG_CLKGEAR_CFG_BIT8_15  0x32
#define ES9218_REG_CLKGEAR_CFG_BIT16_23 0x33
#define ES9218_REG_CHIP_ID_AND_STATUS   0x40
#define ES9218_REG_GPIO_AND_CLOCK_GEAR  0x41
#define ES9218_REG_DPLL_NUMBER_BIT0_7   0x42
#define ES9218_REG_DPLL_NUMBER_BIT8_15  0x43
#define ES9218_REG_DPLL_NUMBER_BIT16_23 0x44
#define ES9218_REG_DPLL_NUMBER_BIT24_31 0x45
#define ES9218_REG_INPUT_MUTE_STATUS    0x48
#define ES9218_REG_FIR_READ_BIT0_7      0x49
#define ES9218_REG_FIR_READ_BIT8_15     0x4a
#define ES9218_REG_FIR_READ_BIT16_23    0x4b

extern void es9218_open(void);
extern void es9218_close(void);

extern int es9218_read(int reg);
extern void es9218_write(int reg, uint8_t val);

/* GPIO pin setting callbacks */
extern void es9218_set_power_pin(int level);
extern void es9218_set_reset_pin(int level);

#endif /* __ES9218_H__ */
