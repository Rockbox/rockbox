/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Amaury Pouly
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
#ifndef __button_imx233__
#define __button_imx233__

#include "button.h"
#include "button-target.h"
#include "lradc-imx233.h"
#include "pinctrl-imx233.h"
#include "power-imx233.h"

/* special values for .btn field */
#define IMX233_BUTTON_END   -1 /* end of the list */
#define IMX233_BUTTON_HOLD  -2 /* HOLD button */
#define IMX233_BUTTON_JACK  -3 /* JACK detect */
#define IMX233_BUTTON_VDDIO -4 /* VDDIO value */

/* values for .periph field */
#define IMX233_BUTTON_GPIO      0 /* GPIO pin */
#define IMX233_BUTTON_LRADC     1 /* LRADC channel */
#define IMX233_BUTTON_PSWITCH   2 /* PSWITCH */

/* values for the .flags field */
#define IMX233_BUTTON_INVERTED  (1 << 0) /* invert button detection */
#define IMX233_BUTTON_PULLUP    (1 << 1) /* pin needs a pullup (GPIO) */

/* values for the .op field */
#define IMX233_BUTTON_EQ    0 /* channel is equal to value up to error margin */
#define IMX233_BUTTON_GT    1 /* channel is greater than value */
#define IMX233_BUTTON_LT    2 /* channel is less than value */

/** target-defined
 * NOTE for proper operation:
 * - the table must end which a dummy entry of type IMX233_BUTTON_END
 * - default is active: high for GPIO, around value for LRADC, exact value for PSWITCH
 * - inverted flags reverses the above behaviour
 * - if lradc channel is relative, the .relative field specify a entry in the
 *   button table to which it is relative. This entry must have <0 .btn value
 *   and be of type LRADC. For example, if a channel is VDDIO relative, set
 *   .relative to IMX233_BUTTON_VDDIO and create an entry which .btn set to
 *   IMX233_BUTTON_VDDIO, channel set to VDDIO source and value set to the ideal values
 *   for which no correction is necessary. Note that the IMX233_BUTTON_VDDIO value is 
 *   defined for convenience */
struct imx233_button_map_t
{
    int btn; /* target button or magic value (END, HOLD, JACK) */
    int periph; /* GPIO, LRADC, or PSWITCH, ... */
    union
    {
        struct
        {
            int bank;
            int pin;
        }gpio;
        struct
        {
            int src; /* source channel */
            int value; /* comparison value */
            int op; /* comparison operation */
            int margin; /* error margin for equal operation (0 means default) */
            int relative; /* button to which it is relative or -1 if none */
        }lradc;
        struct
        {
            int level; /* value of the PSWITCH field: low, mid, high */
        }pswitch;
    }u;
    int flags; /* flags */
    const char *name; /* name of the button */
    /** private fields */
    bool last_val; /* last interpreted value */
    int rounds; /* how many rounds it survived (debouncing) */
    int threshold; /* round threshold for acceptance */
};

/* macros for the common cases (see below for explanation) */

#define IMX233_BUTTON_PATH_GPIO(bank_, pin_) .periph = IMX233_BUTTON_GPIO, \
    .u = {.gpio = {.bank = bank_, .pin = pin_}}
