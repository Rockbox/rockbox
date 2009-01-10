/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2008 by Michael Sevakis
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
#include <stdlib.h>
#include "config.h"
#include "system.h"
#include "thread.h"
#include "mc13783.h"
#include "adc.h"
#include "powermgmt.h"
#include "power.h"
#include "power-imx31.h"

/* TODO: Battery tests to get the right values! */
const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3450
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3400
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    /* Toshiba Gigabeat Li Ion 830mAH figured from discharge curve */
    { 3480, 3550, 3590, 3610, 3630, 3650, 3700, 3760, 3800, 3910, 3990 },
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    /* Toshiba Gigabeat Li Ion 830mAH */
    3480, 3550, 3590, 3610, 3630, 3650, 3700, 3760, 3800, 3910, 3990
};

/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    /* ADC reading 0-1023 = 2400mV-4700mV */
    return ((adc_read(ADC_BATTERY) * 2303) >> 10) + 2400;
}

/* Returns the application supply voltage from ADC [millvolts] */
unsigned int application_supply_adc_voltage(void)
{
    return ((adc_read(ADC_APPLICATION_SUPPLY) * 2303) >> 10) + 2400;
}

unsigned int chrgraw_adc_voltage(void)
{
    return (adc_read(ADC_CHARGER_VOLTAGE) * 23023) >> 10;
}

/* Returns battery charge current from ADC [milliamps] */
int battery_adc_charge_current(void)
{
    /* Positive reading = charger to battery
     * Negative reading = battery to charger terminal
     * ADC reading -512-511 = -2875mA-2875mA */
    unsigned int value = adc_read(ADC_CHARGER_CURRENT);
    int I;

    if (value == ADC_READ_ERROR)
        return INT_MIN;

    I = ((((int32_t)value << 22) >> 22) * 2881) >> 9;
    return ILEVEL_ADJUST_IN(I);
}

/* Estimate power dissipation in the charge path regulator in mW. */
unsigned int cccv_regulator_dissipation(void)
{
    /* BATTISNS is shorted to BATT so we don't need to use the
     * battery current reading. */
    int chrgraw = (adc_read(ADC_CHARGER_VOLTAGE) * 230225) >> 10;
    int batt = ((adc_read(ADC_BATTERY) * 23023) >> 10) + 24000;
    int ichrgsn = adc_read(ADC_CHARGER_CURRENT);
    ichrgsn = ((((int32_t)ichrgsn << 22) >> 22) * 2881) >> 9;
    ichrgsn = abs(ichrgsn);

    return (chrgraw - ichrgsn - batt)*ILEVEL_ADJUST_IN(ichrgsn) / 10000;
}

/* Returns battery temperature from ADC [deg-C] */
int battery_adc_temp(void)
{
    /* E[volts] = value * 2.3V / 1023
     * R[ohms] = E/I = E[volts] / 0.00002[A] (Thermistor bias current source)
     *
     * Steinhart-Hart thermistor equation:
     * [A + B*ln(R) + D*ln^3(R)] = 1 / T[°K]
     *
     * Coeffients that fit experimental data (one thermistor so far, one run):
     * A = 0.0013002631685462800
     * B = 0.0002000841932612330
     * D = 0.0000000640446750919
     */
    static const unsigned short ntc_table[] =
    {
#if 0   /* These have degree deltas > 1  (except the final two) so leave them
         * out. 70 deg C upper limit is quite sufficient. */
           0, /* INF */   1, /* 171 */   2, /* 145 */   3, /* 130 */
           4, /* 121 */   5, /* 114 */   6, /* 108 */   7, /* 104 */
           8, /* 100 */   9, /*  96 */  10, /*  93 */  11, /*  91 */
          12, /*  88 */  13, /*  86 */  14, /*  84 */  15, /*  82 */
          16, /*  81 */  17, /*  79 */  18, /*  78 */  19, /*  76 */
          20, /*  75 */  21, /*  74 */  22, /*  72 */  23, /*  71 */
#endif
          24, /*  70 */  25, /*  69 */  26, /*  68 */  27, /*  67 */
          28, /*  66 */  30, /*  65 */  31, /*  64 */  32, /*  63 */
          33, /*  62 */  35, /*  61 */  36, /*  60 */  38, /*  59 */
          39, /*  58 */  41, /*  57 */  43, /*  56 */  45, /*  55 */
          47, /*  54 */  49, /*  53 */  51, /*  52 */  53, /*  51 */
          56, /*  50 */  58, /*  49 */  61, /*  48 */  63, /*  47 */
          66, /*  46 */  69, /*  45 */  73, /*  44 */  76, /*  43 */
          80, /*  42 */  83, /*  41 */  87, /*  40 */  92, /*  39 */
          96, /*  38 */ 101, /*  37 */ 106, /*  36 */ 111, /*  35 */
         116, /*  34 */ 122, /*  33 */ 128, /*  32 */ 135, /*  31 */
         142, /*  30 */ 149, /*  29 */ 156, /*  28 */ 164, /*  27 */
         173, /*  26 */ 182, /*  25 */ 192, /*  24 */ 202, /*  23 */
         212, /*  22 */ 224, /*  21 */ 236, /*  20 */ 249, /*  19 */
         262, /*  18 */ 277, /*  17 */ 292, /*  16 */ 308, /*  15 */
         325, /*  14 */ 344, /*  13 */ 363, /*  12 */ 384, /*  11 */
         406, /*  10 */ 429, /*   9 */ 454, /*   8 */ 480, /*   7 */
         509, /*   6 */ 539, /*   5 */ 571, /*   4 */ 605, /*   3 */
         641, /*   2 */ 680, /*   1 */ 722, /*   0 */ 766, /*  -1 */
         813, /*  -2 */ 864, /*  -3 */ 918, /*  -4 */ 976, /*  -5 */
    };

    unsigned int value = adc_read(ADC_BATTERY_TEMP);

    if (value > 0)
    {
        unsigned i;

        for (i = 1; i < ARRAYLEN(ntc_table); i++)
        {
            if (ntc_table[i] > value)
                break;
        }

        return 71 - i;
    }

    return INT_MIN;
}

