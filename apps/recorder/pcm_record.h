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

/** General functions for high level codec recording **/
/* pcm_rec_error_clear is deprecated for general use. audio_error_clear
   should be used */
void pcm_rec_error_clear(void);
/* pcm_rec_status is deprecated for general use. audio_status merges the
   results for consistency with the hardware codec version */
unsigned long pcm_rec_status(void);
unsigned long pcm_rec_get_warnings(void);
void pcm_rec_init(void);
int  pcm_rec_current_bitrate(void);
int  pcm_rec_encoder_afmt(void); /* AFMT_* value, AFMT_UNKNOWN if none */
int  pcm_rec_rec_format(void);   /* Format index or -1 otherwise */
unsigned long pcm_rec_sample_rate(void);
int  pcm_get_num_unprocessed(void);

/* audio.h contains audio_* recording functions */

#endif /* PCM_RECORD_H */
