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
#include "config.h"
#include "system.h"
#include "dsp.h"
#include "dsp_sample_io.h"
#include <sys/types.h>

/* Define LOGF_ENABLE to enable logf output in this file */
/*#define LOGF_ENABLE*/
#include "logf.h"

#if 0
#include <debug.h>
#else
#undef DEBUGF
#define DEBUGF(...)
#endif

/**
 * Main database of effects
 *
 * Order is not particularly relevant and has no intended correlation with IDs.
 * 
 * Notable exceptions in ordering:
 *  * Sample input: which is first in line and has special responsibilities
 *    (not an effect per se).
 *  * Anything that depends on the native sample rate must go after the
 *    resampling stage.
 *  * Some bizarre dependency I didn't think of but you decided to implement.
 *  * Sample output: Naturally, this takes the final result and converts it to
 *    the target PCM format (not an effect per se).
 */
static struct dsp_proc_db_entry const * const dsp_proc_database[] =
{
    &dsp_misc_handler_proc_db_entry, /* misc stuff (doesn't process samples) */
    &pga_proc_db_entry,          /* pre-gain amp */
#ifdef HAVE_PITCHSCREEN
    &tdspeed_proc_db_entry,      /* time-stretching */
#endif
    &resample_proc_db_entry,     /* resampler providing NATIVE_FREQUENCY */

    &crossfeed_proc_db_entry,    /* stereo crossfeed */
    &eq_proc_db_entry,           /* n-band equalizer */
#ifdef HAVE_SW_TONE_CONTROLS
    &tone_proc_db_entry,         /* bass and treble */
#endif
    &channel_mode_proc_db_entry, /* channel modes */
    &compressor_proc_db_entry,   /* dynamic-range compressor */
};

/* Number of effects in database - all available in audio DSP */
#define DSP_NUM_PROC_STAGES ARRAYLEN(dsp_proc_database)

/* Number of possible effects for voice DSP */
#ifdef HAVE_SW_TONE_CONTROLS
#define DSP_VOICE_NUM_PROC_STAGES 2 /* resample, tone */
#else
#define DSP_VOICE_NUM_PROC_STAGES 1 /* resample */
#endif
/*
 ***************************************************************************/

/* Linked lists give fewer loads in processing loop compared to some index
 * list, which is more important than keeping occasionally executed code
 * simple */
struct dsp_proc_slot
{
    struct dsp_proc_entry proc_entry;
    const struct dsp_proc_db_entry *db_entry;
    struct dsp_proc_slot *next[2]; /* [0]=active next, [1]=enabled next */
};

/** General DSP-local data **/
struct dsp_config
{
    struct sample_io_data io_data; /* Sample input-output data (first) */
#ifdef CPU_COLDFIRE
    unsigned long old_macsr;     /* Old macsr value to restore */
#endif
#if 0 /* Not needed now but enable if something must know this */
#ifdef HAVE_PITCHSCREEN
    bool processing;             /* DSP is processing (to thwart inopportune
                                    buffer moves) */
#endif
#endif
    /** General dependant processing functions **/
    uint32_t slot_free_mask;     /* Mask of free slots */
    uint32_t proc_masks[2];      /* Mask of active/enabled stages */
    struct dsp_proc_slot *proc_slots[2]; /* Pointer to first in list of
                                            active/enabled stages */
    struct dsp_proc_slot *proc_slot_arr; /* Pointer to beginning of pool */
};

/* Allocate slots separately from dsp_config because not all may be applied
   to voice (avoid IRAM waste) */
static struct dsp_proc_slot
audio_dsp_proc_slots[DSP_NUM_PROC_STAGES] IBSS_ATTR;

static struct dsp_proc_slot
voice_dsp_proc_slots[DSP_VOICE_NUM_PROC_STAGES] IBSS_ATTR;

/* General DSP config */
static struct dsp_config dsp_conf[DSP_COUNT] IBSS_ATTR;

#define AUDIO_DSP (dsp_conf[CODEC_IDX_AUDIO])
#define VOICE_DSP (dsp_conf[CODEC_IDX_VOICE])

/** Processing stages support functions **/

/* Broadcast to all enabled stages */
static void proc_broadcast(struct dsp_config *dsp, enum dsp_settings setting,
                           intptr_t value)
{
    struct dsp_proc_slot *s = dsp->proc_slots[1];

    while (s != NULL)
    {
        s->db_entry->configure(&s->proc_entry, dsp, setting, value);
        s = s->next[1];
    }
}

