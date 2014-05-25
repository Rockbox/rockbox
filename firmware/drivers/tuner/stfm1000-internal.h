/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Amaury Pouly
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
#ifndef __STFM1000_INTERNAL_H__
#define __STFM1000_INTERNAL_H__

/** API with stmf1000-audio.c */
/* one time init */
void stfm1000_audio_init(void);
/* start/stop streaming and playing */
void stfm1000_audio_run(bool en);
/* soft-(un)mute audio, returns previous value */
bool stfm1000_audio_softmute(bool mute);
/* (un)force mono */
void stfm1000_audio_set_mono(bool force_mono);
/* enable/disable raw stream: audio stream is replaced by L+R, L-R, RDS and RSSI */
void stfm1000_audio_set_raw_stream(int raw_stream);

#endif /* __STFM1000_INTERNAL_H__ */
