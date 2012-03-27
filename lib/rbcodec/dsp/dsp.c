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
#include "kernel.h"
#include <sound.h>
#include "dsp.h"
#include "dsp-util.h"
#include "settings.h"
#include "replaygain.h"
#include "fixedpoint.h"
#include "fracmul.h"

/* Define LOGF_ENABLE to enable logf output in this file */
/*#define LOGF_ENABLE*/
#include "logf.h"

/* Remove this to enable debug messages */
#undef DEBUGF
#define DEBUGF(...)

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

/* 16-bit samples are scaled based on these constants. The shift should be
 * no more than 15.
 */
#define WORD_SHIFT              12
#define WORD_FRACBITS           27
#define NATIVE_DEPTH            16


/** Common function prototypes **/

/* DSP initial buffer input function call prototype */
typedef void (*sample_input_fn_type)(struct dsp_config *dsp,
                                     struct dsp_buffer **buf_p);

/* DSP final buffer output function call prototype */
typedef void (*sample_output_fn_type)(int count, struct dsp_buffer *src,
                                      struct dsp_buffer *dst);

/*
 ***************************************************************************/

/* May be implemented in here or externally.*/
void sample_output_mono(int count, struct dsp_buffer *src,
                        struct dsp_buffer *dst);
void sample_output_stereo(int count, struct dsp_buffer *src,
                          struct dsp_buffer *dst);
void sample_output_dithered(int count, struct dsp_buffer *src,
                            struct dsp_buffer *dst);

/* Linked lists give fewer loads in processing loop compared to some index
 * list, which is more important than keeping occasionally executed code
 * simple */
struct dsp_proc_slot
{
    struct dsp_proc_entry proc_entry;
    const struct dsp_proc_db_entry *db_entry;
    struct dsp_proc_slot *next[2]; /* [0]=active next, [1]=enabled next */
};

#define SAMPLE_BUF_COUNT 128 /* Per channel, per DSP */

struct dsp_config
{
    /** General DSP-local data **/
    struct sample_format format; /* Next format to be sent */
    int sample_depth;            /* Codec-specified sample depth */
    int stereo_mode;             /* Codec-specified input format */
#ifdef CPU_COLDFIRE
    unsigned long old_macsr;     /* Old macsr value to restore */
#endif
#if 0 /* Not needed now but enable if something must know this */
#ifdef HAVE_PITCHSCREEN
    bool processing;             /* DSP is processing (to thwart inopportune
                                    buffer moves) */
#endif
#endif
    /** Sample input from src buffer **/
    sample_input_fn_type input_samples[2]; /* Input function (always set) */
    struct dsp_buffer sample_buf; /* Buffer descriptor for converted samples */
    int32_t sample_buf_arr[2][SAMPLE_BUF_COUNT]; /* Internal format */

    /** General dependant processing functions **/
    uint32_t slot_free_mask;     /* Mask of free slots */
    uint32_t proc_masks[2];      /* Mask of active/enabled stages */
    struct dsp_proc_slot *proc_slots[2]; /* Pointer to first in list of
                                            active/enabled stages */
    struct dsp_proc_slot *proc_slot_arr; /* Pointer to beginning of pool */

    /** Final sample output **/
    sample_output_fn_type output_samples[2]; /* Output function (always set) */
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

/* Some forward decs */
static void dsp_replaygain_update(struct dsp_config *dsp,
                                  const struct dsp_replay_gains *gains);

#ifdef HAVE_PITCHSCREEN
static void dsp_pitch_update(struct dsp_config *dsp);
#else
#define dsp_pitch_update(dsp) ({ (void)(dsp); })
#endif


/** Processing stages support functions **/

/* Broadcast to all enabled stages */
static void proc_broadcast(struct dsp_config *dsp, enum dsp_settings setting,
                           intptr_t value)
{
    for (struct dsp_proc_slot *s = dsp->proc_slots[1]; s; s = s->next[1])
        s->db_entry->configure(&s->proc_entry, dsp, setting, value);
}

/* Find the slot for a given enabled id */
static struct dsp_proc_slot * find_proc_slot(struct dsp_config *dsp,
                                             enum dsp_proc_ids id)
{
    const uint32_t mask = BIT_N(id);