/** Charger control **/

/* All code has a preference for the main charger being connected over
 * USB. USB is considered in the algorithm only if it is the sole source. */
static uint32_t int_sense0 = 0;      /* Interrupt Sense 0 bits */
static unsigned int last_inputs = POWER_INPUT_NONE; /* Detect input changes */
static int charger_total_timer = 0;  /* Total allowed charging time */
static int icharger_ave = 0;         /* Filtered charging current */
static bool charger_close = false;   /* Shutdown notification */
static bool service_wdt = true;      /* Service the watchdog timer, if things
                                        go very wrong, cease and shut down. */
static uint32_t charger_setting = 0; /* Current ICHRG and VCHRG regulator
                                      * setting (register bits) */
#define CHARGER_ADJUST ((uint32_t)-1)/* Force change in regulator setting */
static int autorecharge_counter = 0 ; /* Battery < threshold debounce */
static int chgcurr_timer = 0;        /* Countdown to CHGCURR error */
#define AUTORECHARGE_COUNTDOWN (10*2) /* 10s debounce */
#define WATCHDOG_TIMEOUT       (10*2) /* If not serviced, poweroff in 10s */
#define CHGCURR_TIMEOUT        (4*2)  /* 4s debounce */

/* Temperature monitoring */
static enum
{
    TEMP_STATE_NORMAL = 0, /* Within range */
    TEMP_STATE_WAIT = 1,   /* Went out of range, wait to come back */
    TEMP_LOW_LIMIT = 0,    /* Min temp */
    TEMP_HIGH_LIMIT = 1,   /* Max temp */
} temp_state = TEMP_STATE_NORMAL;

/* Set power thread priority for charging mode or not */
static inline void charging_set_thread_priority(bool charging)
{
#ifdef HAVE_PRIORITY_SCHEDULING
    thread_set_priority(THREAD_ID_CURRENT,
                        charging ? PRIORITY_REALTIME : PRIORITY_SYSTEM);
#endif
    (void)charging;
}

/* Update filtered charger current - exponential moving average */
static bool charger_current_filter_step(void)
{
    int value = battery_adc_charge_current();

    if (value == ADC_READ_ERROR)
        return false;

    icharger_ave += value - (icharger_ave / ICHARGER_AVE_SAMPLES);
    return true;
}

/* Return true if the main charger is connected. */
static bool main_charger_connected(void)
{
    return (last_inputs &
            POWER_INPUT_MAIN_CHARGER &
            POWER_INPUT_CHARGER) != 0;
}

/* Return the voltage level which should automatically trigger
 * another recharge cycle based upon which power source is available.
 * Assumes at least one is. */
static unsigned int auto_recharge_voltage(void)
{
    if (main_charger_connected())
        return BATT_VAUTO_RECHARGE;
    else
        return BATT_USB_VAUTO_RECHARGE;
}

