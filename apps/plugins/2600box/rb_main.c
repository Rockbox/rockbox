/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2019 Sebastian Leonhardt
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

#include "rbconfig.h"
#include "rb_test.h"

#include "memory.h"
#include "mos6502.h"
#include "raster.h"

#include "rb_menu.h"

#include "keyboard.h"
#include "files.h"
#include "config.h"
#include "vmachine.h"
#include "profile.h"
#include "palette.h"
#include "tia_audio.h"

#include "rb_lcd.h"
#include "lib/helper.h"

#include "rb_main.h"

#ifndef ATARI_NO_SOUND
struct event_queue audiosync SHAREDBSS_ATTR;
#endif


/*
 * functions to pause/resume emulation are used when menu is entered/left
 */
void emulation_pause(void)
{
    backlight_use_settings();

#ifndef ATARI_NO_SOUND
    stop_audio();
#endif
#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(false);
#endif
}

void emulation_resume(void)
{
    backlight_ignore_timeout();

#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(true);
#endif
#ifndef ATARI_NO_SOUND
    if (pcm.audio_on)
        start_audio();
#endif
}


enum plugin_status plugin_start(const void* parameter)
{

    DEBUGF( "\nAtari 2600 Emulator v" VERSION_MAIN "\n"
            "(CPU v" VERSION_CPU "/MEM v" VERSION_MEM
            "/RASTER v" VERSION_RASTER ")\n" );

    TST_INIT_ASSERT();

#if CONFIG_CODEC == SWCODEC && !defined SIMULATOR
    rb->pcm_play_stop();
#endif

    if (parameter == NULL) {
        rb->splashf(3*HZ, "Play *.a26 file!");
        return PLUGIN_OK;
    }

    /* Initialise the 2600 hardware */
    init_hardware();

    /* load cartridge image */
    if (loadCart((char *)parameter)) {
        rb->splash (2*HZ, "Error loading cartridge image.\n");
    }

    /* Cannot be done until file is loaded */
    init_banking();
    init_memory_map();

    setup_palette(TV_NTSC);
    init_rb_screen();
    init_tests(); /* *** */
#ifndef ATARI_NO_SOUND
    rb->queue_init(&audiosync, false);
    atari_pcm_init();
#endif

    emulation_resume();
    start_machine();
    emulation_pause();

#ifndef ATARI_NO_SOUND
    rb->queue_delete(&audiosync);
#endif

    return PLUGIN_OK;
}