/* Find the slot for a given enabled id */
static struct dsp_proc_slot * find_proc_slot(struct dsp_config *dsp,
                                             enum dsp_proc_ids id)
{
    const uint32_t mask = BIT_N(id);

    if ((dsp->proc_masks[1] & mask) == 0)
        return NULL; /* Not enabled */

    struct dsp_proc_slot *s = dsp->proc_slots[1];

    while (1) /* In proc_masks == it must be there */
    {
        if (BIT_N(s->db_entry->id) == mask)
            return s;

        s = s->next[1];
    }
}

/* Generic handler for this->process[0] */
static void dsp_process_null(struct dsp_proc_entry *this,
                             struct dsp_buffer **buf_p)
{
    (void)this; (void)buf_p;
}

/* Generic handler for this->process[1] */
static void dsp_format_change_process(struct dsp_proc_entry *this,
                                      struct dsp_buffer **buf_p)
{
    enum dsp_proc_ids id =
        TYPE_FROM_MEMBER(struct dsp_proc_slot, this, proc_entry)->db_entry->id;

    DSP_PRINT_FORMAT(<Default Handler>, id, (*buf_p)->format);

    /* We don't keep back references to the DSP, so just search for it */
    struct dsp_config *dsp;
    for (int i = 0; (dsp = dsp_get_config(i)); i++)
    {
        struct dsp_proc_slot *slot = find_proc_slot(dsp, id);
        /* Found one with the id, check if it's this one */
        if (&slot->proc_entry == this && dsp_proc_active(dsp, id))
        {
            dsp_proc_call(this, buf_p, 0);
            break;
        }
    }
}

/* Add an item to the enabled list */
static struct dsp_proc_slot *
dsp_proc_enable_enlink(struct dsp_config *dsp, uint32_t mask)
{
    /* Use the lowest-indexed available slot */
    int slot = find_first_set_bit(dsp->slot_free_mask);

    if (slot == 32)
    {
        /* Should NOT happen, ever, unless called before init */
        DEBUGF("DSP %d: no slots!\n", (int)dsp_get_id(dsp));
        return NULL;
    }

    int prev_id = -1;
    const struct dsp_proc_db_entry *db_entry;

    /* Order of enabled list is same as DB array */
    for (unsigned int i = 0;; i++)
    {
        if (i >= DSP_NUM_PROC_STAGES)
            return NULL;

        db_entry = dsp_proc_database[i];

        enum dsp_proc_ids id = db_entry->id;
        uint32_t m = BIT_N(id);

        if (m == mask)
            break;

        if (dsp->proc_masks[1] & m)
            prev_id = id;
    }

    struct dsp_proc_slot *s = &dsp->proc_slot_arr[slot];

    if (prev_id >= 0)
    {
        struct dsp_proc_slot *prev = find_proc_slot(dsp, prev_id);
        s->next[0] = prev->next[0];
        s->next[1] = prev->next[1];
        prev->next[1] = s;
    }
    else
    {
        s->next[0] = dsp->proc_slots[0];
        s->next[1] = dsp->proc_slots[1];
        dsp->proc_slots[1] = s;
    }

    s->db_entry = db_entry; /* record DB entry */
    dsp->proc_masks[1] |= mask;
    dsp->slot_free_mask &= ~BIT_N(slot);

    return s;
}

/* Remove an item from the enabled list */
static struct dsp_proc_slot *
dsp_proc_enable_delink(struct dsp_config *dsp, uint32_t mask)
{
    struct dsp_proc_slot *s = dsp->proc_slots[1];
    struct dsp_proc_slot *prev = NULL;

    while (1) /* In proc_masks == it must be there */
    {
        if (BIT_N(s->db_entry->id) == mask)
        {
            if (prev)
                prev->next[1] = s->next[1];
            else
                dsp->proc_slots[1] = s->next[1];

            dsp->proc_masks[1] &= ~mask;
            dsp->slot_free_mask |= BIT_N(s - dsp->proc_slot_arr);
            return s;
        }

        prev = s;
        s = s->next[1];
    }
}

