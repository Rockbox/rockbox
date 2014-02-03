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
#include "button-target.h"
#include "system.h"
#include "system-target.h"
#include "pinctrl-imx233.h"
#include "power-imx233.h"
#include "button-lradc-imx233.h"

#if defined(CREATIVE_ZENXFI) || defined(CREATIVE_ZENMOZAIC)
#define JACK_DET_BANK   2
#define JACK_DET_PIN    8
#define JACK_DET_INVERTED
#elif defined(CREATIVE_ZENXFISTYLE)
#define JACK_DET_BANK   2
#define JACK_DET_PIN    7
#endif

struct imx233_button_lradc_mapping_t imx233_button_lradc_mapping[] =
{
#if defined(CREATIVE_ZEN)
    {200, IMX233_BUTTON_LRADC_HOLD},
    {250, BUTTON_MENU},
    {520, BUTTON_SHORTCUT},
    {1490, BUTTON_UP},
    {1740, BUTTON_SELECT},
    {2015, BUTTON_LEFT},
    {2255, BUTTON_RIGHT},
    {2485, BUTTON_DOWN},
    {2700, BUTTON_BACK},
    {2945, BUTTON_PLAYPAUSE},
    {3400, 0},
    {0, IMX233_BUTTON_LRADC_END},
#elif defined(CREATIVE_ZENXFI)
    {0, IMX233_BUTTON_LRADC_HOLD},
    {200, BUTTON_MENU},
    {445, BUTTON_SHORTCUT},
    {645, BUTTON_UP},
    {860, BUTTON_LEFT},
    {1060, BUTTON_RIGHT},
    {1260, BUTTON_DOWN},
    {1480, BUTTON_SELECT},
    {1700, BUTTON_TOPRIGHT},
    {1920, BUTTON_BOTTOMLEFT},
    {2145, BUTTON_TOPLEFT},
    {2460, BUTTON_BOTTOMRIGHT},
    {2700, BUTTON_BACK},
    {2945, BUTTON_PLAYPAUSE},
    {3400, 0},
    {0, IMX233_BUTTON_LRADC_END},
#elif defined(CREATIVE_ZENV)
    {190, IMX233_BUTTON_LRADC_HOLD},
    {250, BUTTON_PLAYPAUSE},
    {530, BUTTON_BACK},
    {785, BUTTON_VOL_UP},
    {1040, BUTTON_VOL_DOWN},
    {1295, BUTTON_MENU},
    {1540, BUTTON_UP},
    {1800, BUTTON_SELECT},
    {2070, BUTTON_LEFT},
    {2315, BUTTON_RIGHT},
    {2550, BUTTON_DOWN},
    {3450, 0},
    {0, IMX233_BUTTON_LRADC_END},
#elif defined(CREATIVE_ZENMOZAIC)
    {0, IMX233_BUTTON_LRADC_HOLD},
    {200, BUTTON_MENU},
    {445, BUTTON_SHORTCUT},
    {645, BUTTON_UP},
    {860, BUTTON_LEFT},
    {1060, BUTTON_RIGHT},
    {1260, BUTTON_DOWN},
    {1480, BUTTON_SELECT},
    {2700, BUTTON_BACK},
    {2945, BUTTON_PLAYPAUSE},
    {3400, 0},
    {0, IMX233_BUTTON_LRADC_END},
#elif defined(CREATIVE_ZENXFISTYLE)
    {230, BUTTON_MENU},
    {480, BUTTON_SHORTCUT},
    {690, BUTTON_UP},
    {920, BUTTON_LEFT},
    {1120, BUTTON_RIGHT},
    {1335, BUTTON_DOWN},
    {1565, BUTTON_SELECT},
    {2850, BUTTON_BACK},
    {3110, BUTTON_PLAYPAUSE},
    {3620, 0},
    {0, IMX233_BUTTON_LRADC_END},
#else
#error wrong target
#endif
};

void button_init_device(void)
{
    imx233_button_lradc_init();
#ifdef HAVE_HEADPHONE_DETECTION
    imx233_pinctrl_acquire(JACK_DET_BANK, JACK_DET_PIN, "jack_detect");
    imx233_pinctrl_set_function(JACK_DET_BANK, JACK_DET_PIN, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(JACK_DET_BANK, JACK_DET_PIN, false);
#endif
#ifdef CREATIVE_ZENXFISTYLE
    imx233_pinctrl_acquire(0, 11, "power_detect");
    imx233_pinctrl_set_function(0, 11, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(0, 11, false);
#endif
}

#ifdef HAS_BUTTON_HOLD
bool button_hold(void)
{
    return imx233_button_lradc_hold();
}
#endif

#ifdef HAVE_HEADPHONE_DETECTION
bool headphones_inserted(void)
{
    bool det = imx233_pinctrl_get_gpio(JACK_DET_BANK, JACK_DET_PIN);
#ifdef JACK_DET_INVERTED
    det = !det;
#endif
    return det;
}
#endif

int button_read_device(void)
{
    int btn = 0;
#ifdef CREATIVE_ZENXFISTYLE
    /* The ZEN X-Fi Style uses a GPIO because both select and power are wired
     * to PSWITCH resulting in slow and unreliable readings */
    if(!imx233_pinctrl_get_gpio(0, 11))
        btn |= BUTTON_POWER;
#else
    if(imx233_power_read_pswitch() == 1)
        btn |= BUTTON_POWER;
#endif
    return imx233_button_lradc_read(btn);
}
