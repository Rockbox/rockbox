/***************************************************************************
 * Samsung YP-P2 audio routing.
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
#ifndef YPP2_AUDIO_TARGET_H
#define YPP2_AUDIO_TARGET_H

#include <stdbool.h>

/* External headphone pass switch (schematic net MUTE): high passes audio. */
void ypp2_audio_output_enable(bool enable);

#endif
