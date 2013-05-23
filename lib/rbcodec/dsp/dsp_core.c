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

/* Define LOGF_ENABLE to enable logf output in this file */
/*#define LOGF_ENABLE*/
#include "logf.h"

/* Actually generate the database of stages */
#define DSP_PROC_DB_CREATE
#include "dsp_proc_entry.h"

#ifndef DSP_PROCESS_START
/* These do nothing if not previously defined */
#define DSP_PROCESS_START()
#define DSP_PROCESS_LOOP()
#define DSP_PROCESS_END()
#endif /* !DSP_PROCESS_START */

/* Linked lists give fewer loads in processing loop compared to some index
 * list, which is more important than keeping occasionally executed code
 * simple */

struct dsp_config
{
    /** General DSP-local data **/
    struct sample_io_data io_data;  /* Sample input-output data (first) */
    uint32_t slot_free_mask;        /* Mask of free slots for this DSP */
    uint32_t proc_mask_enabled;     /* Mask of enabled stages */
    uint32_t proc_mask_active;      /* Mask of active stages */
    struct dsp_proc_slot
    {
        struct dsp_proc_entry proc_entry; /* This enabled stage */
        struct dsp_proc_slot *next; /* Next enabled slot */
        uint32_t mask;              /* In place operation mask/flag */
        uint8_t version;            /* Sample format version */
        uint8_t db_index;           /* Index in database array */
    } *proc_slots;                  /* Pointer to first in list of enabled
                                       stages */
};

#define NACT_BIT    BIT_N(___DSP_PROC_ID_RESERVED)

/* Pool of slots for stages - supports 32 or fewer combined as-is atm. */
static struct dsp_proc_slot
dsp_proc_slot_arr[DSP_NUM_PROC_STAGES+DSP_VOICE_NUM_PROC_STAGES] IBSS_ATTR;

/* General DSP config */
static struct dsp_config dsp_conf[DSP_COUNT] IBSS_ATTR;

/** Processing stages support functions **/
static const struct dsp_proc_db_entry *
proc_db_entry(const struct dsp_proc_slot *s)
{
    return dsp_proc_database[s->db_index];
}

/* Find the slot for a given enabled id */
static struct dsp_proc_slot * find_proc_slot(struct dsp_config *dsp,
                                             unsigned int id)
{
    const uint32_t mask = BIT_N(id);

    if (!(dsp->proc_mask_enabled & mask))
        return NULL; /* Not enabled */

    struct dsp_proc_slot *s = dsp->proc_slots;

    while (1) /* In proc_mask_enabled == it must be there */
    {
        if (BIT_N(proc_db_entry(s)->id) == mask)
            return s;

        s = s->next;
    }
}

/* Broadcast to all enabled stages or to the one with the specifically
 * crafted setting */
static intptr_t proc_broadcast(struct dsp_config *dsp, unsigned int setting,
                               intptr_t value)
{
    bool multi = setting < DSP_PROC_SETTING;
    struct dsp_proc_slot *s;

    if (multi)
    {
        /* Message to all enabled stages */
        if (dsp_sample_io_configure(&dsp->io_data, setting, &value))
            return value; /* To I/O only */

        s = dsp->proc_slots;
    }
    else
    {
        /* Message to a particular stage */
        s = find_proc_slot(dsp, setting - DSP_PROC_SETTING);
    }

    while (s != NULL)
    {
        intptr_t ret = proc_db_entry(s)->configure(
                            &s->proc_entry, dsp, setting, value);

        if (!multi)
            return ret;

        s = s->next;
    }

    return 0;
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

    unsigned int db_index = 0, db_index_prev = DSP_NUM_PROC_STAGES;

    /* Order of enabled list is same as DB array */
    while (1)
    {
        uint32_t m = BIT_N(dsp_proc_database[db_index]->id);

        if (m == mask)
            break; /* This is the one */

        if (dsp->proc_mask_enabled & m)
            db_index_prev = db_index;

        if (++db_index >= DSP_NUM_PROC_STAGES)
            return NULL;
    }

    struct dsp_proc_slot *s = &dsp_proc_slot_arr[slot];

    if (db_index_prev < DSP_NUM_PROC_STAGES)
    {
        struct dsp_proc_slot *prev =
            find_proc_slot(dsp, dsp_proc_database[db_index_prev]->id);
        s->next = prev->next;
        prev->next = s;
    }
    else
    {
        s->next = dsp->proc_slots;
        dsp->proc_slots = s;
    }

    s->mask = mask | NACT_BIT;
    s->version = 0;
    s->db_index = db_index;
    dsp->proc_mask_enabled |= mask;
    dsp->slot_free_mask &= ~BIT_N(slot);

    return s;
}

