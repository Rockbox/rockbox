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
#include <ctype.h>
#include <stdbool.h>

#define NATIVE_FREQUENCY       44100
#define STEREO_INTERLEAVED     0
#define STEREO_NONINTERLEAVED  1
#define STEREO_MONO            2

struct dsp_configuration {
    long frequency;
    long clip_min, clip_max;
    int sample_depth;
    bool dither_enabled;
    int stereo_mode;
};

extern struct dsp_configuration dsp_config;

int dsp_process(char *dest, char *src, int samplecount);
bool dsp_configure(int setting, void *value);

#endif


