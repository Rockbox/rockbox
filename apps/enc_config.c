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
#include <stdio.h>
#include <sprintf.h>
#include <string.h>
#include "config.h"
#include "atoi.h"
#include "lang.h"
#include "misc.h"
#include "talk.h"
#include "general.h"
#include "codecs.h"
#include "menu.h"
#include "statusbar.h"
#include "settings.h"
#include "audio.h"
#include "pcm_record.h"
#include "enc_config.h"

#define MENU_ITEM_FN(fn) \
    ((bool (*)(void))fn)

#define ENC_MENU_ITEM_FN(fn) \
    ((bool (*)(struct encoder_config *))fn)

#define CALL_FN_(fn, ...) \
    if (fn) fn(__VA_ARGS__)

static bool enc_run_menu(int m, const struct menu_item items[],
                         struct encoder_config *cfg);
static bool enc_no_config_menu(struct encoder_config *cfg);

/** Function definitions for each codec - add these to enc_data
    list following the definitions **/

/** aiff_enc.codec **/

/** mp3_enc.codec **/
/* mp3_enc: return encoder capabilities */
static void mp3_enc_get_caps(const struct encoder_config *cfg,
                             struct encoder_caps *caps,
                             bool for_config)
{
    int i;
    unsigned long bitr;

    if (!for_config)
    {
        /* Overall encoder capabilities */
        caps->samplerate_caps = MPEG1_SAMPR_CAPS | MPEG2_SAMPR_CAPS;
        caps->channel_caps    = CHN_CAP_ALL;
        return;
    }

    /* Restrict caps based on config */
    i = round_value_to_list32(cfg->mp3_enc.bitrate, mp3_enc_bitr,
                              MP3_ENC_NUM_BITR, false);
    bitr = mp3_enc_bitr[i];

    /* sample rate caps */

    /* check if MPEG1 sample rates are available */
    if ((bitr >= 32 && bitr <= 128) || bitr >= 160)
        caps->samplerate_caps |= MPEG1_SAMPR_CAPS;

    /* check if MPEG2 sample rates and mono are available */
    if (bitr <= 160)
    {
        caps->samplerate_caps |= MPEG2_SAMPR_CAPS;
        caps->channel_caps |= CHN_CAP_MONO;
    }

    /* check if stereo is available */
    if (bitr >= 32)
        caps->channel_caps |= CHN_CAP_STEREO;
} /* mp3_enc_get_caps */

/* mp3_enc: return the default configuration */
static void mp3_enc_default_config(struct encoder_config *cfg)
{
    cfg->mp3_enc.bitrate = 128; /* default that works for all types */
} /* mp3_enc_default_config */

static void mp3_enc_convert_config(struct encoder_config *cfg,
                                   bool to_encoder)
{
    if (to_encoder)
    {
        if ((unsigned)global_settings.mp3_enc_config.bitrate > MP3_ENC_NUM_BITR)
            global_settings.mp3_enc_config.bitrate = MP3_ENC_BITRATE_CFG_DEFAULT;
        cfg->mp3_enc.bitrate = mp3_enc_bitr[global_settings.mp3_enc_config.bitrate];
    }
    else
    {
        global_settings.mp3_enc_config.bitrate =
            round_value_to_list32(cfg->mp3_enc.bitrate, mp3_enc_bitr,
                                  MP3_ENC_NUM_BITR, false);
    }
} /* mp3_enc_convert_config */

