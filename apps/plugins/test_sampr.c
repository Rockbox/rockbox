/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Michael Sevakis
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
#include "plugin.h"
#include "lib/oldmenuapi.h"

/* This plugin generates a 1kHz tone + noise in order to quickly verify
 * hardware samplerate setup is operating correctly.
 *
 * While switching to different frequencies, the pitch of the tone should
 * remain constant whereas the upper harmonics of the noise should vary
 * with sample rate.
 */

PLUGIN_HEADER
PLUGIN_IRAM_DECLARE;

const struct plugin_api *rb;

static int hw_freq IDATA_ATTR = HW_FREQ_DEFAULT;
static unsigned long hw_sampr IDATA_ATTR = HW_SAMPR_DEFAULT;

static int gen_thread_stack[DEFAULT_STACK_SIZE/sizeof(int)] IBSS_ATTR;
static bool gen_quit IBSS_ATTR;
static unsigned int gen_thread_id;

#define OUTPUT_CHUNK_COUNT (1 << 1)
#define OUTPUT_CHUNK_MASK (OUTPUT_CHUNK_COUNT-1)
#define OUTPUT_CHUNK_SAMPLES 1152
#define OUTPUT_CHUNK_SIZE (OUTPUT_CHUNK_SAMPLES*sizeof(int16_t)*2)
static uint16_t output_buf[OUTPUT_CHUNK_COUNT][OUTPUT_CHUNK_SAMPLES*2]
        __attribute__((aligned(4)));
static int output_head IBSS_ATTR;
static int output_tail IBSS_ATTR;
static int output_step IBSS_ATTR;

static uint32_t gen_phase_step IBSS_ATTR;
static const uint32_t gen_frequency = 1000;

/* fsin shamelessly stolen from signal_gen.c by Thom Johansen (preglow) */

/* Good quality sine calculated by linearly interpolating
 * a 128 sample sine table. First harmonic has amplitude of about -84 dB.
 * phase has range from 0 to 0xffffffff, representing 0 and
 * 2*pi respectively.
 * Return value is a signed value from LONG_MIN to LONG_MAX, representing
 * -1 and 1 respectively. 
 */
static int16_t ICODE_ATTR fsin(uint32_t phase)
{
    /* 128 sixteen bit sine samples + guard point */
    static const int16_t sinetab[129] ICONST_ATTR =
    {
             0,   1607,   3211,   4807,   6392,   7961,   9511,  11038,
         12539,  14009,  15446,  16845,  18204,  19519,  20787,  22004,
         23169,  24278,  25329,  26318,  27244,  28105,  28897,  29621,
         30272,  30851,  31356,  31785,  32137,  32412,  32609,  32727,
         32767,  32727,  32609,  32412,  32137,  31785,  31356,  30851,
         30272,  29621,  28897,  28105,  27244,  26318,  25329,  24278,
         23169,  22004,  20787,  19519,  18204,  16845,  15446,  14009,
         12539,  11038,   9511,   7961,   6392,   4807,   3211,   1607,
             0,  -1607,  -3211,  -4807,  -6392,  -7961,  -9511, -11038,
        -12539, -14009, -15446, -16845, -18204, -19519, -20787, -22004,
        -23169, -24278, -25329, -26318, -27244, -28105, -28897, -29621,
        -30272, -30851, -31356, -31785, -32137, -32412, -32609, -32727,
        -32767, -32727, -32609, -32412, -32137, -31785, -31356, -30851,
        -30272, -29621, -28897, -28105, -27244, -26318, -25329, -24278,
        -23169, -22004, -20787, -19519, -18204, -16845, -15446, -14009,
        -12539, -11038, -9511,   -7961,  -6392,  -4807,  -3211,  -1607,
             0,
    };

    unsigned int pos = phase >> 25;
    unsigned short frac = (phase & 0x01ffffff) >> 9;
    short diff = sinetab[pos + 1] - sinetab[pos];
    
    return sinetab[pos] + (frac*diff >> 16);
}

/* ISR handler to get next block of data */
static void get_more(unsigned char **start, size_t *size)
{
    /* Free previous buffer */
    output_head += output_step;
    output_step = 0;

    *start = (unsigned char *)output_buf[output_head & OUTPUT_CHUNK_MASK];
    *size  = OUTPUT_CHUNK_SIZE;

    /* Keep repeating previous if source runs low */
    if (output_head != output_tail)
        output_step = 1;
}

static void ICODE_ATTR gen_thread_func(void)
{
    uint32_t gen_random = *rb->current_tick;
    uint32_t gen_phase = 0;

    while (!gen_quit)
    {
        int16_t *p = output_buf[output_tail & OUTPUT_CHUNK_MASK];
        int i = OUTPUT_CHUNK_SAMPLES;

        while (output_tail - output_head >= OUTPUT_CHUNK_COUNT)
        {
            rb->sleep(0);
            if (gen_quit)
                return;
        }

        while (--i >= 0)
        {
            int32_t val = fsin(gen_phase);
            int32_t rnd = (int16_t)gen_random;

            gen_random = gen_random*0x0019660dL + 0x3c6ef35fL;

            val = (rnd + 2*val) / 3;

            *p++ = val;
            *p++ = val;

            gen_phase += gen_phase_step;
        }

        output_tail++;

        rb->yield();
    }
}

static void update_gen_step(void)
{
    gen_phase_step = 0x100000000ull*gen_frequency / hw_sampr;
}

