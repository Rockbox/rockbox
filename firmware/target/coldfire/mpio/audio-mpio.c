/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2010 Marcin Bukat
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
#include "system.h"
#include "cpu.h"
#include "audio.h"
#include "sound.h"

void audio_set_output_source(int source)
{

    (void)source;
    int level = set_irq_level(DMA_IRQ_LEVEL);

    /* PDOR3 */
    IIS2CONFIG = (IIS2CONFIG & ~(7 << 8)) | (3 << 8);

    restore_irq(level);
}

void audio_input_mux(int source, unsigned flags)
{
    (void)source;
    (void)flags;

    switch(source)
    {
        case AUDIO_SRC_FMRADIO:
        break;
    }
    /* empty stub */
}
