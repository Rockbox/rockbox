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

#include <stdbool.h>
#include <stdint.h>

#define AUDIOHW_CAPS (FILTER_ROLL_OFF_CAP|POWER_MODE_CAP)
#define AUDIOHW_HAVE_ES9218_ROLL_OFF

#define ES9218_DIG_VOLUME_MIN   (-1275)
#define ES9218_DIG_VOLUME_MAX   0
#define ES9218_DIG_VOLUME_STEP  5

#define ES9218_AMP_VOLUME_MIN   (-240)
#define ES9218_AMP_VOLUME_MAX   0
#define ES9218_AMP_VOLUME_STEP  10

AUDIOHW_SETTING(VOLUME, "dB", 1, ES9218_DIG_VOLUME_STEP,
                ES9218_DIG_VOLUME_MIN, ES9218_DIG_VOLUME_MAX, -200)
AUDIOHW_SETTING(FILTER_ROLL_OFF, "", 0, 1, 0, 7, 0)
AUDIOHW_SETTING(POWER_MODE, "", 0, 1, 0, 1, 0)

/* Register addresses */
#define ES9218_REG_SYSTEM               0x00
#define ES9218_REG_INPUT_SEL            0x01
#define ES9218_REG_MIX_AUTOMUTE         0x02
#define ES9218_REG_ANALOG_VOL           0x03
#define ES9218_REG_AUTOMUTE_TIME        0x04
#define ES9218_REG_AUTOMUTE_LEVEL       0x05
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
#define ES9218_REG_ANALOG_OVERRIDE_1    0x2d
#define ES9218_REG_ANALOG_OVERRIDE_2    0x2e
#define ES9218_REG_ANALOG_OVERRIDE_3    0x2f
#define ES9218_REG_ANALOG_CTRL          0x30
#define ES9218_REG_CLKGEAR_CFG_BIT0_7   0x31
#define ES9218_REG_CLKGEAR_CFG_BIT8_15  0x32
#define ES9218_REG_CLKGEAR_CFG_BIT16_23 0x33
#define ES9218_REG_RESERVED_4           0x34
#define ES9218_REG_THD_COMP_C2_CH2_LO   0x35
#define ES9218_REG_THD_COMP_C2_CH2_HI   0x36
#define ES9218_REG_THD_COMP_C3_CH2_LO   0x37
#define ES9218_REG_THD_COMP_C3_CH2_HI   0x38
#define ES9218_REG_RESERVED_5           0x39
#define ES9218_REG_RESERVED_6           0x3a
#define ES9218_REG_RESERVED_7           0x3b
#define ES9218_REG_RESERVED_8           0x3c
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

enum es9218_clock_gear {
    ES9218_CLK_GEAR_1 = 0, /* CLK = XI/1 */
    ES9218_CLK_GEAR_2 = 1, /* CLK = XI/2 */
    ES9218_CLK_GEAR_4 = 2, /* CLK = XI/4 */
    ES9218_CLK_GEAR_8 = 3, /* CLK = XI/8 */
};

enum es9218_amp_mode {
    ES9218_AMP_MODE_CORE_ON = 0,
    ES9218_AMP_MODE_LOWFI = 1,
    ES9218_AMP_MODE_1VRMS = 2,
    ES9218_AMP_MODE_2VRMS = 3,
};

enum es9218_iface_role {
    ES9218_IFACE_ROLE_SLAVE = 0,
    ES9218_IFACE_ROLE_MASTER = 1,
};

enum es9218_iface_format {
    ES9218_IFACE_FORMAT_I2S = 0,
    ES9218_IFACE_FORMAT_LJUST = 1,
    ES9218_IFACE_FORMAT_RJUST = 2,
};

enum es9218_iface_bits {
    ES9218_IFACE_BITS_16 = 0,
    ES9218_IFACE_BITS_24 = 1,
    ES9218_IFACE_BITS_32 = 2,
};

enum es9218_filter_type {
    ES9218_FILTER_LINEAR_FAST = 0,
    ES9218_FILTER_LINEAR_SLOW = 1,
    ES9218_FILTER_MINIMUM_FAST = 2,
    ES9218_FILTER_MINIMUM_SLOW = 3,
    ES9218_FILTER_APODIZING_1 = 4,
    ES9218_FILTER_APODIZING_2 = 5,
    ES9218_FILTER_HYBRID_FAST = 6,
    ES9218_FILTER_BRICK_WALL = 7,
};

/* Power DAC on or off */
extern void es9218_open(void);
extern void es9218_close(void);

/* Clock controls
 *
 * - Clock gear divides the input master clock to produce the DAC's clock.
 *   Frequency can be lowered to save power when using lower sample rates.
 *
 * - NCO (numerically controller oscillator), according to the datasheet,
 *   defines the ratio between the DAC's clock and the FSR (for PCM modes,
 *   this is I2S frame clock = sample rate). In master mode it effectively
 *   controls the sampling frequency by setting the I2S frame clock output.
 *   It can also be used in slave mode, but other parts of the datasheet
 *   say contradictory things about synchronous operation in slave mode.
 *
 * - If using NCO mode and a varying MCLK input (eg. input from the SoC) then
 *   you will need to call es9218_recompute_nco() when changing MCLK in order
 *   to refresh the NCO setting.
 */
extern void es9218_set_clock_gear(enum es9218_clock_gear gear);
extern void es9218_set_nco_frequency(uint32_t fsr);
extern void es9218_recompute_nco(void);

/* Amplifier controls */
extern void es9218_set_amp_mode(enum es9218_amp_mode mode);
extern void es9218_set_amp_powered(bool en);

/* Interface selection */
extern void es9218_set_iface_role(enum es9218_iface_role role);
extern void es9218_set_iface_format(enum es9218_iface_format fmt,
                                    enum es9218_iface_bits bits);

/* Volume controls, all volumes given in units of dB/10 */
extern void es9218_set_dig_volume(int vol_l, int vol_r);
extern void es9218_set_amp_volume(int vol);

/* System mute */
extern void es9218_mute(bool muted);

/* Oversampling filter */
extern void es9218_set_filter(enum es9218_filter_type filt);

/* Automute settings */
extern void es9218_set_automute_time(int time);
extern void es9218_set_automute_level(int dB);
extern void es9218_set_automute_fast_mode(bool en);

/* DPLL bandwidth setting (knob = 0-15) */
extern void es9218_set_dpll_bandwidth(int knob);

/* THD compensation */
extern void es9218_set_thd_compensation(bool en);
extern void es9218_set_thd_coeffs(uint16_t c2, uint16_t c3);

/* Direct register read/write/update operations */
extern int es9218_read(int reg);
extern void es9218_write(int reg, uint8_t val);
extern void es9218_update(int reg, uint8_t msk, uint8_t val);

/* GPIO pin setting callbacks */
extern void es9218_set_power_pin(int level);
extern void es9218_set_reset_pin(int level);

/* XI(MCLK) getter -- supplied by the target.
 *
 * Note: when changing the supplied MCLK frequency, the NCO will need to be
 * reprogrammed for the new master clock. Call es9218_recompute_nco() to
 * force this. Not necessary if you're not using NCO mode.
 */
extern uint32_t es9218_get_mclk(void);

#endif /* __ES9218_H__ */
