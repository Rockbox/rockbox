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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "oldmenuapi.h"

PLUGIN_HEADER

const struct plugin_api *rb;

enum
{
    TONE_SINE = 0,
    TONE_TRIANGLE,
    TONE_SAWTOOTH,
    TONE_SQUARE,
    NUM_WAVEFORMS
};

static int freq = HW_FREQ_DEFAULT;
static int waveform = TONE_SINE;

/* A441 at 44100Hz. Pitch will change with changing samplerate.
   Test different waveforms to detect any aliasing in signal which
   indicates duplicated/dropped samples */
static const int16_t A441[NUM_WAVEFORMS][100] =
{
    [TONE_SINE] =
    {
          0,   2057,   4106,   6139,   8148,
      10125,  12062,  13951,  15785,  17557,
      19259,  20886,  22430,  23886,  25247,
      26509,  27666,  28713,  29648,  30465,
      31163,  31737,  32186,  32508,  32702,
      32767,  32702,  32508,  32186,  31737,
      31163,  30465,  29648,  28713,  27666,
      26509,  25247,  23886,  22430,  20886,
      19259,  17557,  15785,  13951,  12062,
      10125,   8148,   6139,   4106,   2057,
          0,  -2057,  -4106,  -6139,  -8148,
     -10125, -12062, -13951, -15785, -17557,
     -19259, -20886, -22430, -23886, -25247,
     -26509, -27666, -28713, -29648, -30465,
     -31163, -31737, -32186, -32508, -32702,
     -32767, -32702, -32508, -32186, -31737,
     -31163, -30465, -29648, -28713, -27666,
     -26509, -25247, -23886, -22430, -20886,
     -19259, -17557, -15785, -13951, -12062,
     -10125,  -8148,  -6139,  -4106,  -2057,
    },
    [TONE_TRIANGLE] =
    {
          0,   1310,   2621,   3932,   5242,
       6553,   7864,   9174,  10485,  11796,
      13106,  14417,  15728,  17038,  18349,
      19660,  20970,  22281,  23592,  24902,
      26213,  27524,  28834,  30145,  31456,
      32767,  31456,  30145,  28834,  27524,
      26213,  24902,  23592,  22281,  20970,
      19660,  18349,  17038,  15728,  14417,
      13106,  11796,  10485,   9174,   7864,
       6553,   5242,   3932,   2621,   1310,
          0,  -1310,  -2621,  -3932,  -5242,
      -6553,  -7864,  -9174, -10485, -11796,
     -13106, -14417, -15728, -17038, -18349,
     -19660, -20970, -22281, -23592, -24902,
     -26213, -27524, -28834, -30145, -31456,
     -32767, -31456, -30145, -28834, -27524,
     -26213, -24902, -23592, -22281, -20970,
     -19660, -18349, -17038, -15728, -14417,
     -13106, -11796, -10485,  -9174,  -7864,
      -6553,  -5242,  -3932,  -2621,  -1310,
    },
    [TONE_SAWTOOTH] =
    {
     -32767, -32111, -31456, -30800, -30145,
     -29490, -28834, -28179, -27524, -26868,
     -26213, -25558, -24902, -24247, -23592,
     -22936, -22281, -21626, -20970, -20315,
     -19660, -19004, -18349, -17694, -17038,
     -16383, -15728, -15072, -14417, -13762,
     -13106, -12451, -11796, -11140, -10485,
      -9830,  -9174,  -8519,  -7864,  -7208,
      -6553,  -5898,  -5242,  -4587,  -3932,
      -3276,  -2621,  -1966,  -1310,   -655,
          0,    655,   1310,   1966,   2621,
       3276,   3932,   4587,   5242,   5898,
       6553,   7208,   7864,   8519,   9174,
       9830,  10485,  11140,  11796,  12451,
      13106,  13762,  14417,  15072,  15728,
      16383,  17038,  17694,  18349,  19004,
      19660,  20315,  20970,  21626,  22281,
      22936,  23592,  24247,  24902,  25558,
      26213,  26868,  27524,  28179,  28834,
      29490,  30145,  30800,  31456,  32111,
    },
    [TONE_SQUARE] =
    {
      32767,  32767,  32767,  32767,  32767,
      32767,  32767,  32767,  32767,  32767,
      32767,  32767,  32767,  32767,  32767,
      32767,  32767,  32767,  32767,  32767,
      32767,  32767,  32767,  32767,  32767,
      32767,  32767,  32767,  32767,  32767,
      32767,  32767,  32767,  32767,  32767,
      32767,  32767,  32767,  32767,  32767,
      32767,  32767,  32767,  32767,  32767,
      32767,  32767,  32767,  32767,  32767,
     -32767, -32767, -32767, -32767, -32767,
     -32767, -32767, -32767, -32767, -32767,
     -32767, -32767, -32767, -32767, -32767,
     -32767, -32767, -32767, -32767, -32767,
     -32767, -32767, -32767, -32767, -32767,
     -32767, -32767, -32767, -32767, -32767,
     -32767, -32767, -32767, -32767, -32767,
     -32767, -32767, -32767, -32767, -32767,
     -32767, -32767, -32767, -32767, -32767,
     -32767, -32767, -32767, -32767, -32767,
    }
};

