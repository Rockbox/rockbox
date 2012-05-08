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

/****************************************************************************
 * -_-~-_-~-_-~-_-~-_-~-_- Main database of effects _-~-_-~-_-~-_-~-_-~-_-~- 
 *
 * Order is not particularly relevant and has no intended correlation with
 * IDs.
 * 
 * Notable exceptions in ordering:
 *  * Sample input: which is first in line and has special responsibilities
 *    (not an effect per se).
 *  * Anything that depends on the native sample rate must go after the
 *    resampling stage.
 *  * Some bizarre dependency I didn't think of but you decided to implement.
 *  * Sample output: Naturally, this takes the final result and converts it
 *    to the target PCM format (not an effect per se).
 */
DSP_PROC_DB_START
    DSP_PROC_DB_ITEM(MISC_HANDLER)  /* misc stuff (null stage) */
    DSP_PROC_DB_ITEM(PGA)           /* pre-gain amp */
#ifdef HAVE_PITCHCONTROL
    DSP_PROC_DB_ITEM(TIMESTRETCH)   /* time-stretching */
#endif
    DSP_PROC_DB_ITEM(RESAMPLE)      /* resampler providing NATIVE_FREQUENCY */
    DSP_PROC_DB_ITEM(CROSSFEED)     /* stereo crossfeed */
    DSP_PROC_DB_ITEM(EQUALIZER)     /* n-band equalizer */
#ifdef HAVE_SW_TONE_CONTROLS
    DSP_PROC_DB_ITEM(TONE_CONTROLS) /* bass and treble */
#endif
    DSP_PROC_DB_ITEM(CHANNEL_MODE)  /* channel modes */
    DSP_PROC_DB_ITEM(COMPRESSOR)    /* dynamic-range compressor */
DSP_PROC_DB_STOP

/* This file is included multiple times with different macro definitions so
   clean up the current ones */
#undef DSP_PROC_DB_START
#undef DSP_PROC_DB_ITEM
#undef DSP_PROC_DB_STOP
