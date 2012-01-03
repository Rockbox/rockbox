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
 * Gigabeat S MC13783 event descriptions
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
#include "config.h"
#include "system.h"
#include "spi-imx31.h"
#include "mc13783.h"
#include "mc13783-target.h"
#include "adc-target.h"
#include "button-target.h"
#include "power-gigabeat-s.h"
#include "powermgmt-target.h"

/* Gigabeat S mc13783 serial interface node. */

struct spi_node mc13783_spi =
{
    /* Based upon original firmware settings */
    CSPI2_NUM,                     /* CSPI module 2 */
    CSPI_CONREG_CHIP_SELECT_SS0 |  /* Chip select 0 */
    CSPI_CONREG_DRCTL_DONT_CARE |  /* Don't care about CSPI_RDY */
    CSPI_CONREG_DATA_RATE_DIV_32 | /* Clock = IPG_CLK/32 = 2,062,500Hz. */
    CSPI_BITCOUNT(32-1) |          /* All 32 bits are to be transferred */
    CSPI_CONREG_SSPOL |            /* SS active high */
    CSPI_CONREG_SSCTL |            /* Negate SS between SPI bursts */
    CSPI_CONREG_MODE,              /* Master mode */
    0,                             /* SPI clock - no wait states */
};


/* Gigabeat S definitions for static MC13783 event registration */

const struct mc13783_event mc13783_events[MC13783_NUM_EVENTS] =
{
    [MC13783_ADCDONE_EVENT] = /* ADC conversion complete */
    {
        .int_id   = MC13783_INT_ID_ADCDONE,
        .sense    = 0,
        .callback = adc_done,
    },
    [MC13783_ONOFD1_EVENT] = /* Power button */
    {
        .int_id   = MC13783_INT_ID_ONOFD1,
        .sense    = MC13783_ONOFD1S,
        .callback = button_power_event,
    },
    [MC13783_SE1_EVENT] = /* Main charger detection */
    {
        .int_id   = MC13783_INT_ID_SE1,
        .sense    = MC13783_SE1S,
        .callback = charger_main_detect_event,
    },
    [MC13783_USB_EVENT] = /* USB insertion/USB charger detection */
    {
        .int_id   = MC13783_INT_ID_USB,
        .sense    = MC13783_USB4V4S,
        .callback = usb_connect_event,
    },
#ifdef HAVE_HEADPHONE_DETECTION
    [MC13783_ONOFD2_EVENT] = /* Headphone jack */
    {
        .int_id   = MC13783_INT_ID_ONOFD2,
        .sense    = 0,
        .callback = headphone_detect_event,
    },
#endif
};