void dsp_proc_enable(struct dsp_config *dsp, enum dsp_proc_ids id,
                     bool enable)
{
    uint32_t mask = BIT_N(id);
    bool enabled = dsp->proc_masks[1] & mask;

    if (enable)
    {
        /* If enabled, just find it in list, if not, link a new one */
        struct dsp_proc_slot *s = enabled ? find_proc_slot(dsp, id) :
                                            dsp_proc_enable_enlink(dsp, mask);

        if (s == NULL)
        {
            DEBUGF("DSP: proc id not valid: %d\n", (int)id);
            return;
        }

        if (!enabled)
        {
            /* New entry - set defaults */
            s->proc_entry.ip_mask = mask;
            s->proc_entry.process[0] = dsp_process_null;
            s->proc_entry.process[1] = dsp_format_change_process;
            s->proc_entry.data = 0;
        }

        s->db_entry->configure(&s->proc_entry, dsp, DSP_PROC_INIT, enabled);
    }
    else if (enabled)
    {
        dsp_proc_activate(dsp, id, false); /* deactivate it first */
        struct dsp_proc_slot *s = dsp_proc_enable_delink(dsp, mask);
        s->db_entry->configure(&s->proc_entry, dsp, DSP_PROC_CLOSE, 0);
    }
}

/* Maintain the list structure for the active list where each enabled entry
 * has a link to the next active item, even if not active which facilitates
 * switching out of format change mode by a stage during a format change.
 * When that happens, the iterator must jump over inactive but enabled
 * stages. */
static struct dsp_proc_slot *
dsp_proc_activate_link(struct dsp_config *dsp, uint32_t mask,
                       struct dsp_proc_slot *s)
{
    uint32_t m = BIT_N(s->db_entry->id);
    uint32_t mor = m | mask;

    if (mor == m) /* Only if same single bit in common */
    {
        dsp->proc_masks[0] |= mask;
        return s;
    }
    else if (~mor == 0) /* Only if bits complement */
    {
        dsp->proc_masks[0] &= mask;
        return s->next[0];
    }

    struct dsp_proc_slot *next = s->next[1];
    next = dsp_proc_activate_link(dsp, mask, next);

    s->next[0] = next;

    return (m & dsp->proc_masks[0]) ? s : next;
}

/* Activate or deactivate a stage */
void dsp_proc_activate(struct dsp_config *dsp, enum dsp_proc_ids id,
                       bool activate)
{
    const uint32_t mask = BIT_N(id);

    if (!(dsp->proc_masks[1] & mask))
        return; /* Not enabled */

    if (activate != !(dsp->proc_masks[0] & mask))
        return; /* No change in state */

    /* Send mask bit if activating and ones complement if deactivating */
    dsp->proc_slots[0] = dsp_proc_activate_link(
            dsp, activate ? mask : ~mask, dsp->proc_slots[1]);
}

/* Is the stage specified by the id currently active? */
bool dsp_proc_active(struct dsp_config *dsp, enum dsp_proc_ids id)
{
    return (dsp->proc_masks[0] & BIT_N(id)) != 0;
}

/* Determine by the rules if the processing function should be called */
static FORCE_INLINE bool dsp_proc_should_call(struct dsp_proc_entry *this,
                                              struct dsp_buffer *buf,
                                              unsigned fmt)
{
    uint32_t ip_mask = this->ip_mask;

    return UNLIKELY(fmt != 0) || /* Also pass override value */
           ip_mask == 0 || /* Not in-place */
           ((ip_mask & buf->proc_mask) == 0 &&
            (buf->proc_mask |= ip_mask, buf->remcount > 0));
}

/* Call this->process[fmt] according to the rules (for external call) */
bool dsp_proc_call(struct dsp_proc_entry *this, struct dsp_buffer **buf_p,
                   unsigned fmt)
{
    if (dsp_proc_should_call(this, *buf_p, fmt))
    {
        this->process[fmt == (0u-1u) ? 0 : fmt](this, buf_p);
        return true;
    }

    return false;
}

static inline void dsp_process_start(struct dsp_config *dsp)
{
#if defined(CPU_COLDFIRE)
    /* set emac unit for dsp processing, and save old macsr, we're running in
       codec thread context at this point, so can't clobber it */
    dsp->old_macsr = coldfire_get_macsr();
    coldfire_set_macsr(EMAC_FRACTIONAL | EMAC_SATURATE);
#endif
#if 0 /* Not needed now but enable if something must know this */
#ifdef HAVE_PITCHSCREEN
    dsp->processing = true;
#endif
#endif
    (void)dsp;
}