static void output_clear(void)
{
    rb->pcm_play_lock();

    rb->memset(output_buf, 0, sizeof (output_buf));
    output_head = 0;
    output_tail = 0;

    rb->pcm_play_unlock();
}

/* Called to switch samplerate on the fly */
static void set_frequency(int index)
{
    hw_freq = index;
    hw_sampr = rb->hw_freq_sampr[index];

    output_clear();
    update_gen_step();

    rb->pcm_set_frequency(hw_sampr);
    rb->pcm_apply_settings();
}

#ifndef HAVE_VOLUME_IN_LIST
static void set_volume(int value)
{
    rb->global_settings->volume = value;
    rb->sound_set(SOUND_VOLUME, value);
}

static void format_volume(char *buf, size_t len, int value, const char *unit)
{
    rb->snprintf(buf, len, "%d %s", rb->sound_val2phys(SOUND_VOLUME, value),
                 rb->sound_unit(SOUND_VOLUME));
    (void)unit;
}
#endif /* HAVE_VOLUME_IN_LIST */

static void play_tone(bool volume_set)
{
    static struct opt_items names[HW_NUM_FREQ] =
    {
        HW_HAVE_96_([HW_FREQ_96] = { "96kHz",     -1 },)
        HW_HAVE_88_([HW_FREQ_88] = { "88.2kHz",   -1 },)
        HW_HAVE_64_([HW_FREQ_64] = { "64kHz",     -1 },)
        HW_HAVE_48_([HW_FREQ_48] = { "48kHz",     -1 },)
        HW_HAVE_44_([HW_FREQ_44] = { "44.1kHz",   -1 },)
        HW_HAVE_32_([HW_FREQ_32] = { "32kHz",     -1 },)
        HW_HAVE_24_([HW_FREQ_24] = { "24kHz",     -1 },)
        HW_HAVE_22_([HW_FREQ_22] = { "22.05kHz",  -1 },)
        HW_HAVE_16_([HW_FREQ_16] = { "16kHz",     -1 },)
        HW_HAVE_12_([HW_FREQ_12] = { "12kHz",     -1 },)
        HW_HAVE_11_([HW_FREQ_11] = { "11.025kHz", -1 },)
        HW_HAVE_8_( [HW_FREQ_8 ] = { "8kHz",      -1 },)
    };

    int freq = hw_freq;

    rb->audio_stop();

#if INPUT_SRC_CAPS != 0
    /* Select playback */
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    rb->pcm_set_frequency(rb->hw_freq_sampr[freq]);

#if INPUT_SRC_CAPS != 0
    /* Recordable targets can play back from other sources */
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif

    gen_quit = false;
    output_clear();
    update_gen_step();

    gen_thread_id = rb->create_thread(gen_thread_func, gen_thread_stack,
                                      sizeof(gen_thread_stack), 0,
                                      "test_sampr generator"
                                      IF_PRIO(, PRIORITY_PLAYBACK)
                                      IF_COP(, CPU));

    rb->pcm_play_data(get_more, NULL, 0);

#ifndef HAVE_VOLUME_IN_LIST
    if (volume_set)
    {
        int volume = rb->global_settings->volume;

        rb->set_int("Volume", NULL, -1, &volume,
                    set_volume, 1, rb->sound_min(SOUND_VOLUME),
                    rb->sound_max(SOUND_VOLUME), format_volume);
    }
    else
#endif /* HAVE_VOLUME_IN_LIST */
    {
        rb->set_option("Sample Rate", &freq, INT, names,
                       HW_NUM_FREQ, set_frequency);
        (void)volume_set;
    }

    gen_quit = true;

    rb->thread_wait(gen_thread_id);

    rb->pcm_play_stop();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    /* restore default - user of apis is responsible for restoring
       default state - normally playback at 44100Hz */
    rb->pcm_set_frequency(HW_FREQ_DEFAULT);
}

/* Tests hardware sample rate switching */
/* TODO: needs a volume control */
enum plugin_status plugin_start(const struct plugin_api *api,
                                const void *parameter)
{
    enum
    {
        __TEST_SAMPR_MENUITEM_FIRST = -1,
#ifndef HAVE_VOLUME_IN_LIST
        MENU_VOL_SET,
#endif /* HAVE_VOLUME_IN_LIST */
        MENU_SAMPR_SET,
        MENU_QUIT,
    };

    static const struct menu_item items[] =
    {
#ifndef HAVE_VOLUME_IN_LIST
        [MENU_VOL_SET] =
            { "Set Volume",     NULL },
#endif /* HAVE_VOLUME_IN_LIST */
        [MENU_SAMPR_SET] =
            { "Set Samplerate", NULL },
        [MENU_QUIT] =
            { "Quit",           NULL },
    };

    bool exit = false;
    int m;

    /* Disable all talking before initializing IRAM */
    api->talk_disable(true);

    PLUGIN_IRAM_INIT(api);
    rb = api;

    m = menu_init(rb, items, ARRAYLEN(items),
                      NULL, NULL, NULL, NULL);

    while (!exit)
    {
        int result = menu_show(m);

        switch (result)
        {
#ifndef HAVE_VOLUME_IN_LIST
        case MENU_VOL_SET:
            play_tone(true);
            break;
#endif /* HAVE_VOLUME_IN_LIST */
        case MENU_SAMPR_SET:
            play_tone(false);
            break;

        case MENU_QUIT:
            exit = true;
            break;
        }
    }

    menu_exit(m);

    rb->talk_disable(false);

    return PLUGIN_OK;
    (void)parameter;
}
