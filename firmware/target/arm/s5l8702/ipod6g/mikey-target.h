/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Hemant Kumar
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
#ifndef __MIKEY_TARGET_H__
#define __MIKEY_TARGET_H__

/* "Mikey" is the internal controller for the headphone jack microphone
 * and inline remote (I2C bus 0, address 0x72). */

/* Low-level register access. */
unsigned char mikey_read(int address);
int  mikey_write(int address, unsigned char val);
void mikey_reset(void);

/* Recording-path handover: while the jack mic is enabled the remote
 * polling backs off and Mikey stays in plain mic-bias mode. */
void mikey_set_mic_capture(bool enable);

/* Inline-remote button support. mikey_init() starts a polling thread;
 * mikey_button_read() returns the current button mask (multimedia key
 * codes, handled globally by default_event_handler: PLAYPAUSE for the
 * center click, VOLUME_UP/DOWN for volume; BUTTON_NONE if nothing
 * pressed) and is safe to call from the button tick. */
void mikey_init(void);
int  mikey_button_read(void);

/* True once the controller has ever ACKed the bus (it only responds
 * while the jack is occupied, so this latches on first insertion;
 * stays false on the early 2007 models, which lack the chip). Shown
 * in the hardware debug screen. */
bool mikey_present(void);

#endif /* __MIKEY_TARGET_H__ */