/* Remove an item from the enabled list */
static struct dsp_proc_slot *
dsp_proc_enable_delink(struct dsp_config *dsp, uint32_t mask)
{
    struct dsp_proc_slot *s = dsp->proc_slots;
    struct dsp_proc_slot *prev = NULL;

    while (1) /* In proc_mask_enabled == it must be there */
    {
        if (BIT_N(proc_db_entry(s)->id) == mask)
        {
            if (prev)
                prev->next = s->next;
            else
                dsp->proc_slots = s->next;

            dsp->proc_mask_enabled &= ~mask;
            dsp->slot_free_mask |= BIT_N(s - dsp_proc_slot_arr);
            return s;
        }

        prev = s;
        s = s->next;
    }
}

void dsp_proc_enable(struct dsp_config *dsp, enum dsp_proc_ids id,
                     bool enable)
{
    const uint32_t mask = BIT_N(id);
    bool enabled = dsp->proc_mask_enabled & mask;

    if (enable)
    {
        /* If enabled, just find it in list, if not, link a new one */
        struct dsp_proc_slot *s = enabled ? find_proc_slot(dsp, id) :
                                            dsp_proc_enable_enlink(dsp, mask);

        if (s == NULL)
        {
            DEBUGF("DSP- proc id not valid: %d\n", (int)id);
            return;
        }

        if (!enabled)
        {
            /* New entry - set defaults */
            s->proc_entry.data = 0;
            s->proc_entry.process = NULL;
        }

        enabled = proc_db_entry(s)->configure(&s->proc_entry, dsp,
                                              DSP_PROC_INIT, enabled) >= 0;
        if (enabled)
            return;

        DEBUGF("DSP- proc init failed: %d\n", (int)id);
        /* Cleanup below */
    }
    else if (!enabled)
    {
        return; /* No change */
    }

    dsp_proc_activate(dsp, id, false); /* Deactivate it first */
    struct dsp_proc_slot *s = dsp_proc_enable_delink(dsp, mask);
    proc_db_entry(s)->configure(&s->proc_entry, dsp, DSP_PROC_CLOSE, 0);
}

/* Is the stage specified by the id currently enabled? */
bool dsp_proc_enabled(struct dsp_config *dsp, enum dsp_proc_ids id)
{
    return (dsp->proc_mask_enabled & BIT_N(id)) != 0;
}

/* Activate or deactivate a stage */
void dsp_proc_activate(struct dsp_config *dsp, enum dsp_proc_ids id,
                       bool activate)
{
    const uint32_t mask = BIT_N(id);

    if (!(dsp->proc_mask_enabled & mask))
        return; /* Not enabled */

    if (activate != !(dsp->proc_mask_active & mask))
        return; /* No change in state */

    struct dsp_proc_slot *s = find_proc_slot(dsp, id);

    if (activate)
    {
        dsp->proc_mask_active |= mask;
        s->mask &= ~NACT_BIT;
    }        
    else
    {
        dsp->proc_mask_active &= ~mask;
        s->mask |= NACT_BIT;
    }
}

/* Is the stage specified by the id currently active? */
bool dsp_proc_active(struct dsp_config *dsp, enum dsp_proc_ids id)
{
    return (dsp->proc_mask_active & BIT_N(id)) != 0;
}

/* Force the specified stage to receive a format update before the next
 * buffer is sent to process() */
void dsp_proc_want_format_update(struct dsp_config *dsp,
                                 enum dsp_proc_ids id)
{
    struct dsp_proc_slot *s = find_proc_slot(dsp, id);

    if (s)
        s->version = 0; /* Set invalid */
}

/* Set or unset in-place operation */
void dsp_proc_set_in_place(struct dsp_config *dsp, enum dsp_proc_ids id,
                           bool in_place)
{
    struct dsp_proc_slot *s = find_proc_slot(dsp, id);

    if (!s)
        return;

    const uint32_t mask = BIT_N(id);

    if (in_place)
        s->mask |= mask;
    else
        s->mask &= ~mask;
}

/* Determine by the rules if the processing function should be called */
static NO_INLINE bool dsp_proc_new_format(struct dsp_proc_slot *s,
                                          struct dsp_config *dsp,
                                          struct dsp_buffer *buf)
{
    struct dsp_proc_entry *this = &s->proc_entry;
    struct sample_format *format = &buf->format;

