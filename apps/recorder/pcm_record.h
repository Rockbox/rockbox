/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
 * Copyright (C) 2006-2013 by Michael Sevakis
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
#ifndef PCM_RECORD_H
#define PCM_RECORD_H

/** Warnings (recording may continue with possible data loss)
 ** Reset of recording clears, otherwise see notes below
 */

/* PCM buffer has overflowed; PCM samples were dropped */
/* persists until: stop, new file, clear */
#define PCMREC_W_PCM_BUFFER_OVF         0x00000001
/* encoder output buffer has overflowed; encoded data was dropped */
/* persists until: stop, new file, clear */
#define PCMREC_W_ENC_BUFFER_OVF         0x00000002
/* encoder and set/detected sample rates do not match */
/* persists until: rates match, clear */
#define PCMREC_W_SAMPR_MISMATCH         0x00000004
/* PCM frame skipped because of recoverable DMA error */
/* persists until: clear */
#define PCMREC_W_DMA                    0x00000008
/* internal file size limit was reached; encoded data was dropped */
/* persists until: stop, new file, clear */
#define PCMREC_W_FILE_SIZE              0x00000010

/* all warning flags */
#define PCMREC_W_ALL                    0x0000001f

/** Errors (recording should be reset)
 **
 ** Stopping recording if recording clears internally and externally visible
 ** status must be cleared with audio_error_clear()
 ** reset of recording clears
 */

/* DMA callback has reported an error */
#define PCMREC_E_DMA                    0x80010000
/* failed to load encoder */
#define PCMREC_E_LOAD_ENCODER           0x80020000
/* error originating in encoder */
#define PCMREC_E_ENCODER                0x80040000
/* error from encoder setup of stream parameters */
#define PCMREC_E_ENC_SETUP              0x80080000
/* error writing to output stream */
#define PCMREC_E_ENCODER_STREAM         0x80100000
/* I/O error has occurred */
#define PCMREC_E_IO                     0x80200000

/* all error flags */
#define PCMREC_E_ALL                    0x803f0000

/* Functions called by audio_thread.c */
void pcm_rec_error_clear(void);
unsigned int pcm_rec_status(void);
uint32_t pcm_rec_get_warnings(void);
#ifdef HAVE_SPDIF_IN
unsigned long pcm_rec_sample_rate(void);
#endif

void recording_init(void);

/* audio.h contains audio_* recording functions */

#endif /* PCM_RECORD_H */