static inline void dsp_process_end(struct dsp_config *dsp)
{
#if 0 /* Not needed now but enable if something must know this */
#ifdef HAVE_PITCHSCREEN
    dsp->processing = false;
#endif
#endif
#if defined(CPU_COLDFIRE)
    /* set old macsr again */
    coldfire_set_macsr(dsp->old_macsr);
#endif
    (void)dsp;
}

/**
 * dsp_process:
 *
 * Process and convert src audio to dst based on the DSP configuration.
 * dsp:            the DSP instance in use
 *
 * src:
 *     remcount  = number of input samples remaining; set to desired
 *                 number of samples to be processed
 *     pin[0]    = left channel if non-interleaved, audio data if
 *                 interleaved or mono
 *     pin[1]    = right channel if non-interleaved, ignored if
 *                 interleaved or mono
 *     proc_mask = set to zero on first call, updated by this function
 *                 to keep track of which in-place stages have been
 *                 run on the buffers to avoid multiple applications of
 *                 them
 *     format    = for internal buffers, gives the relevant format
 *                 details
 *
 * dst:
 *     remcount  = number of samples placed in buffer so far; set to
 *                 zero on first call
 *     p16out    = current fill pointer in destination buffer; set to
 *                 buffer start on first call
 *     bufcount  = remaining buffer space in samples; set to maximum
 *                 desired output count on first call
 *     format    = ignored
 *
 * Processing stops when src is exhausted or dst is filled, whichever
 * happens first. Samples can still be output when src buffer is empty
 * if samples are held internally. Generally speaking, continue calling
 * until no data is consumed and no data is produced to purge the DSP
 * to the maximum extent feasible. Some internal processing stages may
 * require more input before more output can be generated, thus there
 * is no guarantee the DSP is free of data awaiting processing at that
 * point.
 *
 * Additionally, samples consumed and samples produced do not necessarily
 * have a direct correlation. Samples may be consumed without producing
 * any output and samples may be produced without consuming any input.
 * It depends on which stages are actively processing data at the time
 * of the call and how they function internally.
 */
void dsp_process(struct dsp_config *dsp, struct dsp_buffer *src,
                 struct dsp_buffer *dst)
{
    if (dst->bufcount <= 0)
    {
        /* No place to put anything thus nothing may be safely consumed */
        return;
    }

    /* Tag input with codec-specified sample format */
    src->format = dsp->io_data.format;
    format_change_ack(&dsp->io_data.format);

    /* At least perform one yield before starting */
    long last_yield = current_tick;
    yield();

    dsp_process_start(dsp);

    while (1)
    {
        /* Out-of-place-processing stages take the current buf as input
         * and switch the buffer to their own output buffer */
        struct dsp_buffer *buf = src;
        unsigned fmt = buf->format.changed;

        /* Convert input samples to internal format */
        dsp->io_data.input_samples[fmt](&dsp->io_data, &buf);

        fmt = buf->format.changed;
        struct dsp_proc_slot *s = dsp->proc_slots[fmt];

        /* Call all active/enabled stages depending if format is
           same/changed on the last output buffer */
        while (s != NULL)
        {
            if (dsp_proc_should_call(&s->proc_entry, buf, fmt))
            {
                s->proc_entry.process[fmt](&s->proc_entry, &buf);
                fmt = buf->format.changed;
            }

            /* The buffer may have changed along with the format flag */
            s = s->next[fmt];
        }

        /* Don't overread/write src/destination */
        int outcount = MIN(dst->bufcount, buf->remcount);

        if (fmt == 0 && outcount <= 0)
            break; /* Output full or purged internal buffers */

        dsp->io_data.outcount = outcount;
        dsp->io_data.output_samples[fmt](&dsp->io_data, buf, dst);

        /* Advance buffers by what output consumed and produced */
        dsp_advance_buffer32(buf, outcount);
        dsp_advance_buffer_output(dst, outcount);

        /* Yield at least once each tick */
        long tick = current_tick;
        if (TIME_AFTER(tick, last_yield))
        {
            last_yield = tick;
            yield();
        }
    } /* while */

    dsp_process_end(dsp);
}

