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
 */

/* special value for btn to handle HOLD */
#define IMX233_BUTTON_LRADC_END     -1
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
int imx233_button_lradc_read(void);
bool imx233_button_lradc_hold(void);

#endif /* __button_lradc_imx233__ */
