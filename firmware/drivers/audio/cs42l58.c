/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Driver for Cirrus Logic CS42L58 audio codec
 *
 * Copyright (C) 2026 Viktor Krapivin
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
#include "audio.h"
#include "audiohw.h"
#include "cscodec.h"
#include "pcm_sw_volume.h"

/* The register sequences below are the ones Apple's iPod Nano 4G firmware
 * uses for this codec: 0x02..0x05 power, interface and speed mode, 0x07,
 * 0x08 and 0x0b..0x11 the playback route, 0x1a/0x1b headphone volume and
 * 0x1c/0x1d line out volume (bit 7 mutes, bits 6..0 are dB, two's
 * complement). */

static void update_register(int reg, unsigned char mask, unsigned char value)
{
    cscodec_write(reg, (cscodec_read(reg) & ~mask) | (value & mask));
}

/* Speed mode (0x05 bits 4..0) for each rate with the 6 MHz MCLK */
static const unsigned char clock_values[HW_NUM_FREQ] = {
    [HW_FREQ_8] = 0x1d, [HW_FREQ_11] = 0x1b, [HW_FREQ_12] = 0x19,
    [HW_FREQ_16] = 0x15, [HW_FREQ_22] = 0x13, [HW_FREQ_24] = 0x11,
#ifdef HW_HAVE_32
    [HW_FREQ_32] = 0x0d,
#endif
    [HW_FREQ_44] = 0x0b, [HW_FREQ_48] = 0x09,
};
static int current_freq = -1;
static bool outputs_ready;
static unsigned char output_volume[4] = { 0xc4, 0xc4, 0xc4, 0xc4 };

static void mute_outputs(bool mute)
{
    for (int reg = 0x1a; reg <= 0x1d; reg++)
        cscodec_write(reg, output_volume[reg - 0x1a] | (mute ? 0x80 : 0));
}

static void set_output_volume(int reg, unsigned char volume)
{
    output_volume[reg - 0x1a] = volume;
    cscodec_write(reg, volume | (outputs_ready ? 0 : 0x80));
}

void audiohw_preinit(void)
{
    outputs_ready = false;
    cscodec_power(true);
    cscodec_clock(true);
    cscodec_reset(true);
    udelay(1000);
    cscodec_reset(false);
    udelay(2000);

    /* Select the default rate's speed mode while the codec is still
     * powered down, so the PCM driver's first audiohw_set_frequency()
     * finds it set and does not restart MCLK. */
    update_register(0x05, 0x1f, clock_values[HW_FREQ_DEFAULT]);
    current_freq = HW_FREQ_DEFAULT;

    /* Power up muted at minimum gain. Keep the outputs muted until the
     * playback route and the user's volume have been set in postinit. */
    cscodec_write(0x03, 0xaf);
    update_register(0x04, 0x20, 0x20);
    update_register(0x04, 0x0e, 0);
    update_register(0x07, 0x1c, 0x0c);
    update_register(0x0f, 0x80, 0x80);
    mute_outputs(true);
    update_register(0x02, 0x01, 0);
    udelay(75000);
    update_register(0x05, 0xc0, 0xc0);
}

void audiohw_postinit(void)
{
    /* Route PCM to the headphones with the analog inputs disabled. */
    update_register(0x02, 0xce, 0xce);
    cscodec_write(0x03, 0xaf);
    cscodec_write(0x0b, 0x3d);
    cscodec_write(0x0c, 0x3d);
    update_register(0x08, 0x0f, 0);
    update_register(0x10, 0x80, 0x80);
    update_register(0x11, 0x80, 0x80);
    update_register(0x0f, 0x80, 0x80);
    outputs_ready = true;
    mute_outputs(false);
}

static unsigned char volume_register(int tenth_db)
{
    if (tenth_db < -600)
        return 0x80;
    if (tenth_db > 120)
        tenth_db = 120;
    return (tenth_db / 10) & 0x7f;
}

/* The headphone stage reaches -60 dB. Quieter settings keep it there and
 * attenuate the rest in software; the lowest setting mutes. */
void audiohw_set_volume(int left, int right)
{
    set_output_volume(0x1a, left <= -1000 ? 0x80 : volume_register(MAX(left, -600)));
    set_output_volume(0x1b, right <= -1000 ? 0x80 : volume_register(MAX(right, -600)));
    pcm_set_master_volume(MIN(left + 600, 0), MIN(right + 600, 0));
}

void audiohw_set_lineout_volume(int left, int right)
{
    set_output_volume(0x1c, volume_register(left));
    set_output_volume(0x1d, volume_register(right));
}

void audiohw_enable_lineout(bool enable)
{
    update_register(0x03, 0x0f, enable ? 0x0a : 0x0f);
}

void audiohw_set_frequency(int frequency)
{
    if ((unsigned int)frequency >= HW_NUM_FREQ || frequency == current_freq)
        return;
    current_freq = frequency;
    /* Apple's rate setter restarts MCLK and selects the speed mode
     * without powering down the codec. Its stop/resume power sequence
     * is for suspending playback, not for changing the sample rate. */
    cscodec_clock(false);
    cscodec_clock(true);
    update_register(0x05, 0x1f, clock_values[frequency]);
}

void audiohw_close(void)
{
    outputs_ready = false;
    mute_outputs(true);
    update_register(0x02, 0x01, 0x01);
    update_register(0x04, 0x01, 0x01);
    udelay(1000);
    cscodec_reset(true);
    cscodec_clock(false);
}

#ifdef HAVE_RECORDING
/* Recording stubs: the CS42L58 recording path is not implemented yet */
void audiohw_enable_recording(bool source_mic)
{
    (void)source_mic;
}

void audiohw_disable_recording(void)
{
}

void audiohw_set_recvol(int left, int right, int type)
{
    (void)left;
    (void)right;
    (void)type;
}

void audiohw_set_monitor(bool enable)
{
    (void)enable;
}
#endif /* HAVE_RECORDING */
