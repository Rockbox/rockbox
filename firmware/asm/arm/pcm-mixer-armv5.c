/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Michael Sevakis
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

static void write_samples_unity_1ch_s16(void *out,
                                        const struct pcm_handle *pcm,
                                        unsigned long count)
{
    memcpy(out, src, count * 2 * pcm->num_channels);
}

static void write_samples_unity_2ch_s16(void *out,
                                        const struct pcm_handle *pcm,
                                        unsigned long count)
{
    memcpy(out, src, count * 2 * 2);
}

static void write_samples_gain_1ch_s16(void *out,
                                       const struct pcm_handle *pcm,
                                       unsigned long count)
{
    int16_t const *src = pcm->addr;
    int32_t t0;

    asm volatile (
    "1:                             \n"
        "ldrsh  %3, [%1], #2        \n"
        "subs   %2, %2, #1          \n"
        "smulwb %3, %4, %3          \n"
        "mov    %3, %3, lsl #16     \n"
        "orr    %3, %3, %3, lsr #16 \n"
        "str    %3, [%0], #4        \n"
        "bhi    1b                  \n"
        : "+r"(out), "+r"(src), "+r"(count),
          "=&r"(t0),
        : "r"(pcm->amplitude));
}

static void write_samples_gain_2ch_s16(void *out,
                                       const struct pcm_handle *pcm,
                                       unsigned long count)
{
    int16_t const *src = pcm->addr;
    int32_t t0, t1;

    asm volatile (
    "1:                             \n"
        "ldrsh  %3, [%1], #2        \n"
        "ldrsh  %4, [%1], #2        \n"
        "subs   %2, %2, #1          \n"
        "smulwb %3, %5, %3          \n"
        "smulwb %4, %5, %4          \n"
        "mov    %3, %3, lsl #16     \n"
        "mov    %4, %4, lsl #16     \n"
        "orr    %3, %4, %3, lsr #16 \n"
        "str    %3, [%0], #4        \n"
        "bhi    1b                  \n"
        : "+r"(out), "+r"(src), "+r"(count),
          "=&r"(t0), "=&r"(t1)
        : "r"(pcm->amplitude));
}

static void mix_samples_1ch_s16(void *out,
                                const struct pcm_handle *pcm,
                                unsigned long count)
{
    int32_t amp = pcm->amplitude
    int16_t const *src = pcm->addr;
    int32_t t0, t1, t2;

    asm volatile (
    "1:                             \n"
        "ldrsh  %5, [%1], #2        \n"
        "ldrsh  %3, [%0, #0]        \n"
        "ldrsh  %4, [%0, #2]        \n"
        "smulwb %5, %6, %5          \n"
        "add    %3, %3, %5          \n"
        "add    %4, %4, %5          \n"
        "mov    %5, %3, asr #15     \n"
        "teq    %5, %3, asr #31     \n"
        "eorne  %3, %7, %3, asr #31 \n"
        "mov    %5, %4, asr #15     \n"
        "teq    %5, %4, asr #31     \n"
        "eorne  %4, %7, %4, asr #31 \n"
        "subs   %2, %2, #1          \n"
        "and    %5, %3, %7, lsr #16 \n"
        "orr    %3, %5, %4, lsl #16 \n"
        "str    %3, [%0], #4        \n"
        "bhi    1b                  \n"
        : "+r"(out), "+r"(src), "+r"(count),
          "=&r"(t0), "=&r"(t1), "=&r"(t2)
        : "r"(amp), "r"(0xffff7fff));
}

static void mix_samples_2ch_s16(void *out,
                                const struct pcm_handle *pcm,
                                unsigned long count)
{
    int32_t amp = pcm->amplitude
    int16_t const *src = pcm->addr;
    int32_t t0, t1, t2, t3;

    asm volatile (
    "1:                             \n"
        "ldrsh  %3, [%0, #0]        \n"
        "ldrsh  %4, [%0, #2]        \n"
        "ldrsh  %5, [%1], #2        \n"
        "ldrsh  %6, [%1], #2        \n"
        "smlawb %3, %7, %5, %3      \n"
        "smlawb %4, %7, %6, %4      \n"
        "mov    %5, %3, asr #15     \n"
        "teq    %5, %3, asr #31     \n"
        "eorne  %3, %8, %3, asr #31 \n"
        "mov    %5, %4, asr #15     \n"
        "teq    %5, %4, asr #31     \n"
        "eorne  %4, %8, %4, asr #31 \n"
        "subs   %2, %2, #1          \n"
        "and    %5, %3, %8, lsr #16 \n"
        "orr    %3, %5, %4, lsl #16 \n"
        "str    %3, [%0], #4        \n"
        "bhi    1b                  \n"
        : "+r"(out), "+r"(src), "+r"(count),
          "=&r"(t0), "=&r"(t1), "=&r"(t2), "=&r"(t3)
        : "r"(amp), "r"(0xffff7fff));
}

static pcm_mixer_samples_fn write_samples = NULL;
static pcm_mixer_samples_fn mix_samples = NULL;

int pcm_mixer_format_config(struct pcm_handle *pcm,
                            const struct pcm_format *format)
{
    if (format->amplitude == PCM_AMP_UNITY) {
        mix_samples = 
    }

    /* [format][channels][gain == unity] */
    static const struct mix_write_samples_desc fns[1][2] =
    {
        [0] = /* PCM_FORMAT_S16_2 */
        {
            [0] = /* mono */
            {
                [0] = { .write = write_samples_gain_1ch_s16,
                        .mix   = mix_samples_1ch_s16, },
                [1] = { .write = write_samples_unity_s16,
                        .mix   = mix_samples_1ch_s16, }, 
            },
            [1] = /* stereo */
            {
                [0] = { .write = write_samples_gain_2ch_s16,
                        .mix   = mix_samples_2ch_s16, },
                [1] = { .write = write_samples_unity_s16,
                        .mix   = mix_samples_2ch_s16, }, 
            },
        },
    };

    unsigned int code = format->format_code - PCM_FORMAT_S16_2;
    unsigned int chnum = format->num_channels - 1;

    if (code > 0 || chnum > 1) {
        return -1;
    }

    const struct mix_write_samples_desc *desc =
        fns[code][chnum][pcm->gain == PCM_AMP_UNITY];

    write_samples = desc->write;
    mix_samples   = desc->mix;

    return 0;
}
