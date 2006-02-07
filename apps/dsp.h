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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _DSP_H
#define _DSP_H

#include <stdlib.h>
#include <stdbool.h>

#define NATIVE_FREQUENCY       44100
#define STEREO_INTERLEAVED     0
#define STEREO_NONINTERLEAVED  1
#define STEREO_MONO            2

enum {
    CODEC_SET_FILEBUF_WATERMARK = 1,
    CODEC_SET_FILEBUF_CHUNKSIZE,
    CODEC_DSP_ENABLE,
    DSP_SET_FREQUENCY,
    DSP_SWITCH_FREQUENCY,
    DSP_SET_CLIP_MIN,
    DSP_SET_CLIP_MAX,
    DSP_SET_SAMPLE_DEPTH,
    DSP_SET_STEREO_MODE,
    DSP_RESET,
    DSP_DITHER,
    DSP_SET_TRACK_GAIN,
    DSP_SET_ALBUM_GAIN,
    DSP_SET_TRACK_PEAK,
    DSP_SET_ALBUM_PEAK,
    DSP_CROSSFEED
};

long dsp_process(char *dest, char *src[], long size);
long dsp_input_size(long size);
long dsp_output_size(long size);
int dsp_stereo_mode(void);
bool dsp_configure(int setting, void *value);
void dsp_set_replaygain(bool always);
void dsp_set_crossfeed(bool enable);
void dsp_eq_update_data(bool enabled);
void sound_set_pitch(int r);
int sound_get_pitch(void);
#endif
