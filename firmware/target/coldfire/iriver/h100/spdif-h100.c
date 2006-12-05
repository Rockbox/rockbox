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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
    spdif_set_output_source(AUDIO_SRC_PLAYBACK, true);
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
    static const unsigned short ebu1_config[] =
    {
        /* SCLK2, TXSRC = PDOR3, validity, normal operation */
        [AUDIO_SRC_PLAYBACK+1] = (7 << 12) | (3 << 8) | (1 << 5) | (5 << 2),
        /* Input source is EBUin1, Feed-through monitoring */
        [AUDIO_SRC_SPDIF+1]    =                                   (1 << 2),
        /* SCLK2, TXSRC = IIS1recv, validity, normal operation */
        [AUDIO_SRC_MIC+1]      = (7 << 12) | (4 << 8) | (1 << 5) | (5 << 2),
        [AUDIO_SRC_LINEIN+1]   = (7 << 12) | (4 << 8) | (1 << 5) | (5 << 2),
        [AUDIO_SRC_FMRADIO+1]  = (7 << 12) | (4 << 8) | (1 << 5) | (5 << 2),
    };

    int level;
    unsigned long playing, recording;

    if ((unsigned)source >= ARRAYLEN(ebu1_config))
        source = AUDIO_SRC_PLAYBACK;

    spdif_source = source;
    spdif_on     = spdif_powered() && src_on;

    level = set_irq_level(HIGHEST_IRQ_LEVEL);

    /* Check if DMA peripheral requests are enabled */
    playing   = DCR0 & DMA_EEXT;
    recording = DCR1 & DMA_EEXT;

    EBU1CONFIG = 0x800; /* Reset before reprogram */

    /* Tranceiver must be powered or else monitoring will be disabled */
    EBU1CONFIG = spdif_on ? ebu1_config[source + 1] : 0;

    /* Kick-start DMAs if in progress */
    if (recording)
        DCR1 |= DMA_START;

    if (playing)
        DCR0 |= DMA_START;

    set_irq_level(level);
} /* spdif_set_output_source */

/* Return the last set S/PDIF audio source */
int spdif_get_output_source(bool *src_on)
{
    if (src_on != NULL)
        *src_on = spdif_on;
    return spdif_source;
} /* spdif_get_output_source */
