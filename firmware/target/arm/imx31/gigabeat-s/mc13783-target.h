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
 * Gigabeat S mc13783 event descriptions
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
#ifndef MC13783_TARGET_H
#define MC13783_TARGET_H

#include "mc13783.h"

#ifdef DEFINE_MC13783_VECTOR_TABLE

/* Gigabeat S mc13783 serial interface node. */
static struct spi_node mc13783_spi =
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

MC13783_EVENT_VECTOR_CB(ADCDONE);
#if CONFIG_RTC
MC13783_EVENT_VECTOR_CB(1HZ);
#endif
MC13783_EVENT_VECTOR_CB(ONOFD1);
MC13783_EVENT_VECTOR_CB(SE1);
MC13783_EVENT_VECTOR_CB(USB);
MC13783_EVENT_VECTOR_CB(ONOFD2);

/* Gigabeat S definitions for static MC13783 event registration */
MC13783_EVENT_VECTOR_TBL_START()
    /* ADC conversion complete */
    MC13783_EVENT_VECTOR(ADCDONE, 0)
#if CONFIG_RTC
    /* RTC tick */
    MC13783_EVENT_VECTOR(1HZ, 0)
#endif /* CONFIG_RTC */
    /* Power button */
    MC13783_EVENT_VECTOR(ONOFD1, MC13783_ONOFD1S)
    /* Main charger detection */
    MC13783_EVENT_VECTOR(SE1, MC13783_SE1S)
    /* USB insertion/USB charger detection */
    MC13783_EVENT_VECTOR(USB, MC13783_USB4V4S)
#ifdef HAVE_HEADPHONE_DETECTION
    /* Headphone jack */
    MC13783_EVENT_VECTOR(ONOFD2, 0)
#endif /* HAVE_HEADPHONE_DETECTION */
MC13783_EVENT_VECTOR_TBL_END()

#endif /* DEFINE_MC13783_VECTOR_TABLE */

#endif /* MC13783_TARGET_H */
