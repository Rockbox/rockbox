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
#include "config.h"
#include "audio.h"

/** aiff_enc.codec **/

/** mp3_enc.codec **/

/* These are in descending order rather than in MPEG frequency index
   order */
const unsigned long mp3_enc_sampr[MP3_ENC_NUM_SAMPR] =
{
    48000, 44100, 32000, /* MPEG 1   */
    24000, 22050, 16000, /* MPEG 2   */
#if 0
    12000, 11025,  8000, /* MPEG 2.5 */
#endif
};

/* All bitrates used in the MPA L3 standard */
const unsigned long mp3_enc_bitr[MP3_ENC_NUM_BITR] =
{
      8,  16,  24,  32,  40,  48,  56,  64,  80,
     96, 112, 128, 144, 160, 192, 224, 256, 320
};

/** wav_enc.codec **/
 
/** wavpack_enc.codec **/

/** public functions **/