#define IMX233_BUTTON_PATH_LRADC_EX(src_, op_, val_, rel_, margin_) .periph = IMX233_BUTTON_LRADC, \
    .u = {.lradc = {.src = src_, .value = val_, .relative = rel_, .margin = margin_, \
    .op = IMX233_BUTTON_##op_}}
#define IMX233_BUTTON_PATH_LRADC_REL(src_, val_, rel_) \
    IMX233_BUTTON_PATH_LRADC_EX(src_, EQ, val_, rel_, 0)
#define IMX233_BUTTON_PATH_LRADC(src_, val_) \
    IMX233_BUTTON_PATH_LRADC_REL(src_, val_, -1)
#define IMX233_BUTTON_PATH_PSWITCH(lvl_) .periph = IMX233_BUTTON_PSWITCH, \
    .u = {.pswitch = {.level = lvl_}}
#define IMX233_BUTTON_PATH_END() .periph = -1
/* special macro for VDDIO, for convenience */
#define IMX233_BUTTON_PATH_VDDIO(val_) IMX233_BUTTON_PATH_LRADC(LRADC_SRC_VDDIO, val_)

#define IMX233_BUTTON_NAMEFLAGS1(_,name_) .name = name_
#define IMX233_BUTTON_NAMEFLAGS2(_,name_,f1)  .name = name_, .flags = IMX233_BUTTON_##f1
#define IMX233_BUTTON_NAMEFLAGS3(_,name_,f1,f2)   .name = name_, \
    .flags = IMX233_BUTTON_##f1 | IMX233_BUTTON_##f2
#define IMX233_BUTTON_NAMEFLAGS4(_,name_,f1,f2,f3)    .name = name_, \
    .flags = IMX233_BUTTON_##f1 | IMX233_BUTTON_##f2 | IMX233_BUTTON_##f3

#define IMX233_BUTTON__(btn_, path_, ...) \
    {.btn = btn_, IMX233_BUTTON_PATH_##path_, \
     __VAR_EXPAND(IMX233_BUTTON_NAMEFLAGS, dummy, __VA_ARGS__)}
#define IMX233_BUTTON_(btn_, path_, ...) \
    IMX233_BUTTON__(IMX233_BUTTON_##btn_, path_, __VA_ARGS__)
#define IMX233_BUTTON(btn_, path_, ...) \
    IMX233_BUTTON__(BUTTON_##btn_, path_, __VA_ARGS__)

#define IMX233_BUTTON_END_() \
    {.btn = IMX233_BUTTON_END}

/**
 * This header provides macro to ease definition of buttons. There three types of
 * buttons:
 * - normal buttons: BUTTON_X -> defined using IMX233_BUTTON(X, ...)
 * - special buttons IMX233_BUTTON_X -> defined using IMX233_BUTTON_(X, ...)
 * - others: X -> defined using IMX233_BUTTON__(X, ...)
 * A definition contains three mandatory parts and a fourth optional:
 * IMX233_BUTTON(btn, path_spec, name [, flags])
 * The path_spec can be of the form:
 * - GPIO(bank, pin)
 * - LRADC(src, value)
 * - LRADC_REL(src, value, rel_idx)
 * - LRADC_EX(src, op, val, rel_idx, margin) where op must one of EQ, GT, LT
 *     and margin is the error margin for EQ (use 0 for default value). Operation
 *     EQ means the src value must match val within error margin. Operation GT
 *     (resp. LT) means src value must greater (resp. lower) than val.
 * - PSWITCH(level)
 * - END()
 * The optional flags are specified as a list, without the IMX233_BUTTON_ prefix.
 * Examples:
 * - BUTTON_POWER named "power", mapped to PSWITCH mid level, no flags:
 *   IMX233_BUTTON(POWER, PSWICTH(1), "power")
 * - IMX233_BUTTON_JACK named "jack", mapped to GPIO B2P17 inverted and pullup:
 *   IMX233_BUTTON_(JACK, GPIO(2, 17), "jack", INVERTED, PULLUP)
 * - BIZARRE named "bizarre", mapped to LRADC channel 0, expected value 42:
 *   IMX233_BUTTON__(BIZARRE, LRADC(0, 42), "bizarre")
 * - BUTTON_PLAY named "play", mapped to LRADc channel 2, expected value 300 related
 *   to button entry IDX
 *   IMX233_BUTTON(PLAY, LRADC_REL(2, 300, IDX), "play")
 * - VDDIO entry, expected value 4000 (for relative entries):
 *   IMX233_BUTTON_(VDDIO, VDDIO(4000), "vddio")
 * - last entry:
 *   IMX233_BUTTON_(END, END(), "")
 * 
 * The driver also provides an implementations for headphones_inserted()
 * and button_hold() using the table information.
 *
 * The button-target.h header can also define IMX233_BUTTON_LRADC_MARGIN
 * to control the error margin allowed for button using LRADC. The default
 * margin is 30. Obviously, the margin should be less than M/2 where M is the
 * minimum LRADC value between two buttons. The margin can be overriden one a
 * per button basis using LRADC_EX.
 */

extern struct imx233_button_map_t imx233_button_map[];

/* initialise button driver */
void imx233_button_init(void);
/* others gives the bitmask of other buttons: the function will OR the result
 * with them except if hold is detected in which case 0 is always returned */
int imx233_button_read(int others);
#ifdef HAS_BUTTON_HOLD
bool imx233_button_read_hold(void);
bool button_hold(void);
#endif
#ifdef HAVE_HEADPHONE_DETECTION
bool imx233_button_read_jack(void);
bool headphones_inserted(void);
#endif
/* return interpreted button value, after processing.
 * Argument is an index into the imx233_button_map table */
bool imx233_button_read_btn(int idx);
/* return raw value for a button, before any processing: GPIO state, LRADC
 * channel value, PSWITCH value. Return -1 if there is no match.
 * Argument is an index into the imx233_button_map table */
int imx233_button_read_raw(int idx);

#endif /* __button_imx233__ */
