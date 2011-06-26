/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Yoshihisa Uchida
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
#ifndef CODEC_LIBPCM_IMA_ADPCM_COMMON_H
#define CODEC_LIBPCM_IMA_ADPCM_COMMON_H

#include <stdbool.h>
#include <inttypes.h>

#define IMA_ADPCM_INC_DEPTH (PCM_OUTPUT_DEPTH - 16)

void init_ima_adpcm_decoder(int bit, const int *index_table);
void set_decode_parameters(int channels, int32_t *init_pcmdata, int8_t *init_index);
int16_t create_pcmdata(int ch, uint8_t nibble);
int16_t create_pcmdata_size4(int ch, uint8_t nibble);
#endif