void play_waveform(void)
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

    /* 50 cycles of wavform */
    static int32_t audio[5000];

    void init_audio(int type)
    {
        int i;
        /* Signal amplitudes to adjust for somewhat equal percieved
           volume */
        int amps[NUM_WAVEFORMS] =
        {
            [TONE_SINE]     = 8191,
            [TONE_TRIANGLE] = 5119,
            [TONE_SAWTOOTH] = 2047,
            [TONE_SQUARE]   = 1535
        };

        /* Initialize one cycle of the waveform */
        for (i = 0; i < 100; i++)
        {
            uint16_t val = amps[type]*A441[type][i]/32767;
            audio[i] = (val << 16) | val;
        }

        /* Duplicate it 49 more times */
        for (i = 1; i < 50; i++)
        {
            rb->memcpy(audio + i*100, audio, 100*sizeof(int32_t));
        }
    }

    /* ISR handler to get next block of data */
    void get_more(unsigned char **start, size_t *size)
    {
        *start = (unsigned char *)audio;
        *size  = sizeof (audio);
    }

    /* Called to switch samplerate on the fly */
    void set_frequency(int index)
    {
        rb->pcm_set_frequency(rb->hw_freq_sampr[index]);
        rb->pcm_apply_settings();
    }

    rb->audio_stop();
    rb->sound_set(SOUND_VOLUME, rb->sound_default(SOUND_VOLUME));

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

    rb->pcm_apply_settings();

    init_audio(waveform);
    rb->pcm_play_data(get_more, NULL, 0);

    rb->set_option("Sample Rate", &freq, INT, names,
                    HW_NUM_FREQ, set_frequency);

    rb->pcm_play_stop();

    while (rb->pcm_is_playing())
        rb->yield();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    /* restore default - user of apis is responsible for restoring
       default state - normally playback at 44100Hz */
    rb->pcm_set_frequency(HW_FREQ_DEFAULT);
}

void set_waveform(void)
{
    static struct opt_items names[NUM_WAVEFORMS] =
    {
        [TONE_SINE]     = { "Sine",     -1 },
        [TONE_TRIANGLE] = { "Triangle", -1 },
        [TONE_SAWTOOTH] = { "Sawtooth", -1 },
        [TONE_SQUARE]   = { "Square",   -1 },
    };

    rb->set_option("Waveform", &waveform, INT, names,
                   NUM_WAVEFORMS, NULL);
}

/* Tests hardware sample rate switching */
/* TODO: needs a volume control */
enum plugin_status plugin_start(const struct plugin_api *api, const void *parameter)
{
    static const struct menu_item items[] =
    {
        { "Set Waveform",  NULL },
        { "Play Waveform", NULL },
        { "Quit",          NULL },
    };

    bool exit = false;
    int m;
    bool talk_menu;

    rb = api;

    /* Have to shut up voice menus or it will mess up our waveform playback */
    talk_menu = rb->global_settings->talk_menu;
    rb->global_settings->talk_menu = false;

    m = menu_init(rb, items, ARRAYLEN(items),
                      NULL, NULL, NULL, NULL);

    while (!exit)
    {
        int result = menu_show(m);

        switch (result)
        {
        case 0:
            set_waveform();
            break;
        case 1:
            play_waveform();
            break;
        case 2:
            exit = true;
            break;
        }
    }

    menu_exit(m);

    rb->global_settings->talk_menu = talk_menu;

    return PLUGIN_OK;
    (void)parameter;
}