    if ((dsp->proc_masks[1] & mask) == 0)
        return NULL; /* Not enabled */

    for (struct dsp_proc_slot *s = dsp->proc_slots[1]; s; s = s->next[1])
    {
        if (BIT_N(s->db_entry->id) == mask)
            return s;
    }

    return NULL;
}

/* Add an item to the enabled list */
static struct dsp_proc_slot *
dsp_proc_enable_enlink(struct dsp_config *dsp, uint32_t mask)
{
    int slot = find_first_set_bit(dsp->slot_free_mask);

    if (slot == 32)
    {
        /* Should NOT happen, ever, unless called before init */
        DEBUGF("DSP %d: no slots!\n", (int)dsp_get_id(dsp));
        return NULL;
    }

    int prev_id = -1;

    for (unsigned int i = 0; i < DSP_NUM_PROC_STAGES; i++)
    {
        const struct dsp_proc_db_entry *db_entry = dsp_proc_database[i];
        uint32_t m = BIT_N(db_entry->id);

        if (m == mask)
        {
            struct dsp_proc_slot *s = &dsp->proc_slot_arr[slot];
            struct dsp_proc_slot *prev = prev_id < 0 ?
                NULL : find_proc_slot(dsp, prev_id);

            if (prev)
            {
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
            return s;
        }

        if (dsp->proc_masks[1] & m)
            prev_id = db_entry->id;
    }

    return NULL;
}

/* Remove an item from the enabled list */
static struct dsp_proc_slot *
dsp_proc_enable_delink(struct dsp_config *dsp, uint32_t mask)
{
    struct dsp_proc_slot *s = dsp->proc_slots[1];
    struct dsp_proc_slot *prev = NULL;

    while (s != NULL)
    {
        if (BIT_N(s->db_entry->id) == mask)
        {
            if (prev)
                prev->next[1] = s->next[1];
            else
                dsp->proc_slots[1] = s->next[1];

            return s;
        }

        prev = s;
        s = s->next[1];
    }

    return NULL;
}

void dsp_proc_enable(struct dsp_config *dsp, enum dsp_proc_ids id,
                     bool enable)
{
    const uint32_t mask = BIT_N(id);
    bool enabled = dsp->proc_masks[1] & mask;
    struct dsp_proc_slot *s = NULL;

    if (enable)
    {
        if (!enabled)
        {
            /* Enabled list is built in the same order as the database array */
            s = dsp_proc_enable_enlink(dsp, mask);

            if (s != NULL)
            {
                dsp->proc_masks[1] |= mask;
                dsp->slot_free_mask &= ~BIT_N(s - dsp->proc_slot_arr);

                s->proc_entry.ip_mask = mask;
                s->proc_entry.process[0] = dsp_format_change_process;
                s->proc_entry.process[1] = dsp_format_change_process;
                s->proc_entry.data = 0;
            }
        }
        else
        {
            /* Already enabled - just find it in the list */
            s = find_proc_slot(dsp, id);
        }

        if (s != NULL)
        {
            /* Call to construct the new stage or call init again for repeat
               enable */
            s->db_entry->configure(&s->proc_entry, dsp, DSP_PROC_INIT,
                                   enabled ? 1 : 0);
            return;
        }
    }
    else if (enabled)
    {
        dsp_proc_activate(dsp, id, false); /* deactivate it first */

        s = dsp_proc_enable_delink(dsp, mask);

        if (s != NULL)
        {
            dsp->proc_masks[1] &= ~mask;
            s->db_entry->configure(&s->proc_entry, dsp, DSP_PROC_CLOSE, 0);
            dsp->slot_free_mask |= BIT_N(s - dsp->proc_slot_arr);
            return;
        }
    }

    DEBUGF("DSP: proc id not valid: %d\n", (int)id);
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

/* Call this->process[fmt] according to the rules */
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

/* Generic handler for this->process[1] */
void dsp_format_change_process(struct dsp_proc_entry *this,
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

/** Sample input **/

/* The internal format is 32-bit samples, non-interleaved, stereo. This
 * format is similar to the raw output from several codecs, so no copying is
 * needed for that case.
 *
 * Note that for mono, dst[0] equals dst[1], as there is no point in
 * processing the same data twice nor should it be done when modifying
 * samples in-place.
 *
 * When conversion is required:
 * Updates source buffer to point past the samples "consumed" also consuming
 * that portion of the input buffer and the destination is set to the buffer
 * of samples for later stages to consume.
 *
 * Input operates similarly to how an out-of-place processing stage should
 * behave.
 */

/* convert count 16-bit mono to 32-bit mono */
static void sample_input_mono16(struct dsp_config *dsp,
                                struct dsp_buffer **buf_p)
{
    struct dsp_buffer *src = *buf_p;
    struct dsp_buffer *dst = &dsp->sample_buf;

    *buf_p = dst;

    if (dst->remcount > 0)
        return; /* data still remains */

    int count = MIN(src->remcount, SAMPLE_BUF_COUNT);

    dst->remcount  = count;
    dst->p32[0]    = dsp->sample_buf_arr[0];
    dst->p32[1]    = dsp->sample_buf_arr[0];
    dst->proc_mask = src->proc_mask;

    if (count <= 0)
        return; /* purged sample_buf */

    const int16_t *s = src->pin[0];
    int32_t *d = dst->p32[0];
    const int scale = WORD_SHIFT;

    dsp_advance_buffer_input(src, count, sizeof (int16_t));

    do
    {
        *d++ = *s++ << scale;
    }
    while (--count > 0);
}

/* convert count 16-bit interleaved stereo to 32-bit noninterleaved */
static void sample_input_i_stereo16(struct dsp_config *dsp,
                                    struct dsp_buffer **buf_p)
{
    struct dsp_buffer *src = *buf_p;
    struct dsp_buffer *dst = &dsp->sample_buf;

    *buf_p = dst;

    if (dst->remcount > 0)
        return; /* data still remains */

    int count = MIN(src->remcount, SAMPLE_BUF_COUNT);

    dst->remcount  = count;
    dst->p32[0]    = dsp->sample_buf_arr[0];
    dst->p32[1]    = dsp->sample_buf_arr[1];
    dst->proc_mask = src->proc_mask;

    if (count <= 0)
        return; /* purged sample_buf */

    const int16_t *s = src->pin[0];
    int32_t *dl = dst->p32[0];
    int32_t *dr = dst->p32[1];
    const int scale = WORD_SHIFT;

    dsp_advance_buffer_input(src, count, 2*sizeof (int16_t));

    do
    {
        *dl++ = *s++ << scale;
        *dr++ = *s++ << scale;
    }
    while (--count > 0);
}

/* convert count 16-bit noninterleaved stereo to 32-bit noninterleaved */
static void sample_input_ni_stereo16(struct dsp_config *dsp,
                                     struct dsp_buffer **buf_p)
{
    struct dsp_buffer *src = *buf_p;
    struct dsp_buffer *dst = &dsp->sample_buf;

    *buf_p = dst;

    if (dst->remcount > 0)
        return; /* data still remains */

    int count = MIN(src->remcount, SAMPLE_BUF_COUNT);

    dst->remcount  = count;
    dst->p32[0]    = dsp->sample_buf_arr[0];
    dst->p32[1]    = dsp->sample_buf_arr[1];
    dst->proc_mask = src->proc_mask;

    if (count <= 0)
        return; /* purged sample_buf */

    const int16_t *sl = src->pin[0];
    const int16_t *sr = src->pin[1];
    int32_t *dl = dst->p32[0];
    int32_t *dr = dst->p32[1];
    const int scale = WORD_SHIFT;

    dsp_advance_buffer_input(src, count, sizeof (int16_t));

    do
    {
        *dl++ = *sl++ << scale;
        *dr++ = *sr++ << scale;
    }
    while (--count > 0);
}

/* convert count 32-bit mono to 32-bit mono */
static void sample_input_mono32(struct dsp_config *dsp,
                                struct dsp_buffer **buf_p)
{
    struct dsp_buffer *dst = &dsp->sample_buf;

    if (dst->remcount > 0)
    {
        *buf_p = dst;
        return; /* data still remains */
    }
    /* else no buffer switch */

    struct dsp_buffer *src = *buf_p;
    src->p32[1] = src->p32[0];
}


/* convert count 32-bit interleaved stereo to 32-bit noninterleaved stereo */
static void sample_input_i_stereo32(struct dsp_config *dsp,
                                    struct dsp_buffer **buf_p)
{
    struct dsp_buffer *src = *buf_p;
    struct dsp_buffer *dst = &dsp->sample_buf;

    *buf_p = dst;

    if (dst->remcount > 0)
        return; /* data still remains */

    int count = MIN(src->remcount, SAMPLE_BUF_COUNT);

    dst->remcount  = count;
    dst->p32[0]    = dsp->sample_buf_arr[0];
    dst->p32[1]    = dsp->sample_buf_arr[1];
    dst->proc_mask = src->proc_mask;

    if (count <= 0)
        return; /* purged sample_buf */

    const int32_t *s = src->pin[0];
    int32_t *dl = dst->p32[0];
    int32_t *dr = dst->p32[1];

    dsp_advance_buffer_input(src, count, 2*sizeof (int32_t));

    do
    {
        *dl++ = *s++;
        *dr++ = *s++;
    }
    while (--count > 0);
}

/* convert 32 bit-noninterleaved stereo to 32-bit noninterleaved stereo */
static void sample_input_ni_stereo32(struct dsp_config *dsp,
                                     struct dsp_buffer **buf_p)
{
    struct dsp_buffer *dst = &dsp->sample_buf;

    if (dst->remcount > 0)
        *buf_p = dst; /* data still remains */
    /* else no buffer switch */
}

/* set the to-native sample conversion function based on dsp sample
 * parameters */
static void sample_input_format_change(struct dsp_config *dsp,
                                       struct dsp_buffer **buf_p)
{
    static const sample_input_fn_type fns[STEREO_NUM_MODES][2] =
    {
        [STEREO_INTERLEAVED] =
            { sample_input_i_stereo16,
              sample_input_i_stereo32 },
        [STEREO_NONINTERLEAVED] =
            { sample_input_ni_stereo16,
              sample_input_ni_stereo32 },
        [STEREO_MONO] =
            { sample_input_mono16,
              sample_input_mono32 },
    };

    struct dsp_buffer *src = *buf_p;
    struct dsp_buffer *dst = &dsp->sample_buf;

    if (dst->remcount > 0)
    {
        *buf_p = dst;
        return; /* data still remains */
    }

    DSP_PRINT_FORMAT(DSP Input, -1, src->format);

    /* new format - remember it and pass it along */
    dst->format = src->format;
    dsp->input_samples[0] = fns[dsp->stereo_mode]
                               [dsp->sample_depth > NATIVE_DEPTH ? 1 : 0];

    dsp->input_samples[0](dsp, buf_p);

    if (*buf_p == dst) /* buffer switch? */
        format_change_ack(&src->format);
}

/* discard the sample buffer */
static void sample_input_flush(struct dsp_config *dsp)
{
    dsp->sample_buf.remcount = 0;
}


/** Sample output **/

#if !defined(CPU_COLDFIRE) && !defined(CPU_ARM)
/* write mono internal format to output format */
void sample_output_mono(int count, struct dsp_buffer *src,
                        struct dsp_buffer *dst)
{
    const int32_t *s0 = src->p32[0];
    int16_t *d = dst->p16out;
    int scale = src->format.output_scale;
    int32_t dc_bias = 1L << (scale - 1);

    do
    {
        int32_t lr = clip_sample_16((*s0++ + dc_bias) >> scale);
        *d++ = lr;
        *d++ = lr;
    }
    while (--count > 0);
}

/* write stereo internal format to output format */
void sample_output_stereo(int count, struct dsp_buffer *src,
                          struct dsp_buffer *dst)
{
    const int32_t *s0 = src->p32[0];
    const int32_t *s1 = src->p32[1];
    int16_t *d = dst->p16out;
    int scale = src->format.output_scale;
    int32_t dc_bias = 1L << (scale - 1);

    do
    {
        *d++ = clip_sample_16((*s0++ + dc_bias) >> scale);
        *d++ = clip_sample_16((*s1++ + dc_bias) >> scale);
    }
    while (--count > 0);
}
#endif /* CPU */

/**
 * The "dither" code to convert the 24-bit samples produced by libmad was
 * taken from the coolplayer project - coolplayer.sourceforge.net
 *
 * This function handles mono and stereo outputs.
 */
static struct dither_data
{
    struct dither_state
    {
        long error[3];  /* 00h: error term history */
        long random;    /* 0ch: last random value */
    } state[2];         /* 0=left, 1=right */
    bool enabled;       /* 20h: dithered output enabled */
                        /* 24h */
} dither_data IBSS_ATTR;

void sample_output_dithered(int count, struct dsp_buffer *src,
                            struct dsp_buffer *dst)
{
    int channels = src->format.num_channels;
    int scale = src->format.output_scale;
    int32_t dc_bias = 1L << (scale - 1); /* 1/2 bit of significance */
    int32_t mask = (1L << scale) - 1; /* Mask of bits quantized away */

    for (int ch = 0; ch < channels; ch++)
    {
        struct dither_state *dither = &dither_data.state[ch];

        const int32_t *s = src->p32[ch];
        int16_t *d = &dst->p16out[ch];

        for (int i = 0; i < count; i++, s++, d += 2)
        {
            /* Noise shape and bias (for correct rounding later) */
            int32_t sample = *s;

            sample += dither->error[0] - dither->error[1] + dither->error[2];
            dither->error[2] = dither->error[1];
            dither->error[1] = dither->error[0] / 2;

            int32_t output = sample + dc_bias;

            /* Dither, highpass triangle PDF */
            int32_t random = dither->random*0x0019660dL + 0x3c6ef35fL;
            output += (random & mask) - (dither->random & mask);
            dither->random = random;

            /* Quantize sample to output range */
            output >>= scale;

            /* Error feedback of quantization */
            dither->error[0] = sample - (output << scale);

            /* Clip and store */
            *d = clip_sample_16(output);
        }
    }

    if (channels > 1)
        return;

    /* Have to duplicate left samples into the right channel since
       output is interleaved stereo */
    int16_t *d = dst->p16out;

    do
    {
        int16_t s = *d++;
        *d++ = s;
    }
    while (--count > 0);
}

/**
 * Initialize the output function for settings and format
 */
static void NO_INLINE
sample_output_format_change(struct dsp_config *dsp,
                            struct sample_format *format)
{
    static const sample_output_fn_type fns[2][2] =
    {
        { sample_output_mono,        /* DC-biased quantizing */
          sample_output_stereo },
        { sample_output_dithered,    /* Tri-PDF dithering */
          sample_output_dithered },
    };

    bool dither = dsp == &AUDIO_DSP && dither_data.enabled;
    int channels = format->num_channels;

    DSP_PRINT_FORMAT(DSP Output, -1, *format);

    dsp->output_samples[0] = fns[dither ? 1 : 0][channels - 1];
    format_change_ack(format);
}

static void sample_output_flush(struct dsp_config *dsp)
{
    if (dsp == &AUDIO_DSP)
        memset(dither_data.state, 0, sizeof (dither_data.state));
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
    src->format = dsp->format;
    format_change_ack(&dsp->format);

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
        dsp->input_samples[fmt](dsp, &buf);

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

        /* Convert internal format to output format */

        if (fmt != 0)
            sample_output_format_change(dsp, &buf->format);

        /* Don't overread/write src/destination */
        int outcount = MIN(dst->bufcount, buf->remcount);

        if (outcount <= 0)
            break; /* Output full or purged internal buffers */

        dsp->output_samples[0](outcount, buf, dst);

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
    /** These notify enabled stages **/
    case DSP_RESET:
        /* Reset all sample descriptions to default */
        format_change_set(&dsp->format);
        dsp->format.num_channels = 2;
        dsp->format.frac_bits = WORD_FRACBITS;
        dsp->format.output_scale = WORD_FRACBITS + 1 - NATIVE_DEPTH;
        dsp->format.frequency = NATIVE_FREQUENCY;
        dsp->format.codec_frequency = NATIVE_FREQUENCY;
        dsp->sample_depth = NATIVE_DEPTH;
        dsp->stereo_mode = STEREO_NONINTERLEAVED;
        dsp_pitch_update(dsp);
        dsp_replaygain_update(dsp, NULL);
        break;

    case DSP_SET_FREQUENCY:
    case DSP_SWITCH_FREQUENCY:
        value = value > 0 ? value : NATIVE_FREQUENCY;
        format_change_set(&dsp->format);
        dsp->format.frequency = value;
        dsp->format.codec_frequency = value;
        dsp_pitch_update(dsp);
        break;

    case DSP_SET_SAMPLE_DEPTH:
        format_change_set(&dsp->format);
        dsp->format.frac_bits = value <= NATIVE_DEPTH ? WORD_FRACBITS : value;
        dsp->format.output_scale = dsp->format.frac_bits + 1 - NATIVE_DEPTH;
        dsp->sample_depth = value;
        break;

    case DSP_SET_STEREO_MODE:
        format_change_set(&dsp->format);
        dsp->format.num_channels = value == STEREO_MONO ? 1 : 2;
        dsp->stereo_mode = value;
        break;

    case DSP_FLUSH:
        /* Clears audio data for handling discontinuity */
        sample_input_flush(dsp);
        sample_output_flush(dsp);
        break;

    case DSP_SET_REPLAY_GAIN:
        dsp_replaygain_update(dsp, (struct dsp_replay_gains *)value);
        break;

    default:
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

/** Timestretch/pitch settings **/

#ifdef HAVE_PITCHSCREEN
static int32_t pitch_ratio = PITCH_SPEED_100;

static void dsp_pitch_update(struct dsp_config *dsp)
{
    /* Account for playback speed adjustment when setting dsp->frequency
       if we're called from the main audio thread. Voice playback thread
       does not support this feature. */
    if (dsp != &AUDIO_DSP)
        return;

    dsp->format.frequency =
        (int64_t)pitch_ratio * dsp->format.codec_frequency / PITCH_SPEED_100;
}

int32_t sound_get_pitch(void)
{
    return pitch_ratio;
}

void sound_set_pitch(int32_t percent)
{
    pitch_ratio = percent > 0 ? percent : PITCH_SPEED_100;
    struct dsp_config *dsp = &AUDIO_DSP;
    dsp_configure(dsp, DSP_SWITCH_FREQUENCY, dsp->format.codec_frequency);
}
#endif /* HAVE_PITCHSCREEN */


/** Replaygain settings **/

static struct dsp_replay_gains current_rpgains;

int get_replaygain_mode(bool have_track_gain, bool have_album_gain)
{
    bool track = global_settings.replaygain_type == REPLAYGAIN_TRACK ||
                 (global_settings.replaygain_type == REPLAYGAIN_SHUFFLE &&
                  global_settings.playlist_shuffle);

    return (!track && have_album_gain) ? REPLAYGAIN_ALBUM 
                : (have_track_gain ? REPLAYGAIN_TRACK : -1);
}

static void dsp_replaygain_update(struct dsp_config *dsp,
                                  const struct dsp_replay_gains *gains)
{
    if (dsp != &AUDIO_DSP)
        return;

    if (gains == NULL)
    {
        /* Use defaults */
        memset(&current_rpgains, 0, sizeof (current_rpgains));
        gains = &current_rpgains;
    }
    else
    {
        current_rpgains = *gains; /* Stash settings */
    }

    int32_t gain = PGA_UNITY;

    if (global_settings.replaygain_type != REPLAYGAIN_OFF ||
        global_settings.replaygain_noclip)
    {
        bool track_mode =
            get_replaygain_mode(gains->track_gain != 0,
                                gains->album_gain != 0) == REPLAYGAIN_TRACK;

        int32_t peak = (track_mode || gains->album_peak == 0) ?
            gains->track_peak : gains->album_peak;

        if (global_settings.replaygain_type != REPLAYGAIN_OFF)
        {
            gain = (track_mode || gains->album_gain == 0) ?
                gains->track_gain : gains->album_gain;

            if (global_settings.replaygain_preamp)
            {
                int32_t preamp = get_replaygain_int(
                    global_settings.replaygain_preamp * 10);

                gain = fp_mul(gain, preamp, 24);
            }
        }

        if (gain == 0)
        {
            /* So that noclip can work even with no gain information. */
            gain = PGA_UNITY;
        }

        if (global_settings.replaygain_noclip && peak != 0 &&
            fp_mul(gain, peak, 24) >= PGA_UNITY)
        {
            gain = fp_div(PGA_UNITY, peak, 24);
        }
    }

    pga_set_gain(PGA_REPLAYGAIN, gain);
    pga_enable_gain(PGA_REPLAYGAIN, gain != PGA_UNITY);
}

void dsp_set_replaygain(void)
{
    dsp_replaygain_update(&AUDIO_DSP, &current_rpgains);
}


/** Output settings **/

void dsp_dither_enable(bool enable)
{
    if (enable == dither_data.enabled)
        return;

    struct dsp_config *dsp = &AUDIO_DSP;
    sample_output_flush(dsp);
    dither_data.enabled = enable;
    format_change_set(&dsp->format);
}


/** Firmware callback interface **/

/* Hook back from firmware/ part of audio, which can't/shouldn't call apps/
 * code directly. */
int dsp_callback(int msg, intptr_t param)
{
    switch (msg)
    {
#ifdef HAVE_SW_TONE_CONTROLS
    case DSP_CALLBACK_SET_PRESCALE:
        tone_set_prescale(param);
        break;
    case DSP_CALLBACK_SET_BASS:
        tone_set_bass(param);
        break;
    case DSP_CALLBACK_SET_TREBLE:
        tone_set_treble(param);
        break;
    /* FIXME: This must be done by bottom-level PCM driver so it works with
              all PCM, not here and not in mixer. I won't fully support it
              here with all streams. -- jethead71 */
#ifdef HAVE_SW_VOLUME_CONTROL
    case DSP_CALLBACK_SET_SW_VOLUME:
        if ((global_settings.volume < SW_VOLUME_MAX ||
             global_settings.volume > SW_VOLUME_MIN) &&
             AUDIO_DSP.input_samples)
        {
            int vol_gain = get_replaygain_int(global_settings.volume * 100);
            pga_set_gain(PGA_VOLUME, vol_gain);
        }
        break;
#endif /* HAVE_SW_VOLUME_CONTROL */
#endif /* HAVE_SW_TONE_CONTROLS */
    case DSP_CALLBACK_SET_CHANNEL_CONFIG:
        channel_mode_set_config(param);
        break;
    case DSP_CALLBACK_SET_STEREO_WIDTH:
        channel_mode_custom_set_width(param);
        break;
    default:
        break;
    }
    return 0;
}

/* Do what needs initializing before enable/disable calls can be made.
 * Must be done before changing settings for the first time */
void INIT_ATTR dsp_init(void)
{
    static const struct dsp_setup
    {
        sample_input_fn_type  input_samples;
 //       sample_output_fn_type output_samples;
        uint32_t              slot_free_mask;
        struct dsp_proc_slot *proc_slot_arr;
    } dsp_setups[DSP_COUNT] /* INITDATA_ATTR */ =
    {
        [CODEC_IDX_AUDIO] =
        {
            .input_samples  = sample_input_format_change,
 //           .output_samples = sample_output_format_change,
            .slot_free_mask = BIT_N(DSP_NUM_PROC_STAGES) - 1,
            .proc_slot_arr  = audio_dsp_proc_slots,
        },
        [CODEC_IDX_VOICE] =
        {
            .input_samples  = sample_input_format_change,
 //           .output_samples = sample_output_format_change,
            .slot_free_mask = BIT_N(DSP_VOICE_NUM_PROC_STAGES) - 1,
            .proc_slot_arr  = voice_dsp_proc_slots,
        },
    };

    for (int i = 0; i < DSP_COUNT; i++)
    {
        struct dsp_config *dsp = &dsp_conf[i];

        const struct dsp_setup *setup = &dsp_setups[i];

        dsp->input_samples[1] = setup->input_samples;
 //       dsp->output_samples[1] = setup->output_samples;
        dsp->slot_free_mask = setup->slot_free_mask;
        dsp->proc_slot_arr = setup->proc_slot_arr;

        dsp_configure(dsp, DSP_RESET, 0);
        /* Always enable resampler so that format changes may be monitored and
         * it self-activated when required */
        dsp_proc_enable(dsp, DSP_PROC_RESAMPLE, true);
    }
}
