/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Audio signal path management APIs
 *
 * Copyright (C) 2007 by Michael Sevakis
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
#include "system.h"
#include "cpu.h"
#include "audio.h"
#include "general.h"
#include "settings.h"
#if defined(HAVE_SPDIF_IN) || defined(HAVE_SPDIF_OUT)
#ifdef HAVE_SPDIF_POWER
#include "power.h"
#endif
#include "spdif.h"
#endif
#if CONFIG_TUNER
#include "radio.h"
#endif

/* Some audio sources may require a boosted CPU */
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#ifdef HAVE_SPDIF_REC
#define AUDIO_CPU_BOOST
#endif
#endif

#ifndef SIMULATOR

#ifdef AUDIO_CPU_BOOST
static void audio_cpu_boost(bool state)
{
    static bool cpu_boosted = false;

    if (state != cpu_boosted)
    {
        cpu_boost(state);
        cpu_boosted = state;
    }
} /* audio_cpu_boost */
#endif /* AUDIO_CPU_BOOST */

/**
 * Selects an audio source for recording or playback
 * powers/unpowers related devices and sets up monitoring.
 */
void audio_set_input_source(int source, unsigned flags)
{
    /** Do power up/down of associated device(s) **/

    /** SPDIF **/
#ifdef AUDIO_CPU_BOOST
    /* Always boost for SPDIF */
    audio_cpu_boost(source == AUDIO_SRC_SPDIF);
#endif /* AUDIO_CPU_BOOST */

#ifdef HAVE_SPDIF_POWER
    /* Check if S/PDIF output power should be switched off or on. NOTE: assumes
       both optical in and out is controlled by the same power source, which is
       the case on H1x0. */
    spdif_power_enable((source == AUDIO_SRC_SPDIF) ||
                       global_settings.spdif_enable);
#endif /* HAVE_SPDIF_POWER */
    /* Set the appropriate feed for spdif output */
#ifdef HAVE_SPDIF_OUT
    spdif_set_output_source(source
        IF_SPDIF_POWER_(, global_settings.spdif_enable));
#endif /* HAVE_SPDIF_OUT */

    /** Tuner **/
#if CONFIG_TUNER
    /* Switch radio off or on per source and flags. */
    if (source != AUDIO_SRC_FMRADIO)
        radio_stop();
    else if (flags & SRCF_FMRADIO_PAUSED)
        radio_pause();
    else
        radio_start();
#endif

    /* set hardware inputs */
    audio_input_mux(source, flags);
} /* audio_set_source */

#ifdef HAVE_SPDIF_IN
/**
 * Return SPDIF sample rate index in audio_master_sampr_list. Since we base
 * our reading on the actual SPDIF sample rate (which might be a bit
 * inaccurate), we round off to the closest sample rate that is supported by
 * SPDIF.
 */
int audio_get_spdif_sample_rate(void)
{
    unsigned long measured_rate = spdif_measure_frequency();
    /* Find which SPDIF sample rate we're closest to. */
    return round_value_to_list32(measured_rate, audio_master_sampr_list,
                                 SAMPR_NUM_FREQ, false);
} /* audio_get_spdif_sample_rate */
#endif /* HAVE_SPDIF_IN */

#else /* SIMULATOR */

/** Sim stubs **/

#ifdef AUDIO_CPU_BOOST
static void audio_cpu_boost(bool state)
{
    (void)state;
} /* audio_cpu_boost */
#endif /* AUDIO_CPU_BOOST */

void audio_set_output_source(int source)
{
   (void)source;
} /* audio_set_output_source */

void audio_set_input_source(int source, unsigned flags)
{
    (void)source;
    (void)flags;
} /* audio_set_input_source */

#ifdef HAVE_SPDIF_IN
int audio_get_spdif_sample_rate(void)
{
    return FREQ_44;
} /* audio_get_spdif_sample_rate */
#endif /* HAVE_SPDIF_IN */

#endif /* !SIMULATOR */
