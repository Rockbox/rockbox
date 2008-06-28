/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Michael Sevakis
 *
 * Convert caps masks into HAVE_* defines
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

/** INPUTS **/

/* NOTE: Playback is implied in all this. Make sense? :) */
#define SRC_MIC      0
#define SRC_LINEIN   1
#define SRC_SPDIF    2
#define SRC_FMRADIO  3

#define SRC_CAP_MIC     (1 << SRC_MIC)
#define SRC_CAP_LINEIN  (1 << SRC_LINEIN)
#define SRC_CAP_SPDIF   (1 << SRC_SPDIF)
#define SRC_CAP_FMRADIO (1 << SRC_FMRADIO)

/* audio monitor mux sources */
#ifndef INPUT_SRC_CAPS
#define INPUT_SRC_CAPS 0 /* Nothing but playback */
#endif

/* Microphone */
#if (INPUT_SRC_CAPS & SRC_CAP_MIC)
    #define HAVE_MIC_IN
    #define HAVE_MIC_IN_(...) __VA_ARGS__
#else
    #define HAVE_MIC_IN_(...)
#endif
/* Line In */
#if (INPUT_SRC_CAPS & SRC_CAP_LINEIN)
    #define HAVE_LINE_IN
    #define HAVE_LINE_IN_(...) __VA_ARGS__
#else
    #define HAVE_LINE_IN_(...)
#endif
/* S/PDIF */
#if (INPUT_SRC_CAPS & SRC_CAP_SPDIF)
    #define HAVE_SPDIF_IN
    #define HAVE_SPDIF_IN_(...) __VA_ARGS__
#else
    #define HAVE_SPDIF_IN_(...)
#endif
/* FM Radio */
#if (INPUT_SRC_CAPS & SRC_CAP_FMRADIO)
    #define HAVE_FMRADIO_IN
    #define HAVE_FMRADIO_IN_(...) __VA_ARGS__
#else
    #define HAVE_FMRADIO_IN_(...)
#endif

#if INPUT_SRC_CAPS != 0 && (INPUT_SRC_CAPS & (INPUT_SRC_CAPS-1)) != 0
#define HAVE_MULTI_INPUT_SRC
#endif

#ifdef HAVE_RECORDING
/* Recordable source implies it has the input as well */

/* For now there's no restrictions on any targets with which inputs
   are recordable so define them as equivalent - if they do differ,
   special handling is needed right now. */
#ifndef REC_SRC_CAPS
#define REC_SRC_CAPS INPUT_SRC_CAPS
#endif

/* Microphone */
#if (REC_SRC_CAPS & SRC_CAP_MIC)
    #define HAVE_MIC_REC
    #define HAVE_MIC_REC_(...) __VA_ARGS__
#else
    #define HAVE_MIC_REC_(...)
#endif
/* Line In */
#if (REC_SRC_CAPS & SRC_CAP_LINEIN)
    #define HAVE_LINE_REC
    #define HAVE_LINE_REC_(...) __VA_ARGS__
#else
    #define HAVE_LINE_REC_(...)
#endif
/* S/PDIF */
#if (REC_SRC_CAPS & SRC_CAP_SPDIF)
    #define HAVE_SPDIF_REC
    #define HAVE_SPDIF_REC_(...) __VA_ARGS__
#else
    #define HAVE_SPDIF_REC_(...)
#endif
/* FM Radio */
#if (REC_SRC_CAPS & SRC_CAP_FMRADIO)
    #define HAVE_FMRADIO_REC
    #define HAVE_FMRADIO_REC_(...) __VA_ARGS__
#else
    #define HAVE_FMRADIO_REC_(...)
#endif

#if REC_SRC_CAPS != 0 && (REC_SRC_CAPS & (REC_SRC_CAPS-1)) != 0
#define HAVE_MULTI_REC_SRC
#endif

#endif /* HAVE_RECORDING */
