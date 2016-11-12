/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 Amaury Pouly
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
#ifndef _NWZLIB_H_
#define _NWZLIB_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/input.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "nwz_keys.h"
#include "nwz_fb.h"
#include "nwz_adc.h"
#include "nwz_ts.h"
#include "nwz_power.h"
#include "nwz_db.h"

/* get model ID, either from ICX_MODEL_ID env var or using nvpflag, return 0
 * in case of error; note that the result is cached so this function is only
 * expensive the first time it is called */
unsigned long nwz_get_model_id(void);
/* get series (index into nwz_series, or -1 on error) */
int nwz_get_series(void);
/* get model name, or null on error */
const char *nwz_get_model_name(void);

/* run a program and exit with nonzero status in case of error
 * argument list must be NULL terminated */
int nwz_run(const char *file, const char *args[], bool wait);
/* run a program and return program output */
char *nwz_run_pipe(const char *file, const char *args[], int *status);

/* invoke /usr/local/bin/lcdmsg to display a message using the small font, optionally
 * clearing the screen before */
void nwz_lcdmsg(bool clear, int x, int y, const char *msg);
void nwz_lcdmsgf(bool clear, int x, int y, const char *format, ...);
/* invoke /usr/local/bin/display to do various things:
 * - clear screen
 * - display text
 * - display bitmap
 * Currently all operations are performed on the LCD only.
 * The small text font is 8x12 and the big one is 14x24 */
typedef int nwz_color_t;
#define NWZ_COLOR(r, g, b) /* each component between 0 and 255 */ \
    ((r) <<  16 | (g) << 8 | (b))
#define NWZ_COLOR_RED(col)      ((col) >> 16)
#define NWZ_COLOR_GREEN(col)    (((col) >> 8) & 0xff)
#define NWZ_COLOR_BLUE(col)    ((col) & 0xff)
#define NWZ_COLOR_NO_KEY    (1 << 24)

#define NWZ_FONT_W(big_font) ((big_font) ? 14 : 8)
#define NWZ_FONT_H(big_font) ((big_font) ? 24 : 14)

void nwz_display_clear(nwz_color_t color);
void nwz_display_text(int x, int y, bool big_font, nwz_color_t foreground_col,
    nwz_color_t background_col, int background_alpha, const char *text);
void nwz_display_text_center(int width, int y, bool big_font, nwz_color_t foreground_col,
    nwz_color_t background_col, int background_alpha, const char *text);
void nwz_display_textf(int x, int y, bool big_font, nwz_color_t foreground_col,
    nwz_color_t background_col, int background_alpha, const char *fmt, ...);
void nwz_display_textf_center(int width, int y, bool big_font, nwz_color_t foreground_col,
    nwz_color_t background_col, int background_alpha, const char *fmt, ...);
void nwz_display_bitmap(int x, int y, const char *file, int left, int top,
    int width, int height, nwz_color_t key, int bmp_alpha);

/* open icx_key input device and return file descriptor */
int nwz_key_open(void);
void nwz_key_close(int fd);
/* return HOLD status: 0 or 1, or -1 on error */
int nwz_key_get_hold_status(int fd);
/* wait for an input event (and return 1), or a timeout (return 0), or error (-1)
 * set the timeout to -1 to block */
int nwz_key_wait_event(int fd, long tmo_us);
/* read an event from the device (may block unless you waited for an event before),
 * return 1 on success, <0 on error */
int nwz_key_read_event(int fd, struct input_event *evt);
/* return keycode from event */
int nwz_key_event_get_keycode(struct input_event *evt);
/* return press/released status from event */
bool nwz_key_event_is_press(struct input_event *evt);
/* return HOLD status from event */
bool nwz_key_event_get_hold_status(struct input_event *evt);
/* get keycode name */
const char *nwz_key_get_name(int keycode);

/* open framebuffer device */
int nwz_fb_open(bool lcd);
/* close framebuffer device */
void nwz_fb_close(int fb);
/* get screen resolution, parameters are allowed to be NULL */
int nwz_fb_get_resolution(int fd, int *x, int *y, int *bpp);
/* get backlight brightness (return -1 on error, 1 on success) */
int nwz_fb_get_brightness(int fd, struct nwz_fb_brightness *bl);
/* set backlight brightness (return -1 on error, 1 on success) */
int nwz_fb_set_brightness(int fd, struct nwz_fb_brightness *bl);
/* setup framebuffer to its standard mode: LCD output, page 0, no transparency
 * and no rotation, 2D only updates */