intptr_t dsp_configure(struct dsp_config *dsp, enum dsp_settings setting,
                       intptr_t value)
{
    switch (setting)
    {
    case DSP_RESET:
        /* Reset all sample descriptions to default */
        format_change_set(&dsp->io_data.format);
        dsp->io_data.format.num_channels = 2;
        dsp->io_data.format.frac_bits = WORD_FRACBITS;
        dsp->io_data.format.output_scale = WORD_FRACBITS + 1 - NATIVE_DEPTH;
        dsp->io_data.format.frequency = NATIVE_FREQUENCY;
        dsp->io_data.format.codec_frequency = NATIVE_FREQUENCY;
        dsp->io_data.sample_depth = NATIVE_DEPTH;
        dsp->io_data.stereo_mode = STEREO_NONINTERLEAVED;
        break;

    case DSP_SET_FREQUENCY:
        value = value > 0 ? value : NATIVE_FREQUENCY;
        format_change_set(&dsp->io_data.format);
        dsp->io_data.format.frequency = value;
        dsp->io_data.format.codec_frequency = value;
        break;

    case DSP_SET_SAMPLE_DEPTH:
        format_change_set(&dsp->io_data.format);
        dsp->io_data.format.frac_bits =
            value <= NATIVE_DEPTH ? WORD_FRACBITS : value;
        dsp->io_data.format.output_scale =
            dsp->io_data.format.frac_bits + 1 - NATIVE_DEPTH;
        dsp->io_data.sample_depth = value;
        break;

    case DSP_SET_STEREO_MODE:
        format_change_set(&dsp->io_data.format);
        dsp->io_data.format.num_channels = value == STEREO_MONO ? 1 : 2;
        dsp->io_data.stereo_mode = value;
        break;

    case DSP_FLUSH:
        dsp_sample_input_flush(&dsp->io_data);
        dsp_sample_output_flush(&dsp->io_data);
        break;

    case DSP_SET_REPLAY_GAIN:
        break;

    default:
        logf("DSP- Illegal key: %d", (int)setting);
        return 0;
    }

    /* Forward to all enabled processing stages */
    proc_broadcast(dsp, setting, value);
    return 1;
}

struct dsp_config * dsp_get_config(enum dsp_ids id)
{
    switch (id)
    {
    case CODEC_IDX_AUDIO:
        return &AUDIO_DSP;
    case CODEC_IDX_VOICE:
        return &VOICE_DSP;
    default:
        return NULL;
    }
}

/* Return the id given a dsp pointer (or even something within
   the struct itself */
enum dsp_ids dsp_get_id(const struct dsp_config *dsp)
{
    if (dsp < &dsp_conf[0] || dsp >= &dsp_conf[DSP_COUNT])
        return DSP_COUNT; /* obviously invalid */

    return (enum dsp_ids)(dsp - dsp_conf);
}

#if 0 /* Not needed now but enable if something must know this */
#ifdef HAVE_PITCHSCREEN
bool dsp_is_busy(const struct dsp_config *dsp)
{
    return dsp->processing;
}
#endif
#endif /* 0 */

/* Do what needs initializing before enable/disable calls can be made.
 * Must be done before changing settings for the first time */
void INIT_ATTR dsp_init(void)
{
    static const struct dsp_setup
    {
        uint32_t              slot_free_mask;
        struct dsp_proc_slot *proc_slot_arr;
    } dsp_setups[DSP_COUNT] /* INITDATA_ATTR */ =
    {
        [CODEC_IDX_AUDIO] =
        {
            .slot_free_mask = BIT_N(DSP_NUM_PROC_STAGES) - 1,
            .proc_slot_arr  = audio_dsp_proc_slots,
        },
        [CODEC_IDX_VOICE] =
        {
            .slot_free_mask = BIT_N(DSP_VOICE_NUM_PROC_STAGES) - 1,
            .proc_slot_arr  = voice_dsp_proc_slots,
        },
    };

    for (unsigned i = 0; i < DSP_COUNT; i++)
    {
        struct dsp_config *dsp = &dsp_conf[i];

        const struct dsp_setup *setup = &dsp_setups[i];

        dsp->slot_free_mask = setup->slot_free_mask;
        dsp->proc_slot_arr = setup->proc_slot_arr;

        dsp_sample_input_init(&dsp->io_data);
        dsp_sample_output_init(&dsp->io_data);

        /* Notify each db entry of global init for each dsp */
        for (unsigned j = 0; j < DSP_NUM_PROC_STAGES; j++)
            dsp_proc_database[j]->configure(NULL, dsp, DSP_INIT, i);

        dsp_configure(dsp, DSP_RESET, 0);
    }
}