/* mp3_enc: show the bitrate setting options */
static bool mp3_enc_bitrate(struct encoder_config *cfg)
{
    static const struct opt_items items[] =
    {
                            /* Available in MPEG Version: */
#ifdef HAVE_MPEG2_SAMPR
#if 0
        /* this sounds awful no matter what */
        { "8 kBit/s",   TALK_ID(8,   UNIT_KBIT) }, /*   2 */
#endif
        /* mono only */
        { "16 kBit/s",  TALK_ID(16,  UNIT_KBIT) }, /*   2 */
        { "24 kBit/s",  TALK_ID(24,  UNIT_KBIT) }, /*   2 */
#endif
        /* stereo/mono */
        { "32 kBit/s",  TALK_ID(32,  UNIT_KBIT) }, /* 1,2 */
        { "40 kBit/s",  TALK_ID(40,  UNIT_KBIT) }, /* 1,2 */
        { "48 kBit/s",  TALK_ID(48,  UNIT_KBIT) }, /* 1,2 */
        { "56 kBit/s",  TALK_ID(56,  UNIT_KBIT) }, /* 1,2 */
        { "64 kBit/s",  TALK_ID(64,  UNIT_KBIT) }, /* 1,2 */
        { "80 kBit/s",  TALK_ID(80,  UNIT_KBIT) }, /* 1,2 */
        { "96 kBit/s",  TALK_ID(96,  UNIT_KBIT) }, /* 1,2 */
        { "112 kBit/s", TALK_ID(112, UNIT_KBIT) }, /* 1,2 */
        { "128 kBit/s", TALK_ID(128, UNIT_KBIT) }, /* 1,2 */
#if 0
        /* oddball MPEG2-only rate stuck in the middle */
        { "144 kBit/s", TALK_ID(144, UNIT_KBIT) }, /*   2 */
#endif
        { "160 kBit/s", TALK_ID(160, UNIT_KBIT) }, /* 1,2 */
        /* stereo only */
        { "192 kBit/s", TALK_ID(192, UNIT_KBIT) }, /* 1   */
        { "224 kBit/s", TALK_ID(224, UNIT_KBIT) }, /* 1   */
        { "256 kBit/s", TALK_ID(256, UNIT_KBIT) }, /* 1   */
        { "320 kBit/s", TALK_ID(320, UNIT_KBIT) }, /* 1   */
    };

    unsigned long rate_list[ARRAYLEN(items)];

    /* This is rather constant based upon the build but better than
       storing and maintaining yet another list of numbers */
    int n_rates = make_list_from_caps32(
            MPEG1_BITR_CAPS | MPEG2_BITR_CAPS, mp3_enc_bitr,
            MPEG1_BITR_CAPS
#ifdef HAVE_MPEG2_SAMPR
            | (MPEG2_BITR_CAPS & ~(MP3_BITR_CAP_144 | MP3_BITR_CAP_8)),
#endif
            rate_list);

    int index = round_value_to_list32(cfg->mp3_enc.bitrate, rate_list,
                                      n_rates, false);
    bool res = set_option(str(LANG_BITRATE), &index, INT,
                          items, n_rates, NULL);
    index = round_value_to_list32(rate_list[index], mp3_enc_bitr,
                                  MP3_ENC_NUM_BITR, false);
    cfg->mp3_enc.bitrate = mp3_enc_bitr[index];

    return res;
} /* mp3_enc_bitrate */

/* mp3_enc: show the configuration menu */
static bool mp3_enc_menu(struct encoder_config *cfg)
{
    static const struct menu_item items[] =
    {
        { ID2P(LANG_BITRATE), MENU_ITEM_FN(mp3_enc_bitrate) }
    };

    bool result;
    int m = menu_init(items, ARRAYLEN(items), NULL, NULL, NULL, NULL);
    result = enc_run_menu(m, items, cfg);
    menu_exit(m);
    return result;
} /* mp3_enc_menu */

/** wav_enc.codec **/
/* wav_enc: show the configuration menu */
#if 0
static bool wav_enc_menu(struct encoder_config *cfg);
#endif

/** wavpack_enc.codec **/
/* wavpack_enc: show the configuration menu */
#if 0
static bool wavpack_enc_menu(struct encoder_config *cfg);
#endif

/** config function pointers and/or data for each codec **/
static const struct encoder_data
{
    void   (*get_caps)(const struct encoder_config *, struct encoder_caps *,
                       bool);
    void   (*default_cfg)(struct encoder_config *);
    void   (*convert_cfg)(struct encoder_config *, bool to_encoder);
    bool   (*menu)(struct encoder_config *);
} enc_data[REC_NUM_FORMATS] =
{
    /* aiff_enc.codec */
    [REC_FORMAT_AIFF] = {
        NULL,
        NULL,
        NULL,
        enc_no_config_menu,
    },
    /* mp3_enc.codec */
    [REC_FORMAT_MPA_L3] = {
        mp3_enc_get_caps,
        mp3_enc_default_config,
        mp3_enc_convert_config,
        mp3_enc_menu,
    },
    /* wav_enc.codec */
    [REC_FORMAT_PCM_WAV] = {
        NULL,
        NULL,
        NULL,
        enc_no_config_menu,
    },
    /* wavpack_enc.codec */
    [REC_FORMAT_WAVPACK] = {
        NULL,
        NULL,
        NULL,
        enc_no_config_menu,
    },
};

static inline bool rec_format_ok(int rec_format)
{
    return (unsigned)rec_format < REC_NUM_FORMATS;
}

static bool enc_run_menu(int m, const struct menu_item items[],
                         struct encoder_config *cfg)
{
    int selected;
    while (1)
    {
        switch (selected=menu_show(m))
        {
        case MENU_SELECTED_EXIT:
            return false;

        case MENU_ATTACHED_USB:
            return true;

        default:
            if (items[selected].function &&
                ENC_MENU_ITEM_FN(items[selected].function)(cfg))
                return  true;
            gui_syncstatusbar_draw(&statusbars, true);
        }
    }
} /* enc_run_menu */

/* menu created when encoder has no configuration options */
static bool enc_no_config_menu(struct encoder_config *cfg)
{
    static const struct menu_item items[] =
    {
        { ID2P(LANG_NO_SETTINGS), NULL }
    };
    int m;
    bool result;

    m = menu_init(items, ARRAYLEN(items), NULL, NULL, NULL, NULL);
    result = enc_run_menu(m, items, NULL);
    menu_exit(m);

    return result;
    (void)cfg;
} /* enc_no_config_menu */