/* Return greater of supply (BP) or filtered battery voltage. */
unsigned int input_millivolts(void)
{
    unsigned int app_millivolts = application_supply_adc_voltage();
    unsigned int bat_millivolts = battery_voltage();

    return MAX(app_millivolts, bat_millivolts);
}

/* Get smoothed readings for initializing filtered data. */
static int stat_battery_reading(int type)
{
    int high = INT_MIN, low = INT_MAX;
    int value = 0;
    int i;

    for (i = 0; i < 7; i++)
    {
        int reading = ADC_READ_ERROR;

        sleep(2); /* Get unique readings */

        switch (type)
        {
        case ADC_BATTERY:
            reading = battery_adc_voltage();
            break;

        case ADC_CHARGER_CURRENT:
            reading = battery_adc_charge_current();
            break;
        }

        if (reading == ADC_READ_ERROR)
            return INT_MIN;

        if (reading > high)
            high = reading;

        if (reading < low)
            low = reading;

        value += reading;
    }

    /* Discard extremes */
    return (value - high - low) / 5;
}

/* Update filtered battery voltage instead of waiting for filter
 * decay. */
static bool update_filtered_battery_voltage(void)
{
    int millivolts = stat_battery_reading(ADC_BATTERY);

    if (millivolts != INT_MIN)
    {
        reset_battery_filter(millivolts);
        return true;
    }

    return false;
}

/* Sets the charge current limit based upon state. charge_state should be
 * set before calling. */
static bool adjust_charger_current(void)
{
    static const uint8_t charger_bits[][2] =
    {
        [DISCHARGING] =
        {
            /* These are actually zeros but reflect this setting */
            MC13783_ICHRG_0MA | MC13783_VCHRG_4_050V,
            MC13783_ICHRG_0MA | MC13783_VCHRG_4_050V,
        },
        /* Main(+USB): Charge slowly from the adapter until voltage is
         * sufficient for normal charging.
         *
         * USB: The truth is that things will probably not make it this far.
         * Cover the case, just in case the disk isn't used and it is
         * manageable. */
        [TRICKLE] =
        {
            BATTERY_ITRICKLE | BATTERY_VCHARGING,
            BATTERY_ITRICKLE_USB | BATTERY_VCHARGING
        },
        [TOPOFF] =
        {
            BATTERY_IFAST | BATTERY_VCHARGING,
            BATTERY_IFAST_USB | BATTERY_VCHARGING
        },
        [CHARGING] =
        {
            BATTERY_IFAST | BATTERY_VCHARGING,
            BATTERY_IFAST_USB | BATTERY_VCHARGING
        },
        /* Must maintain battery when on USB power only - utterly nasty
         * but true and something retailos does (it will even end up charging
         * the battery but not reporting that it is doing so).
         * Float lower than MAX - could end up slightly discharging after
         * a full charge but this is safer than maxing it out. */
        [CHARGING+1] =
        {
            BATTERY_IFLOAT_USB | BATTERY_VFLOAT_USB,
            BATTERY_IMAINTAIN_USB | BATTERY_VMAINTAIN_USB
        },
#if 0
        /* Slower settings to so that the linear regulator doesn't dissipate
         * an excessive amount of power when coming out of precharge state. */
        [CHARGING+2] =
        {
            BATTERY_ISLOW | BATTERY_VCHARGING,
            BATTERY_ISLOW_USB | BATTEYR_VCHARGING
        },
#endif
    };

    bool success = false;
    int usb_select;
    uint32_t i;

    usb_select = ((last_inputs & POWER_INPUT) == POWER_INPUT_USB)
                    ? 1 : 0;

    if (charge_state == DISCHARGING && usb_select == 1)
    {
        /* USB-only, DISCHARGING, = maintaining battery */
        int select = (last_inputs & POWER_INPUT_CHARGER) ? 0 : 1;
        charger_setting = charger_bits[CHARGING+1][select];
    }
    else
    {
        /* Take very good care not to write garbage. */
        int state = charge_state;

        if (state < DISCHARGING || state > CHARGING)
            state = DISCHARGING;

        charger_setting = charger_bits[state][usb_select];
    }

    if (charger_setting != 0)
    {
        charging_set_thread_priority(true);

        /* Turn regulator logically ON. Hardware may still override. */
        i = mc13783_write_masked(MC13783_CHARGER,
                                 charger_setting | MC13783_CHRGRAWPDEN,
                                 MC13783_ICHRG | MC13783_VCHRG |
                                 MC13783_CHRGRAWPDEN);

        if (i != MC13783_DATA_ERROR)
        {
            int icharger;

            /* Enable charge current conversion */
            adc_enable_channel(ADC_CHARGER_CURRENT, true);

            /* Charge path regulator turn on takes ~100ms max. */
            sleep(HZ/10);

            icharger = stat_battery_reading(ADC_CHARGER_CURRENT);

            if (icharger != INT_MIN)
            {
                icharger_ave = icharger * ICHARGER_AVE_SAMPLES;

                if (update_filtered_battery_voltage())
                    return true;
            }
        }

        /* Force regulator OFF. */
        charge_state = CHARGE_STATE_ERROR;
    }

    /* Turn regulator OFF. */
    icharger_ave = 0;
    i = mc13783_write_masked(MC13783_CHARGER, charger_bits[0][0],
                             MC13783_ICHRG | MC13783_VCHRG |
                             MC13783_CHRGRAWPDEN);

    if (MC13783_DATA_ERROR == i)
    {
        /* Failed. Force poweroff by not servicing the watchdog. */
        service_wdt = false;
    }
    else if (0 == charger_setting)
    {
        /* Here because OFF was requested state */
        success = true;
    }

    charger_setting = 0;

    adc_enable_channel(ADC_CHARGER_CURRENT, false);
    update_filtered_battery_voltage();
    charging_set_thread_priority(false);

    return success;
}

