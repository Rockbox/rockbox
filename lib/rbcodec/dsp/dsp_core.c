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
#include "platform.h"
#include "dsp_core.h"
#include "dsp_sample_io.h"
#include <sys/types.h>

/* Define LOGF_ENABLE to enable logf output in this file */
/*#define LOGF_ENABLE*/
#include "logf.h"

/* Actually generate the database of stages */
#define DSP_PROC_DB_CREATE
#include "dsp_proc_entry.h"

/* Linked lists give fewer loads in processing loop compared to some index
 * list, which is more important than keeping occasionally executed code
 * simple */

struct dsp_config
{
    /** General DSP-local data **/
    struct sample_io_data io_data; /* Sample input-output data (first) */
    uint32_t slot_free_mask;       /* Mask of free slots for this DSP */
    uint32_t proc_masks[2];        /* Mask of active/enabled stages */
    struct dsp_proc_slot
    {
        struct dsp_proc_entry proc_entry; /* This enabled stage */
        struct dsp_proc_slot *next[2]; /* [0]=active next, [1]=enabled next */
        const struct dsp_proc_db_entry *db_entry;
    } *proc_slots[2];              /* Pointer to first in list of
                                      active/enabled stages */

    /** Misc. extra stuff **/
#if 0 /* Not needed now but enable if something must know this */
    bool processing;               /* DSP is processing (to thwart inopportune
                                      buffer moves) */
#endif
};

/* Pool of slots for stages - supports 32 or fewer combined as-is atm. */
static struct dsp_proc_slot
dsp_proc_slot_arr[DSP_NUM_PROC_STAGES+DSP_VOICE_NUM_PROC_STAGES] IBSS_ATTR;

/* General DSP config */
static struct dsp_config dsp_conf[DSP_COUNT] IBSS_ATTR;

/** Processing stages support functions **/

/* Find the slot for a given enabled id */
static struct dsp_proc_slot * find_proc_slot(struct dsp_config *dsp,
                                             unsigned int id)
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

/* Broadcast to all enabled stages or to the one with the specifically
 * crafted setting */
static intptr_t proc_broadcast(struct dsp_config *dsp, unsigned int setting,
                               intptr_t value)
{
    bool multi = setting < DSP_PROC_SETTING;
    struct dsp_proc_slot *s = multi ?
        dsp->proc_slots[1] : find_proc_slot(dsp, setting - DSP_PROC_SETTING);

    while (s != NULL)
    {
        intptr_t ret = s->db_entry->configure(&s->proc_entry, dsp, setting,
                                              value);
        if (!multi)
            return ret;

        s = s->next[1];
    }

    return multi ? 1 : 0;
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
    for (unsigned int i = 0; (dsp = dsp_get_config(i)); i++)
    {
        /* Found one with the id, check if it's this one
           (NULL return doesn't matter) */
        if (&find_proc_slot(dsp, id)->proc_entry == this)
        {
            if (dsp_proc_active(dsp, id))
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

    const struct dsp_proc_db_entry *db_entry_prev = NULL;
    const struct dsp_proc_db_entry *db_entry;

    /* Order of enabled list is same as DB array */
    for (unsigned int i = 0;; i++)
    {
        if (i >= DSP_NUM_PROC_STAGES)
            return NULL;

        db_entry = dsp_proc_database[i];

        uint32_t m = BIT_N(db_entry->id);

        if (m == mask)
            break; /* This is the one */

        if (dsp->proc_masks[1] & m)
            db_entry_prev = db_entry;
    }

    struct dsp_proc_slot *s = &dsp_proc_slot_arr[slot];

    if (db_entry_prev != NULL)
    {
        struct dsp_proc_slot *prev = find_proc_slot(dsp, db_entry_prev->id);
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
            dsp->slot_free_mask |= BIT_N(s - dsp_proc_slot_arr);
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
            DEBUGF("DSP- proc id not valid: %d\n", (int)id);
            return;
        }

        if (!enabled)
        {
            /* New entry - set defaults */
            s->proc_entry.data = 0;
            s->proc_entry.ip_mask = mask;
            s->proc_entry.process[0] = dsp_process_null;
            s->proc_entry.process[1] = dsp_format_change_process;
        }

        enabled = s->db_entry->configure(&s->proc_entry, dsp, DSP_PROC_INIT,
                                         enabled) >= 0;
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
    s->db_entry->configure(&s->proc_entry, dsp, DSP_PROC_CLOSE, 0);
}

/* Maintain the list structure for the active list where each enabled entry
 * has a link to the next active item, even if not active which facilitates
 * switching out of format change mode by a stage during a format change.
 * When that happens, the iterator must jump over inactive but enabled
 * stages after its current position. */
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
                                              unsigned int fmt)
{
    uint32_t ip_mask = this->ip_mask;

    return UNLIKELY(fmt != 0) || /* Also pass override value */
           ip_mask == 0 || /* Not in-place */
           ((ip_mask & buf->proc_mask) == 0 &&
            (buf->proc_mask |= ip_mask, buf->remcount > 0));
}

/* Call this->process[fmt] according to the rules (for external call) */
bool dsp_proc_call(struct dsp_proc_entry *this, struct dsp_buffer **buf_p,
                   unsigned int fmt)
{
    if (dsp_proc_should_call(this, *buf_p, fmt))
    {
        this->process[fmt == (0u-1u) ? 0 : fmt](this, buf_p);
        return true;
    }

    return false;
}

#ifndef DSP_PROCESS_START
/* These do nothing if not previously defined */
#define DSP_PROCESS_START()
#define DSP_PROCESS_LOOP()
#define DSP_PROCESS_END()
#endif /* !DSP_PROCESS_START */

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
#if 0 /* Not needed now but enable if something must know this */
    dsp->processing = true;
#endif

    /* Tag input with codec-specified sample format */
    src->format = dsp->io_data.format;

    while (1)
    {
        /* Out-of-place-processing stages take the current buf as input
         * and switch the buffer to their own output buffer */
        struct dsp_buffer *buf = src;
        unsigned int fmt = buf->format.changed;

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

        DSP_PROCESS_LOOP();
    } /* while */

#if 0 /* Not needed now but enable if something must know this */
    dsp->process = false;
#endif

    DSP_PROCESS_END();
}

intptr_t dsp_configure(struct dsp_config *dsp, unsigned int setting,
                       intptr_t value)
{
    dsp_sample_io_configure(&dsp->io_data, setting, value);
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

#if 0 /* Not needed now but enable if something must know this */
bool dsp_is_busy(const struct dsp_config *dsp)
{
    return dsp->processing;
}
#endif /* 0 */

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

        dsp_sample_io_configure(&dsp->io_data, DSP_INIT, i);

        /* Notify each db entry of global init for each DSP */
        for (unsigned int j = 0; j < DSP_NUM_PROC_STAGES; j++)
            dsp_proc_database[j]->configure(NULL, dsp, DSP_INIT, i);

        dsp_configure(dsp, DSP_RESET, 0);
    }
}