/* update settings dependent upon encoder settings */
static void enc_rec_settings_changed(struct encoder_config *cfg)
{
    struct encoder_config enc_config;
    struct encoder_caps caps;
    long table[MAX(CHN_NUM_MODES, REC_NUM_FREQ)];
    int n;

    if (cfg == NULL)
    {
        cfg = &enc_config;
        cfg->rec_format = global_settings.rec_format;
        global_to_encoder_config(cfg);
    }

    /* have to sync other settings when encoder settings change */
    if (!enc_get_caps(cfg, &caps, true))
        return;

    /* rec_channels */
    n = make_list_from_caps32(CHN_CAP_ALL, NULL,
                              caps.channel_caps, table);

    /* no zero check needed: encoder must support at least one
       sample rate that recording supports or it shouldn't be in
       available in the recording options */
    n = round_value_to_list32(global_settings.rec_channels,
                              table, n, true);
    global_settings.rec_channels = table[n];

    /* rec_frequency */
    n = make_list_from_caps32(REC_SAMPR_CAPS, rec_freq_sampr,
                              caps.samplerate_caps, table);

    n = round_value_to_list32(
                rec_freq_sampr[global_settings.rec_frequency],
                table, n, false);

    global_settings.rec_frequency = round_value_to_list32(
            table[n], rec_freq_sampr, REC_NUM_FREQ, false);
} /* enc_rec_settings_changed */

/** public stuff **/
void global_to_encoder_config(struct encoder_config *cfg)
{
    const struct encoder_data *data = &enc_data[cfg->rec_format];
    CALL_FN_(data->convert_cfg, cfg, true);
} /* global_to_encoder_config */

void encoder_config_to_global(const struct encoder_config *cfg)
{
    const struct encoder_data *data = &enc_data[cfg->rec_format];
    CALL_FN_(data->convert_cfg, (struct encoder_config *)cfg, false);
} /* encoder_config_to_global */

bool enc_get_caps(const struct encoder_config *cfg,
                  struct encoder_caps *caps,
                  bool for_config)
{
    /* get_caps expects caps to be zeroed first */
    memset(caps, 0, sizeof (*caps));

    if (!rec_format_ok(cfg->rec_format))
        return false;

    if (enc_data[cfg->rec_format].get_caps)
    {
        enc_data[cfg->rec_format].get_caps(cfg, caps, for_config);
    }
    else
    {
        /* If no function provided...defaults to all */
        caps->samplerate_caps = SAMPR_CAP_ALL;
        caps->channel_caps    = CHN_CAP_ALL;
    }

    return true;
} /* enc_get_caps */

/* Initializes the config struct with default values */
bool enc_init_config(struct encoder_config *cfg)
{
    if (!rec_format_ok(cfg->rec_format))
        return false;
    CALL_FN_(enc_data[cfg->rec_format].default_cfg, cfg);
    return true;
} /* enc_init_config */

/** Encoder Menus **/
bool enc_config_menu(struct encoder_config *cfg)
{
    if (!rec_format_ok(cfg->rec_format))
        return false;
    return enc_data[cfg->rec_format].menu(cfg);
} /* enc_config_menu */

/** Global Settings **/

/* Reset all codecs to defaults */
void enc_global_settings_reset(void)
{
    struct encoder_config cfg;
    cfg.rec_format = 0;

    do
    {
        global_to_encoder_config(&cfg);
        enc_init_config(&cfg);
        encoder_config_to_global(&cfg);
        if (cfg.rec_format == global_settings.rec_format)
            enc_rec_settings_changed(&cfg);
    }
    while (++cfg.rec_format < REC_NUM_FORMATS);
} /* enc_global_settings_reset */

/* Apply new settings */
void enc_global_settings_apply(void)
{
    struct encoder_config cfg;
    if (!rec_format_ok(global_settings.rec_format))
        global_settings.rec_format = REC_FORMAT_DEFAULT;

    cfg.rec_format = global_settings.rec_format;
    global_to_encoder_config(&cfg);
    enc_rec_settings_changed(&cfg);
    encoder_config_to_global(&cfg);
} /* enc_global_settings_apply */

/* Show an encoder's config menu based on the global_settings.
   Modified settings are placed in global_settings.enc_config. */
bool enc_global_config_menu(void)
{
    struct encoder_config cfg;

    bool res;

    if (!rec_format_ok(global_settings.rec_format))
        global_settings.rec_format = REC_FORMAT_DEFAULT;

    cfg.rec_format = global_settings.rec_format;

    global_to_encoder_config(&cfg);

    res = enc_config_menu(&cfg);
    if (!res)
    {
        enc_rec_settings_changed(&cfg);
        encoder_config_to_global(&cfg);
    }

    return res;
} /* enc_global_config_menu */
