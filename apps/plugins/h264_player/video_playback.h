/***************************************************************************
 * Hardware H.264 player implementation.
 *
 * Copyright (C) 2025-2026 David Cormier
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 ****************************************************************************/
#ifndef VIDEO_PLAYBACK_H
#define VIDEO_PLAYBACK_H

#include <stddef.h>

/* Play a validated H.264 Baseline/AAC-LC MP4 from caller-owned memory.
 * The caller must retain the plugin audio buffer until this function returns.
 * Returns 0 at end of stream, 1 on user exit, 2 for USB, or -1 on failure. */
int video_h264_play(const char *filepath, void *buffer, size_t buffer_size);

#endif /* VIDEO_PLAYBACK_H */