    switch (proc_db_entry(s)->configure(
                this, dsp, DSP_PROC_NEW_FORMAT, (intptr_t)format))
    {
    case PROC_NEW_FORMAT_OK:
        s->version = format->version;
        return true;

    case PROC_NEW_FORMAT_TRANSITION:
        return true;

    case PROC_NEW_FORMAT_DEACTIVATED:
        s->version = format->version;
        return false;

    default:
        return false;
    }
}

static FORCE_INLINE void dsp_proc_call(struct dsp_proc_slot *s,
                                       struct dsp_config *dsp,
                                       struct dsp_buffer **buf_p)
{
    struct dsp_buffer *buf = *buf_p;

    if (UNLIKELY(buf->format.version != s->version))
    {
        if (!dsp_proc_new_format(s, dsp, buf))
            return;
    }

    if (s->mask)
    {
        if ((s->mask & (buf->proc_mask | NACT_BIT)) || buf->remcount <= 0)
            return;

        buf->proc_mask |= s->mask;
    }

    s->proc_entry.process(&s->proc_entry, buf_p);
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

    DSP_PROCESS_START();

    /* Tag input with codec-specified sample format */
    src->format = dsp->io_data.format;

    if (src->format.version != dsp->io_data.sample_buf.format.version)
        dsp_sample_input_format_change(&dsp->io_data, &src->format);

    while (1)
    {
        /* Out-of-place-processing stages take the current buf as input
         * and switch the buffer to their own output buffer */
        struct dsp_buffer *buf = src;

        /* Convert input samples to internal format */
        dsp->io_data.input_samples(&dsp->io_data, &buf);

        /* Call all active/enabled stages depending if format is
           same/changed on the last output buffer */
        for (struct dsp_proc_slot *s = dsp->proc_slots; s; s = s->next)
            dsp_proc_call(s, dsp, &buf);

        /* Don't overread/write src/destination */
        int outcount = MIN(dst->bufcount, buf->remcount);

        if (outcount <= 0)
            break; /* Output full or purged internal buffers */

        if (UNLIKELY(buf->format.version != dsp->io_data.output_version))
            dsp_sample_output_format_change(&dsp->io_data, &buf->format);

        dsp->io_data.outcount = outcount;
        dsp->io_data.output_samples(&dsp->io_data, buf, dst);

        /* Advance buffers by what output consumed and produced */
        dsp_advance_buffer32(buf, outcount);
        dsp_advance_buffer_output(dst, outcount);

        DSP_PROCESS_LOOP();
    } /* while */

    DSP_PROCESS_END();
}

intptr_t dsp_configure(struct dsp_config *dsp, unsigned int setting,
                       intptr_t value)
{
    return proc_broadcast(dsp, setting, value);
}

struct dsp_config * dsp_get_config(enum dsp_ids id)
{
    if (id >= DSP_COUNT)
        return NULL;

    return &dsp_conf[id];
}

/* Return the id given a dsp pointer (or even via something within
   the struct itself) */
enum dsp_ids dsp_get_id(const struct dsp_config *dsp)
{
    ptrdiff_t id = dsp - dsp_conf;

    if (id < 0 || id >= DSP_COUNT)
        return DSP_COUNT; /* obviously invalid */

    return (enum dsp_ids)id;
}

/* Do what needs initializing before enable/disable calls can be made.
 * Must be done before changing settings for the first time. */
void INIT_ATTR dsp_init(void)
{
    static const uint8_t slot_count[DSP_COUNT] INITDATA_ATTR =
    {
        [CODEC_IDX_AUDIO] = DSP_NUM_PROC_STAGES,
        [CODEC_IDX_VOICE] = DSP_VOICE_NUM_PROC_STAGES
    };

    for (unsigned int i = 0, count, shift = 0;
         i < DSP_COUNT;
         i++, shift += count)
    {
        struct dsp_config *dsp = &dsp_conf[i];

        count = slot_count[i];
        dsp->slot_free_mask = MASK_N(uint32_t, count, shift);

        intptr_t value = i;
        dsp_sample_io_configure(&dsp->io_data, DSP_INIT, &value);

        /* Notify each db entry of global init for each DSP */
        for (unsigned int j = 0; j < DSP_NUM_PROC_STAGES; j++)
            dsp_proc_database[j]->configure(NULL, dsp, DSP_INIT, i);

        dsp_configure(dsp, DSP_RESET, 0);
    }
}
