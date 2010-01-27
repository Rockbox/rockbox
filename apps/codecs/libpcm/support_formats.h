/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Yoshihisa Uchida
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
#ifndef CODEC_LIBPCMS_SUPPORT_FORMATS_H
#define CODEC_LIBPCMS_SUPPORT_FORMATS_H

#include "pcm_common.h"

/* Linear PCM */
const struct pcm_codec *get_linear_pcm_codec(void);

/* ITU-T G.711 A-law */
const struct pcm_codec *get_itut_g711_alaw_codec(void);

/* ITU-T G.711 mu-law */
const struct pcm_codec *get_itut_g711_mulaw_codec(void);

/* Intel DVI ADPCM */
const struct pcm_codec *get_dvi_adpcm_codec(void);

/* IEEE float */
const struct pcm_codec *get_ieee_float_codec(void);
#endif
