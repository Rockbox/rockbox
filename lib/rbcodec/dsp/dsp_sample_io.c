/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Miika Pekkarinen
 * Copyright (C) 2012 Michael Sevakis
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
#include "rbcodecconfig.h"
#include "platform.h"
#include "dsp_core.h"
#include "dsp_sample_io.h"
#include "dsp_proc_entry.h"
#include "fixedpoint.h"

/* increment the format version counter */
static void format_change_set(struct sample_io_data *this)
{
    if (this->format_dirty)
        return;

    this->format.version = (uint8_t)(this->format.version + 1) ?: 1;
    this->format_dirty = 1;
}

bool dsp_sample_io_configure(struct sample_io_data *this,
                             unsigned int setting,
                             intptr_t *value_p)
{
    intptr_t value = *value_p;

    switch (setting)
    {
    case DSP_INIT:
        this->output_sampr = DSP_OUT_DEFAULT_HZ;
        dsp_sample_input_init(this, value);
        dsp_sample_output_init(this);
        break;

    case DSP_RESET:
        /* Reset all sample descriptions to default */
        format_change_set(this);
        this->format.num_channels = 2;
        this->format.frac_bits = WORD_FRACBITS;
        this->format.output_scale = WORD_FRACBITS + 1 - NATIVE_DEPTH;
        this->format.frequency = this->output_sampr;
        this->format.codec_frequency = this->output_sampr;
        this->sample_depth = NATIVE_DEPTH;
        this->stereo_mode = STEREO_NONINTERLEAVED;
        break;

    case DSP_SET_FREQUENCY:
        format_change_set(this);
        value = value > 0 ? (unsigned int)value : this->output_sampr;
        this->format.frequency = value;
        this->format.codec_frequency = value;
        break;

    case DSP_SET_SAMPLE_DEPTH:
        format_change_set(this);
        this->format.frac_bits =
            value <= NATIVE_DEPTH ? WORD_FRACBITS : value;
        this->format.output_scale =
            this->format.frac_bits + 1 - NATIVE_DEPTH;
        this->sample_depth = value;
        break;

    case DSP_SET_STEREO_MODE:
        format_change_set(this);
        this->format.num_channels = value == STEREO_MONO ? 1 : 2;
        this->stereo_mode = value;
        break;

    case DSP_FLUSH:
        dsp_sample_input_flush(this);
        dsp_sample_output_flush(this);
        break;

    case DSP_SET_PITCH:
        format_change_set(this);
        value = value > 0 ? value : (1 << 16);
        this->format.frequency =
            fp_mul(value, this->format.codec_frequency, 16);
        break;

    case DSP_SET_OUT_FREQUENCY:
        value = value > 0 ? value : DSP_OUT_DEFAULT_HZ;
        value = MIN(DSP_OUT_MAX_HZ, MAX(DSP_OUT_MIN_HZ, value));
        *value_p = value;

        if ((unsigned int)value == this->output_sampr)
            return true; /* No change; don't broadcast */

        this->output_sampr = value;
        break;

    case DSP_GET_OUT_FREQUENCY:
        *value_p = this->output_sampr;
        return true; /* Only I/O handles it */
    }

    return false;
}