/* Stop the charger - if USB only then the regulator will not really be
 * turned off. ERROR or DISABLED will turn it off however. */
static void stop_charger(void)
{
    charger_total_timer = 0;

    if (charge_state > DISCHARGING)
        charge_state = DISCHARGING;

    adjust_charger_current();
}

/* Return OK if it is acceptable to start the regulator. */
static bool charging_ok(void)
{
    bool ok = charge_state >= DISCHARGING; /* Not an error condition? */

    if (ok)
    {
        /* Is the battery even connected? */
        ok = (last_inputs & POWER_INPUT_BATTERY) != 0;
    }

    if (ok)
    {
        /* No tolerance for any over/under temp - wait for it to
         * come back into safe range. */
        static const signed char temp_ranges[2][2] =
        {
            { 0, 45 }, /* Temperature range before beginning charging */
            { 5, 40 }, /* Temperature range after out-of-range detected */
        };

        int temp = battery_adc_temp();
        const signed char *range = temp_ranges[temp_state];

        ok = temp >= range[TEMP_LOW_LIMIT] &&
             temp <= range[TEMP_HIGH_LIMIT];

        switch (temp_state)
        {
        case TEMP_STATE_NORMAL:
            if (!ok)
                temp_state = TEMP_STATE_WAIT;
            break;

        case TEMP_STATE_WAIT:
            if (ok)
                temp_state = TEMP_STATE_NORMAL;
            break;

        default:
            break;
        }
    }

    if (ok)
    {
        /* Any events that should stop the regulator? */

        /* Overvoltage at CHRGRAW? */
        ok = (int_sense0 & MC13783_CHGOVS) == 0;

        if (ok)
        {
            /* CHGCURR sensed? */
            ok = (int_sense0 & MC13783_CHGCURRS) != 0;

            if (!ok)
            {
                /* Debounce transient states */
                if (chgcurr_timer > 0)
                {
                    chgcurr_timer--;
                    ok = true;
                }
            }
            else
            {
                chgcurr_timer = CHGCURR_TIMEOUT;
            }
        }

        /* Charger may need to be reinserted */
        if (!ok)
            charge_state = CHARGE_STATE_ERROR;
    }

    if (charger_setting != 0)
    {
        if (ok)
        {
            /* Watch to not overheat FET (nothing should go over about 1012.7mW).
             * Trying a higher voltage AC adapter can work (up to 6.90V) but
             * we'll just reject that. Reducing current for adapters that bring
             * CHRGRAW to > 4.900V is another possible action. */
            ok = cccv_regulator_dissipation() < 1150;
            if (!ok)
                charge_state = CHARGE_STATE_ERROR;
        }

        if (!ok)
        {
            int state = charge_state;

            if (state > DISCHARGING)
                state = DISCHARGING;

            /* Force off for all states including maintaining the battery level
             * on USB. */
            charge_state = CHARGE_STATE_ERROR;
            stop_charger();
            charge_state = state;
        }
    }

    return ok;
}

