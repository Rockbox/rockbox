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
#ifndef DSP_PROC_SETTINGS_H
#define DSP_PROC_SETTINGS_H

/* Collect all headers together */
#include "channel_mode.h"
#include "compressor.h"
#include "crossfeed.h"
#include "dsp_misc.h"
#include "eq.h"
#include "pga.h"
#ifdef HAVE_PITCHCONTROL
#include "tdspeed.h"
#endif
#ifdef HAVE_SW_TONE_CONTROLS
#include "tone_controls.h"
#endif

#endif /* DSP_PROC_SETTINGS_H */