int nwz_fb_set_standard_mode(int fd);
/* change framebuffer page and update screen */
int nwz_fb_set_page(int fd, int page);
/* map framebuffer */
void *nwz_fb_mmap(int fd, int offset, int size);

/* open adc device */
int nwz_adc_open(void);
/* close adc device */
void nwz_adc_close(int fd);
/* get channel name */
const char *nwz_adc_get_name(int ch);
/* read channel value, return -1 on error */
int nwz_adc_get_val(int fd, int ch);

/* open touchscreen device */
int nwz_ts_open(void);
/* close touchscreen device */
void nwz_ts_close(int fd);
/* structure to track touch state */
struct nwz_ts_state_t
{
    int x, y; /* current position (valid is touch is true) */
    int max_x, max_y; /* maximum possible values */
    int pressure, tool_width; /* current pressure and tool width */
    int max_pressure, max_tool_width; /* maximum possible values */
    bool touch; /* is the user touching the screen? */
    bool flick; /* was the action a flick? */
    int flick_x, flick_y; /* if so, this is the flick direction */
};
/* get touchscreen information and init state, return -1 on error, 1 on success */
int nwz_ts_state_init(int fd, struct nwz_ts_state_t *state);
/* update state with an event, return -1 on unhandled event, >=0 on handled:
 * 1 if sync event, 0 otherwise */
int nwz_ts_state_update(struct nwz_ts_state_t *state, struct input_event *evt);
/* update state after a sync event to prepare for next round of events */
int nwz_ts_state_post_syn(struct nwz_ts_state_t *state);
/* read at most N events from touch screen, and return the number of events */
int nwz_ts_read_events(int fd, struct input_event *evts, int nr_evts);

/* wait for events on several file descriptors, return a bitmap of active ones
 * or 0 on timeout, the timeout can be -1 to block */
long nwz_wait_fds(int *fds, int nr_fds, long timeout_us);

/* open power device */
int nwz_power_open(void);
/* close power device */
void nwz_power_close(int fd);
/* get power status (return -1 on error, bitmap on success) */
int nwz_power_get_status(int fd);
/* get vbus adval (or -1 on error) */
int nwz_power_get_vbus_adval(int fd);
/* get vbus voltage in mV (or -1 on error) */
int nwz_power_get_vbus_voltage(int fd);
/* get vbus current limit (or -1 on error) */
int nwz_power_get_vbus_limit(int fd);
/* get charge switch (or -1 on error) */
int nwz_power_get_charge_switch(int fd);
/* get charge current (or -1 on error) */
int nwz_power_get_charge_current(int fd);
/* get battery gauge (or -1 on error) */
int nwz_power_get_battery_gauge(int fd);
/* get battery adval (or -1 on error) */
int nwz_power_get_battery_adval(int fd);
/* get battery voltage in mV (or -1 on error) */
int nwz_power_get_battery_voltage(int fd);
/* get vbat adval (or -1 on error) */
int nwz_power_get_vbat_adval(int fd);
/* get vbat voltage (or -1 on error) */
int nwz_power_get_vbat_voltage(int fd);
/* get sample count (or -1 on error) */
int nwz_power_get_sample_count(int fd);
/* get vsys adval (or -1 on error) */
int nwz_power_get_vsys_adval(int fd);
/* get vsys voltage in mV (or -1 on error) */
int nwz_power_get_vsys_voltage(int fd);
/* get accessory charge mode */
int nwz_power_get_acc_charge_mode(int fd);
/* is battery fully charged? (or -1 on error) */
int nwz_power_is_fully_charged(int fd);

/* open pminfo device */
int nwz_pminfo_open(void);
/* close pminfo device */
void nwz_pminfo_close(int fd);
/* get pminfo factor (or 0 on error) */
unsigned int nwz_pminfo_get_factor(int fd);

/* read a nvp node and return its size, if the data pointer is null, then simply
 * return the size, return -1 on error */
int nwz_nvp_read(enum nwz_nvp_node_t node, void *data);
/* write a nvp node, return 0 on success and -1 on error, the size of the buffer
 * must be the one returned by nwz_nvp_read */
int nwz_nvp_write(enum nwz_nvp_node_t node, void *data);


#endif /* _NWZLIB_H_ */
