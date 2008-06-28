/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Michal Sevakis
 * Based on the work of Thom Johansen
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
#include <stdbool.h>
#include "config.h"
#include "cpu.h"
#include "power.h"
#include "system.h"
#include "audio.h"
#include "spdif.h"

static int spdif_source = AUDIO_SRC_PLAYBACK;
static int spdif_on     = false;

/* Initialize the S/PDIF driver */
void spdif_init(void)
{
    /* PHASECONFIG setup: gain = 3*2^13, source = EBUIN */
    PHASECONFIG = (6 << 3) | (4 << 0);
    spdif_set_output_source(AUDIO_SRC_PLAYBACK
        IF_SPDIF_POWER_(, true));
}

/* Return the S/PDIF frequency in herz - unrounded */
unsigned long spdif_measure_frequency(void)
{
    /* The following formula is specified in MCF5249 user's manual section
     * 17.6.1. The 128 divide is because of the fact that the SPDIF clock is
     * the sample rate times 128.
     */
    return (unsigned long)((unsigned long long)FREQMEAS*CPU_FREQ /
                                               ((1 << 15)*3*(1 << 13))/128);
} /* spdif_measure_frequency */

/* Set the S/PDIF audio feed */
void spdif_set_output_source(int source, bool src_on)
{
    static const unsigned short ebu1_config[AUDIO_NUM_SOURCES+1] =
    {
        /* SCLK2, TXSRC = PDOR3, validity, normal operation */
        [AUDIO_SRC_PLAYBACK+1] = (3 << 8) | (1 << 5) | (5 << 2),
        /* Input source is EBUin1, Feed-through monitoring */
        [AUDIO_SRC_SPDIF+1]    =                       (1 << 2),
        /* SCLK2, TXSRC = IIS1recv, validity, normal operation */
        [AUDIO_SRC_MIC+1]      = (4 << 8) | (1 << 5) | (5 << 2),
        [AUDIO_SRC_LINEIN+1]   = (4 << 8) | (1 << 5) | (5 << 2),
        [AUDIO_SRC_FMRADIO+1]  = (4 << 8) | (1 << 5) | (5 << 2),
    };

    bool kick;
    int level;
    int iis2config = 0;

    if ((unsigned)source >= ARRAYLEN(ebu1_config))
        source = AUDIO_SRC_PLAYBACK;

    spdif_source = source;
    spdif_on     = spdif_powered() && src_on;

    /* Keep a DMA interrupt initiated stop from changing play state */
    level = set_irq_level(DMA_IRQ_LEVEL);

    kick         = spdif_on && source == AUDIO_SRC_PLAYBACK
                   && (DCR0 & DMA_EEXT);

    /* FIFO must be in reset condition to reprogram bits 15-12 */
    or_l(0x800, &EBU1CONFIG);

    if (kick)
    {
        iis2config = IIS2CONFIG;
        or_l(0x800, &IIS2CONFIG); /* Have to resync IIS2 TXSRC */
    }

    /* Tranceiver must be powered or else monitoring will be disabled.
       CLOCKSEL bits only have relevance to normal operation so just
       set them always. */
    EBU1CONFIG = (spdif_on ? ebu1_config[source + 1] : 0) | (7 << 12);

    if (kick)
    {
        IIS2CONFIG = iis2config;
        PDOR3 = 0; /* A write to the FIFO kick-starts playback */
    }

    restore_irq(level);
} /* spdif_set_output_source */

/* Return the last set S/PDIF audio source */
int spdif_get_output_source(bool *src_on)
{
    if (src_on != NULL)
        *src_on = spdif_on;
    return spdif_source;
} /* spdif_get_output_source */