void powermgmt_init_target(void)
{
#ifdef IMX31_ALLOW_CHARGING
    const uint32_t regval_w =
        MC13783_VCHRG_4_050V | MC13783_ICHRG_0MA |
        MC13783_ICHRGTR_0MA | MC13783_OVCTRL_6_90V;

    /* Use watchdog to shut system down if we lose control of the charging
     * hardware. */
    watchdog_init(WATCHDOG_TIMEOUT);

    mc13783_write(MC13783_CHARGER, regval_w);

    if (mc13783_read(MC13783_CHARGER) == regval_w)
    {
        /* Divide CHRGRAW input by 10 */
        mc13783_clear(MC13783_ADC0, MC13783_CHRGRAWDIV);
        /* Turn off BATTDETB. It's worthless on MESx0V since the battery
         * isn't removable (nor the thermistor). */
        mc13783_clear(MC13783_POWER_CONTROL0, MC13783_BATTDETEN);
    }
    else
    {
        /* Register has the wrong value - set error condition and disable
         * since something is wrong. */
        charge_state = CHARGE_STATE_DISABLED;
        stop_charger();
    }
#else
    /* Disable charger use */
    charge_state = CHARGE_STATE_DISABLED;
#endif
}

/* Returns true if the unit is charging the batteries. */
bool charging_state(void)
{
    switch (charge_state)
    {
    case TRICKLE:
    case TOPOFF:
    case CHARGING:
        return true;
    default:
        return false;
    }
}

/* Filtered battery charge current */
int battery_charge_current(void)
{
    return icharger_ave / ICHARGER_AVE_SAMPLES;
}

static void charger_plugged(void)
{
    adc_enable_channel(ADC_BATTERY_TEMP, true);
    autorecharge_counter = -1;
}

static void charger_unplugged(void)
{
    /* Charger pulled - turn off current sources (though hardware
     * will have done that anyway). */
    if (charge_state > CHARGE_STATE_DISABLED)
    {
        /* Reset state and clear any error. If disabled, the charger
         * will not have been started or will have been stopped already. */
        stop_charger();
        charge_state = DISCHARGING;
    }

    /* Might need to reevaluate these bits in charger_none. */
    last_inputs &= ~(POWER_INPUT | POWER_INPUT_CHARGER);
    temp_state = TEMP_STATE_NORMAL;
    autorecharge_counter = 0;
    chgcurr_timer = 0;

    adc_enable_channel(ADC_BATTERY_TEMP, false);
}

static void charger_none(void)
{
    unsigned int pwr = power_thread_inputs;

    if (last_inputs != pwr)
    {
        last_inputs = pwr;

        if (charge_state == CHARGE_STATE_DISABLED)
            return;

        if ((pwr & (POWER_INPUT | POWER_INPUT_CHARGER)) == POWER_INPUT_USB)
        {
            /* USB connected but not configured. Maintain battery to the
             * greatest degree possible. It probably won't be enough but the
             * discharge won't be so severe. */
            charger_plugged();
            charger_setting = CHARGER_ADJUST;
        }
        else
        {
            charger_unplugged();
            last_inputs = pwr; /* Restore status */
        }
    }
    else if (charger_setting != 0)
    {
        /* Maintaining - keep filter going and check charge state */
        int_sense0 = mc13783_read(MC13783_INTERRUPT_SENSE0);

        if (!charger_current_filter_step())
        {
            /* Failed to read current */
            charge_state = CHARGE_STATE_ERROR;
        }

        charging_ok();
    }
}

