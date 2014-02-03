/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
#ifndef __button_lradc_imx233__
#define __button_lradc_imx233__

#include "button.h"
#include "button-target.h"

/** This driver implements button sensing with lradc. It does debouncing
 * and can handle hold sensing too. It can be tweaked using the following defines
 * and variables:
 * - imx233_button_lradc_mapping: target-defined table of adc values and mapping
 * - IMX233_BUTTON_LRADC_CHANNEL: lradc channel to use
 * - IMX233_BUTTON_LRADC_HOLD_DET: define hold detection method (ignored if !HAS_BUTTON_HOLD)
 * - IMX233_BUTTON_LRADC_MODE: define the button lradc mode
 *
 * The LRADC code supports two modes of operations: VDDIO relative or absolute.
 * In the (default) absolute value mode, the LRADC channel is sampled and its value
 * is compared to the one in the imx233_button_lradc_mapping table. This is
 * appropriate when the resistor ladder is derived from a fixed voltage.
 * In the VDDIO relative mode, the values in imx233_button_lradc_mapping are
 * the values for a specific value of VDDIO which is given by
 * IMX233_BUTTON_LRADC_VDDIO. In this mode, the code will also sample VDDIO
 * and do the following comparison:
 *      lradc_value <=? imx233_button_lradc_mapping[i] * vddio_ref / vddio_value
 * where vddio_ref is IMX233_BUTTON_LRADC_VDDIO.
 * 
 * The available values of IMX233_BUTTON_LRADC_HOLD are:
 * - BLH_ADC: detect hold using adc
 * - BLH_EXT: target button driver implements imx233_button_lradc_hold() using
 *   any external method of its choice
 * - BLH_GPIO: detect hold using a GPIO, needs to define additional defines:
 *   + BLH_GPIO_BANK: pin bank
 *   + BLH_GPIO_PIN: pin in bank
 *   + BLH_GPIO_INVERTED: define if inverted, default is active high
 *   + BLH_GPIO_PULLUP: define if pins needs pullup
 *
 * WARNING
 * There must always be entry in imx233_button_lradc_mapping whose value is the steady
 * value of the channel when no button is pressed, and which maps to no button (.btn = 0)
 */

/* hold detect method */
#define BLH_ADC     0
#define BLH_EXT     1
#define BLH_GPIO    2

/* special value for btn to indicate end of list */
#define IMX233_BUTTON_LRADC_END     -1
/* special value for btn to handle HOLD */
#define IMX233_BUTTON_LRADC_HOLD    -2

struct imx233_button_lradc_mapping_t
{
    int adc_val;
    int btn;
};

/* target-defined
 * NOTE for proper operation, the table should contain entries for all states, including:
 * - one with adc_val set to default value when no button is pressed
 * - one with adc_val set to value when hold is selected (if hold is sensed using adc)
 * - a dummy entry with btn set to IMX233_BUTTON_LRADC_END as the last entry
 * The table must be sorted by increasing values of adc_val */
extern struct imx233_button_lradc_mapping_t imx233_button_lradc_mapping[];

void imx233_button_lradc_init(void);
/* others gives the bitmask of other buttons: the function will OR the result
 * with them except if hold is detected in which case 0 is always returned */
int imx233_button_lradc_read(int others);
#ifdef HAS_BUTTON_HOLD
bool imx233_button_lradc_hold(void);
#endif
int imx233_button_lradc_read_raw(void); // return raw adc value
#ifdef IMX233_BUTTON_LRADC_VDDIO
int imx233_button_lradc_read_vddio(void);
#endif

#endif /* __button_lradc_imx233__ */
