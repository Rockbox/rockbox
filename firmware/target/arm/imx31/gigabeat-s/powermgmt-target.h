/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Michael Sevakis
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
#ifndef POWERMGMT_TARGET_H
#define POWERMGMT_TARGET_H

/* Can't just run this code willy-nilly. Do not allow charger engagement
 * without carefully verifying compatibility.
 *
 * Things to check:
 *   1) Charge path configuration for the PMIC.
 *   2) Correct thermistor reading
 *   3) Accurate voltage readings
 *   4) Accurate current sense for the charge path as the sense resistor may
 *      deviate from the 0.1 ohms assumed by the charge path regulator.
 */
#ifdef TOSHIBA_GIGABEAT_S
/*
 * Gigabeat S qualifications:
 * 1) Setup for dual-supply mode with separate inputs and providing USB
 *    charging capability through external components.
 * 2) Curve obtained experimentally - extreme deviation from "optimized"
 *    characteristics.
 * 3) Verified at battery terminals - no deviation from datasheet formula.
 * 4) 0.316 ohms  <=?? - verified by comparitive current readings on device
 *    with ammeter readings and measurement of on-board components.
 */
#ifndef BOOTLOADER
#define IMX31_ALLOW_CHARGING
#endif

#else
#warning This iMX31 target requires validation of charging algorithm - charging disabled
#endif

#define BATT_VTRICKLE_CHARGE        2900 /* Must charge slowly */
#define BATT_VSLOW_CHARGE           3500 /* Lower-current charge mode below
                                          * this level */
#define BATT_FULL_VOLTAGE           4161 /* Battery already topped */
#define BATT_VAUTO_RECHARGE         4100 /* When to begin another cycle */
#define BATT_USB_VAUTO_RECHARGE     4000 /* When to cycle with USB only */
#define BATT_USB_VSTOP              4140 /* When to "stop" when USB only */
#define BATT_TOO_LOW                2400 /* No battery? Short? Can't
                                             read below 2400mV. */
#define CHARGER_TOTAL_TIMER          300 /* minutes */

/* .316 ohms is closest standard value as measured in 1% tolerance - adjust
 * relative to .100 ohm which is what the PMIC is "tuned" for. */
#define ILEVEL_ADJUST_IN(I)         (100*(I) / 316)
#define ILEVEL_ADJUST_OUT(I)        (316*(I) / 100)

/* Relative draw to battery capacity - adjusted for sense resistor */
#define BATTERY_ICHARGE_COMPLETE    (505*9/100) /* 9% of nominal max output */
/* All charging modes use 4.200V for regulator */
#define BATTERY_VCHARGING           MC13783_VCHRG_4_200V
/* Slow charging - MAIN - Still below 3.5V (avoid excessive reg. dissipation) */
/* #define BATTERY_ISLOW */
/* Fast charging - MAIN */
#define BATTERY_IFAST               MC13783_ICHRG_1596MA /* 505mA */
/* Trickle charging low battery - MAIN (~10% Imax) */
#define BATTERY_ITRICKLE            MC13783_ICHRG_177MA  /*  56mA */
/* Slow charging - USB - Still below 3.5V (avoid excessive reg. dissipation) */
/* #define BATTERY_ISLOW_USB */
/* Fast charging - USB */
#define BATTERY_IFAST_USB           MC13783_ICHRG_1152MA /* 365mA */
/* Trickle charging low battery - USB (Ibat = Icccv - Idevice) */
#define BATTERY_ITRICKLE_USB        MC13783_ICHRG_532MA  /* 168mA */
/* Maintain charge - USB 500mA */
#define BATTERY_IFLOAT_USB          MC13783_ICHRG_1152MA /* 365mA */
#define BATTERY_VFLOAT_USB          MC13783_VCHRG_4_150V
/* Maintain charge - USB 100mA */
#define BATTERY_IMAINTAIN_USB       MC13783_ICHRG_266MA  /*  84mA */
#define BATTERY_VMAINTAIN_USB       MC13783_VCHRG_4_150V

/* Battery filter lengths in samples */
#define BATT_AVE_SAMPLES            32
#define ICHARGER_AVE_SAMPLES        32

void powermgmt_init_target(void);
void charging_algorithm_step(void);
void charging_algorithm_close(void);

/* Provide filtered charge current */
int battery_charge_current(void);

#define CURRENT_MAX_CHG battery_charge_current()

#endif /* POWERMGMT_TARGET_H */
