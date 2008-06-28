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
#ifndef ENC_CONFIG_H
#define ENC_CONFIG_H

#include "misc.h"
#include "enc_base.h"

/** Capabilities **/

/* Capabilities returned by enc_get_caps that depend upon encoder settings */
struct encoder_caps
{
    unsigned long samplerate_caps;  /* Mask composed of SAMPR_CAP_* flags */
    unsigned long channel_caps;     /* Mask composed of CHN_CAP_* flags   */
};

/* for_config:
 * true-  the capabilities returned should be contextual based upon the
 *        settings in the config structure
 * false- the overall capabilities are being requested
 */
bool enc_get_caps(const struct encoder_config *cfg,
                  struct encoder_caps *caps,
                  bool for_config);

/** Configuration **/

/* These translate to a back between the global format and the per-
   instance format */
void global_to_encoder_config(struct encoder_config *cfg);
void encoder_config_to_global(const struct encoder_config *cfg);

/* Initializes the config struct with default values.
   set afmt member before calling. */
bool enc_init_config(struct encoder_config *cfg);

/** Encoder Menus **/

/* Shows an encoder's config menu given an encoder config returned by one
   of the enc_api functions. Modified settings are not saved to disk but
   instead are placed in the structure. Call enc_save_config to commit
   the data. */
bool enc_config_menu(struct encoder_config *cfg);

/** Global Settings **/

/* Reset all codecs to defaults */
void enc_global_settings_reset(void);

/* Apply new settings */
void enc_global_settings_apply(void);

/* Show an encoder's config menu based on the global_settings.
   Modified settings are placed in global_settings.enc_config. */
bool enc_global_config_menu(void);
#endif /* ENC_CONFIG_H */
