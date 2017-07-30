/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
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
#ifndef DSP_PROC_ENTRY_H
#define DSP_PROC_ENTRY_H

#if 0 /* Set to '1' to enable local debug messages */
#include <debug.h>
#else
#undef DEBUGF
#define DEBUGF(...)
#endif

/* Macros to generate the right stuff */
#ifdef DSP_PROC_DB_CREATE
struct dsp_proc_db_entry;

#define DSP_PROC_DB_START
#define DSP_PROC_DB_ITEM(name) \
    extern const struct dsp_proc_db_entry name##_proc_db_entry;
#define DSP_PROC_DB_STOP

/* Create database as externs to be able to build array */
#include "dsp_proc_database.h"

#define DSP_PROC_DB_START \
    static struct dsp_proc_db_entry const * const dsp_proc_database[] = {

#define DSP_PROC_DB_ITEM(name) \
    &name##_proc_db_entry,

#define DSP_PROC_DB_STOP };

/* Create database as array */
#include "dsp_proc_database.h"

/* Number of effects in database - all available in audio DSP */
#define DSP_NUM_PROC_STAGES ARRAYLEN(dsp_proc_database)

/* Number of possible effects for voice DSP */
#ifdef HAVE_SW_TONE_CONTROLS
#define DSP_VOICE_NUM_PROC_STAGES 2 /* resample, tone */
#else
#define DSP_VOICE_NUM_PROC_STAGES 1 /* resample */
#endif

#else /* !DSP_PROC_DB_CREATE */

#ifdef DEBUG
#define DSP_PROC_DB_ENTRY(_name, _configure) \
    const struct dsp_proc_db_entry _name##_proc_db_entry = \
    { .id = DSP_PROC_##_name, .configure = _configure,    \
      .name = #_name };
#else /* !DEBUG */
#define DSP_PROC_DB_ENTRY(_name, _configure) \
    const struct dsp_proc_db_entry _name##_proc_db_entry = \
    { .id = DSP_PROC_##_name, .configure = _configure };
#endif /* DEBUG */

#endif /* DSP_PROC_DB_CREATE */

#define DSP_PROC_DB_START \
    enum dsp_proc_ids             \
    {                             \
        ___DSP_PROC_ID_RESERVED = 0,

#define DSP_PROC_DB_ITEM(name) \
    DSP_PROC_##name,

#define DSP_PROC_DB_STOP };

/* Create database as enums for use as ids */
#include "dsp_proc_database.h"

struct dsp_proc_entry;

#include "dsp_core.h"

/* DSP sample transform function prototype */
typedef void (*dsp_proc_fn_type)(struct dsp_proc_entry *this,
                                 struct dsp_buffer **buf);

/**
 * dsp_proc_entry
 * The structure allocated to every stage when enabled.
 *
 * default settings:
 *  .data    = 0
 *  .process = NULL
 *
 * DSP_PROC_INIT handler just has to change what it needs to change. It may
 * also be modified at any time to implement the stage's demands.
 */
struct dsp_proc_entry
{
    intptr_t         data;    /* 00h: any value, used by asm */
    dsp_proc_fn_type process; /* Processing vector */
};

/* Return values for DSP_PROC_NEW_FORMAT setting */
enum
{
    PROC_NEW_FORMAT_OK = 0,      /* Acks, calls process() (default) */
    PROC_NEW_FORMAT_DEACTIVATED, /* Acks, does not call process() */
    PROC_NEW_FORMAT_TRANSITION   /* Does not ack, calls process() */
};

/* DSP transform configure function prototype */
typedef intptr_t (*dsp_proc_config_fn_type)(struct dsp_proc_entry *this,
                                            struct dsp_config *dsp,
                                            unsigned int setting,
                                            intptr_t value);

/* Enable/disable a processing stage - not to be called during processing
 * by processing code! */
void dsp_proc_enable(struct dsp_config *dsp, enum dsp_proc_ids id,
                     bool enable);

/* Is the specified stage enabled on the DSP? */
bool dsp_proc_enabled(struct dsp_config *dsp, enum dsp_proc_ids id);

/* Activate/deactivate processing stage, doesn't affect enabled status
 * thus will not enable anything -
 * may be called during processing to activate/deactivate for format
 * changes */
void dsp_proc_activate(struct dsp_config *dsp, enum dsp_proc_ids id,
                       bool activate);

/* Is the specified stage active on the DSP? */
bool dsp_proc_active(struct dsp_config *dsp, enum dsp_proc_ids id);

/* Force the specified stage to receive a format update before the next
 * buffer is sent to process() */
void dsp_proc_want_format_update(struct dsp_config *dsp,
                                 enum dsp_proc_ids id);

/* Set or unset in-place operation */
void dsp_proc_set_in_place(struct dsp_config *dsp, enum dsp_proc_ids id,
                           bool in_place);

#define DSP_PRINT_FORMAT(id, format) \
    DEBUGF("DSP format- " #id "\n"                      \
           "  ver:%u ch:%u fb:%u os:%u hz:%u chz:%u\n", \
           (unsigned int)(format).version,              \
           (unsigned int)(format).num_channels,         \
           (unsigned int)(format).frac_bits,            \
           (unsigned int)(format).output_scale,         \
           (unsigned int)(format).frequency,            \
           (unsigned int)(format).codec_frequency);

struct dsp_proc_db_entry
{
    enum dsp_proc_ids id;              /* id of this stage */
    dsp_proc_config_fn_type configure; /* dsp_configure hook */
#ifdef DEBUG
    const char *name;
#endif
};

#endif /* DSP_PROC_ENTRY_H */