static void charger_control(void)
{
    unsigned int pwr = power_thread_inputs;

    if (last_inputs != pwr)
    {
        unsigned int changed = last_inputs ^ pwr;

        last_inputs = pwr;

        if (charger_setting != 0)
            charger_setting = CHARGER_ADJUST;

        if (charge_state == DISCHARGING)
        {
            if (main_charger_connected())
            {
                /* If main is connected, ignore USB plugs. */
                if (changed & POWER_INPUT_MAIN_CHARGER)
                {
                    /* Main charger plugged - try charge */
                    autorecharge_counter = -1;
                }
            }
            else if (pwr & POWER_INPUT_USB_CHARGER
                        & POWER_INPUT_CHARGER)
            {
                /* USB power only */
                if (changed & POWER_INPUT_USB_CHARGER)
                {
                    /* USB charger plugged - try charge */
                    autorecharge_counter = -1;
                }
                else if (changed & POWER_INPUT_MAIN_CHARGER)
                {
                    /* Main charger pulled - go to battery maintenence. */
                    charger_setting = CHARGER_ADJUST;
                }
            }
        }
    }

    if (charger_setting != 0 && !charger_current_filter_step())
    {
        /* Failed to read current */
        charge_state = CHARGE_STATE_ERROR;
    }

    int_sense0 = mc13783_read(MC13783_INTERRUPT_SENSE0);

    if (!charging_ok())
        return;

    switch (charge_state)
    {
    case DISCHARGING:
    {
        /* Battery voltage may have dropped and a charge cycle should
         * start again. Debounced. */
        if (autorecharge_counter < 0 &&
            battery_adc_voltage() < BATT_FULL_VOLTAGE)
        {
            /* Try starting a cycle now if battery isn't already topped
             * off to allow user to ensure the battery is full. */
        }
        else if (battery_voltage() > auto_recharge_voltage())
        {
            /* Still above threshold - reset counter */
            autorecharge_counter = AUTORECHARGE_COUNTDOWN;
            break;
        }
        else if (autorecharge_counter > 0)
        {
            /* Coundown to restart */
            autorecharge_counter--;
            break;
        }

        autorecharge_counter = 0;

        charging_set_thread_priority(true);

        if (stat_battery_reading(ADC_BATTERY) < BATT_VTRICKLE_CHARGE)
        {
            /* Battery is deeply discharged - precharge at lower current. */
            charge_state = TRICKLE;
        }
        else
        {
            /* Ok for fast charge */
            charge_state = CHARGING;
        }

        charger_setting = CHARGER_ADJUST;
        charger_total_timer = CHARGER_TOTAL_TIMER*60*2;
        break;
        } /* DISCHARGING: */

    case TRICKLE: /* Very low - precharge */
    {
        if (battery_voltage() <= BATT_VTRICKLE_CHARGE)
            break;

        /* Switch to normal charge mode. */
        charge_state = CHARGING;
        charger_setting = CHARGER_ADJUST;
        break;
        } /* TRICKLE: */

    case CHARGING: /* Constant-current stage */
    case TOPOFF:   /* Constant-voltage stage */
    {
        /* Reg. mode is more informative than an operational necessity. */
        charge_state = (int_sense0 & MC13783_CCCVS) ? TOPOFF : CHARGING;

        if (main_charger_connected())
        {
            /* Monitor and stop if current drops below threshold. */
            if (battery_charge_current() > BATTERY_ICHARGE_COMPLETE)
                break;
        }
        else
        {
            /* Accurate I-level can't be determined since device also
             * powers through the I sense. This simply stops the reporting
             * of charging but the regulator remains on. */
            if (battery_voltage() <= BATT_USB_VSTOP)
                break;
        }

        stop_charger();
        break;
        } /* CHARGING: TOPOFF: */

    default:
        break;
    } /* switch */

    /* Check if charger timer expired and stop it if so. */
    if (charger_total_timer > 0 && --charger_total_timer == 0)
    {
        charge_state = CHARGE_STATE_ERROR;
        stop_charger(); /* Time ran out - error */
    }
}

/* Main charging algorithm - called from powermgmt.c */
void charging_algorithm_step(void)
{
#ifdef IMX31_ALLOW_CHARGING
    if (service_wdt)
        watchdog_service();
#endif

    /* Switch by input state */
    switch (charger_input_state)
    {
    case NO_CHARGER:
        charger_none();
        break;

    case CHARGER_PLUGGED:
        charger_plugged();
        break;

    case CHARGER:
        charger_control();
        break;

    case CHARGER_UNPLUGGED:
        charger_unplugged();
        break;
    } /* switch */

    if (charger_close)
    {
        if (charge_state != CHARGE_STATE_DISABLED)
        {
            /* Disable starts while shutting down */
            charge_state = CHARGE_STATE_DISABLED;
            stop_charger();
        }

        charger_close = false;
        return;
    }

    if (charger_setting != 0)
    {
        if ((mc13783_read(MC13783_CHARGER) & (MC13783_ICHRG | MC13783_VCHRG)) !=
            charger_setting)
        {
            /* The hardware setting doesn't match. It could have turned the
             * charger off in a race of plugging/unplugging or the setting
             * was changed in one of the calls. */
            adjust_charger_current();
        }
    }
}

/* Disable the charger and prepare for poweroff - called off-thread so we
 * signal the charging thread to prepare to quit. */
void charging_algorithm_close(void)
{
    charger_close = true;

    /* Power management thread will set it false again */
    while (charger_close)
        sleep(HZ/10);
}
