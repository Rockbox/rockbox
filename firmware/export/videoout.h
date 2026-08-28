/***************************************************************************
 * Composite video output interface.
 *
 * Copyright (C) 2026 David Cormier
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 ****************************************************************************/

#ifndef VIDEOOUT_H
#define VIDEOOUT_H

#include <stdbool.h>

enum videoout_mode
{
    VIDEOOUT_OFF = 0,
    VIDEOOUT_AUTO,
    VIDEOOUT_ON,
};

enum videoout_accessory
{
    VIDEOOUT_ACCESSORY_NONE = 0,
    VIDEOOUT_ACCESSORY_PENDING,
    VIDEOOUT_ACCESSORY_VIDEO,
    VIDEOOUT_ACCESSORY_OTHER,
};

bool videoout_disable(void);
bool videoout_active(void);
bool videoout_lcd_clock_required(void);
bool videoout_mirror_yuv420(const unsigned char *luma,
                            const unsigned char *cb,
                            const unsigned char *cr,
                            int source_x, int source_y,
                            int source_stride,
                            int x, int y, int width, int height);
void videoout_mirror_rgb565(const void *source, int x, int y,
                            int width, int height, int stride);
void videoout_set_mode(enum videoout_mode mode, const void *framebuffer,
                       int width, int height);

/* IRQ-safe: requests, but does not perform, the sleeping ADC transaction. */
void videoout_request_accessory_identification(void);
void videoout_accessory_state(enum videoout_accessory accessory);

#endif /* VIDEOOUT_H */
