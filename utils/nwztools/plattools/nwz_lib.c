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
#include "nwz_lib.h"
#include "nwz_db.h"

int nwz_run(const char *file, const char *args[], bool wait)
{
    pid_t child_pid = fork();
    if(child_pid != 0)
    {
        if(wait)
        {
            int status;
            waitpid(child_pid, &status, 0);
            return status;
        }
        else
            return 0;
    }
    else
    {
        execvp(file, (char * const *)args);
        _exit(1);
    }
}

char *nwz_run_pipe(const char *file, const char *args[], int *status)
{
    int pipe_fds[2];
    pipe(pipe_fds);
    pid_t child_pid = fork();
    if(child_pid == 0)
    {
        dup2(pipe_fds[1], 1); /* redirect stdout */
        dup2(pipe_fds[1], 2); /* redirect stderr */
        close(pipe_fds[0]); /* close reading */
        close(pipe_fds[1]); /* close writing */
        execvp(file, (char * const *)args);
        _exit(1);
    }
    else
    {
        close(pipe_fds[1]); /* close writing */
        char buffer[1024];
        char *output = malloc(1);
        ssize_t count;
        size_t size = 0;
        while((count = read(pipe_fds[0], buffer, sizeof(buffer))) > 0)
        {
            output = realloc(output, size + count + 1);
            memcpy(output + size, buffer, count);
            size += count;
        }
        close(pipe_fds[0]);
        output[size] = 0;
        waitpid(child_pid, status, 0);
        return output;
    }
}

void nwz_lcdmsg(bool clear, int x, int y, const char *msg)
{
    const char *path_lcdmsg = "/usr/local/bin/lcdmsg";
    const char *args[16];
    int index = 0;
    char locate[32];
    args[index++] = "lcdmsg";
    if(clear)
        args[index++] = "-c";
    args[index++] = "-f";
    args[index++] = "/usr/local/bin/font_08x12.bmp";
    args[index++] = "-l";
    sprintf(locate, "%d,%d", x, y);
    args[index++] = locate;
    args[index++] = msg;
    args[index++] = NULL;
    /* wait for lcdmsg to finish to avoid any race conditions in framebuffer
     * accesses */
    nwz_run(path_lcdmsg, args, true);
}

void nwz_lcdmsgf(bool clear, int x, int y, const char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    nwz_lcdmsg(clear, x, y, buffer);
}

#define NWZ_COLOR_RGB(col) \
    NWZ_COLOR_RED(col), NWZ_COLOR_GREEN(col), NWZ_COLOR_BLUE(col)

void nwz_display_clear(nwz_color_t color)
{
    const char *path_display = "/usr/local/bin/display";
    const char *args[16];
    int index = 0;
    char col[32];
    args[index++] = "display";
    args[index++] = "lcd";
    args[index++] = "clear";
    sprintf(col, "%d,%d,%d", NWZ_COLOR_RGB(color));
    args[index++] = col;
    args[index++] = NULL;
    /* wait for lcdmsg to finish to avoid any race conditions in framebuffer
     * accesses */
    nwz_run(path_display, args, true);
}

void nwz_display_text(int x, int y, bool big_font, nwz_color_t foreground_col,
    nwz_color_t background_col, int alpha, const char *text)
{
    const char *path_display = "/usr/local/bin/display";
    const char *args[16];
    int index = 0;
    char fg[32],bg[32], pos[32], transp[16];
    args[index++] = "display";
    args[index++] = "lcd";
    args[index++] = "text";
    sprintf(pos, "%d,%d", x, y);
    args[index++] = pos;
    if(big_font)
        args[index++] = "/usr/local/bin/font_14x24.bmp";
    else
        args[index++] = "/usr/local/bin/font_08x12.bmp";
    sprintf(fg, "%d,%d,%d", NWZ_COLOR_RGB(foreground_col));
    args[index++] = fg;
    sprintf(bg, "%d,%d,%d", NWZ_COLOR_RGB(background_col));
    args[index++] = bg;
    sprintf(transp, "%d", alpha);
    args[index++] = transp;
    args[index++] = text;
    args[index++] = NULL;
    /* wait for lcdmsg to finish to avoid any race conditions in framebuffer
     * accesses */
    nwz_run(path_display, args, true);
}

void nwz_display_text_center(int width, int y, bool big_font, nwz_color_t fg,
    nwz_color_t bg, int alpha, const char *text)
{
    int txt_w = NWZ_FONT_W(big_font) * strlen(text);
    nwz_display_text((width - txt_w) / 2, y, big_font, fg, bg, alpha, text);
}

void nwz_display_textf(int x, int y, bool big_font, nwz_color_t foreground_col,
    nwz_color_t background_col, int alpha, const char *fmt, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);
    nwz_display_text(x, y, big_font, foreground_col, background_col, alpha, buffer);
}

void nwz_display_textf_center(int width, int y, bool big_font, nwz_color_t fg,
    nwz_color_t bg, int alpha, const char *fmt, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);
    nwz_display_text_center(width, y, big_font, fg, bg, alpha, buffer);
}

void nwz_display_bitmap(int x, int y, const char *file, int left, int top,
    int width, int height, nwz_color_t key_col, int bmp_alpha)
{
    const char *path_display = "/usr/local/bin/display";
    const char *args[16];
    int index = 0;
    char pos[32], topleft[32], dim[32], key[32], transp[16];
    args[index++] = "display";
    args[index++] = "lcd";
    args[index++] = "bitmap";
    sprintf(pos, "%d,%d", x, y);
    args[index++] = pos;
    args[index++] = file;
    sprintf(topleft, "%d,%d", left, top);
    args[index++] = topleft;
    sprintf(dim, "%d,%d", width, height);
    args[index++] = dim;
    if(key_col == NWZ_COLOR_NO_KEY)
        sprintf(key, "no");
    else
        sprintf(key, "%d,%d,%d", NWZ_COLOR_RGB(key_col));
    args[index++] = key;
    sprintf(transp, "%d", bmp_alpha);
    args[index++] = transp;
    args[index++] = NULL;
    /* wait for lcdmsg to finish to avoid any race conditions in framebuffer
     * accesses */
    nwz_run(path_display, args, true);
}

int nwz_input_open(const char *requested_name)
{
    /* try all /dev/input/eventX, there can't a lot of them */
    for(int index = 0; index < 8; index++)
    {
        char buffer[32];
        sprintf(buffer, "/dev/input/event%d", index);
        int fd = open(buffer, O_RDWR);
        if(fd < 0)
            continue; /* try next one */
        /* query name */
        char name[256];
        if(ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0 &&
                strcmp(name, requested_name) == 0)
            return fd;
        close(fd);
    }
    return -1;
}

int nwz_key_open(void)
{
    return nwz_input_open(NWZ_KEY_NAME);
}

void nwz_key_close(int fd)
{
    close(fd);
}

int nwz_key_get_hold_status(int fd)
{
    unsigned long led_hold = 0;
    if(ioctl(fd, EVIOCGLED(sizeof(led_hold)), &led_hold) < 0)
        return -1;
    return led_hold;
}

int nwz_key_wait_event(int fd, long tmo_us)
{
    return nwz_wait_fds(&fd, 1, tmo_us);
}

int nwz_key_read_event(int fd, struct input_event *evt)
{
    int ret = read(fd, evt, sizeof(struct input_event));
    if(ret != sizeof(struct input_event))
        return -1;
    return 1;
}

int nwz_key_event_get_keycode(struct input_event *evt)
{
    return evt->code & NWZ_KEY_MASK;
}

bool nwz_key_event_is_press(struct input_event *evt)
{
    return evt->value == 0;
}

bool nwz_key_event_get_hold_status(struct input_event *evt)
{
    return !!(evt->code & NWZ_KEY_HOLD_MASK);
}

static const char *nwz_keyname[NWZ_KEY_MASK + 1] =
{
    [0 ... NWZ_KEY_MASK] = "unknown",
    [NWZ_KEY_PLAY] = "PLAY",
    [NWZ_KEY_RIGHT] = "RIGHT",
    [NWZ_KEY_LEFT] = "LEFT",
    [NWZ_KEY_UP] = "UP",
    [NWZ_KEY_DOWN] = "DOWN",
    [NWZ_KEY_ZAPPIN] = "ZAPPIN",
    [NWZ_KEY_AD0_6] = "AD0_6",
    [NWZ_KEY_AD0_7] = "AD0_7",
    [NWZ_KEY_NONE] = "NONE",
    [NWZ_KEY_VOL_DOWN] = "VOL DOWN",
    [NWZ_KEY_VOL_UP] = "VOL UP",
    [NWZ_KEY_BACK] = "BACK",
    [NWZ_KEY_OPTION] = "OPTION",
    [NWZ_KEY_BT] = "BT",
    [NWZ_KEY_AD1_5] = "AD1_5",
    [NWZ_KEY_AD1_6] = "AD1_6",
    [NWZ_KEY_AD1_7] = "AD1_7",
};

const char *nwz_key_get_name(int keycode)
{
    if(keycode <0 || keycode > NWZ_KEY_MASK)
        return "invalid";
    else
        return nwz_keyname[keycode];
}

int nwz_fb_open(bool lcd)
{
    return open(lcd ? NWZ_FB_LCD_DEV : NWZ_FB_TV_DEV, O_RDWR);
}

void nwz_fb_close(int fd)
{
    close(fd);
}

int nwz_fb_get_brightness(int fd, struct nwz_fb_brightness *bl)
{
    if(ioctl(fd, NWZ_FB_GET_BRIGHTNESS, bl) < 0)
        return -1;
    else
        return 1;
}

int nwz_fb_set_brightness(int fd, struct nwz_fb_brightness *bl)
{
    if(ioctl(fd, NWZ_FB_SET_BRIGHTNESS, bl) < 0)
        return -1;
    else
        return 1;
}

int nwz_fb_set_page(int fd, int page)
{
    /* set page mode to no transparency and no rotation */
    struct nwz_fb_image_info mode_info;
    mode_info.tc_enable = 0;
    mode_info.t_color = 0;
    mode_info.alpha = 0;
    mode_info.rot = 0;
    mode_info.page = page;
    mode_info.update = NWZ_FB_ONLY_2D_MODE;
    if(ioctl(fd, NWZ_FB_UPDATE, &mode_info) < 0)
        return -2;
    return 0;
}

int nwz_fb_set_standard_mode(int fd)
{
    /* disable timer (apparently useless with LCD) */
    struct nwz_fb_update_timer update_timer;
    update_timer.timerflag = NWZ_FB_TIMER_OFF;
    update_timer.timeout = NWZ_FB_DEFAULT_TIMEOUT;
    if(ioctl(fd, NWZ_FB_UPDATE_TIMER, &update_timer) < 0)
        return -1;
    return nwz_fb_set_page(fd, 0);
}

int nwz_fb_get_resolution(int fd, int *x, int *y, int *bpp)
{
    struct fb_var_screeninfo vinfo;
    if(ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0)
        return -1;
    if(x)
        *x = vinfo.xres;
    if(y)
        *y = vinfo.yres;
    if(bpp)
        *bpp = vinfo.bits_per_pixel;
    return 0;
}

void *nwz_fb_mmap(int fd, int offset, int size)
{
    return mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)offset);
}

int nwz_adc_open(void)
{
    return open(NWZ_ADC_DEV, O_RDONLY);
}

void nwz_adc_close(int fd)
{
    close(fd);
}

static const char *nwz_adc_name[] =
{
    [NWZ_ADC_VCCBAT] = "VCCBAT",
    [NWZ_ADC_VCCVBUS] = "VCCVBUS",
    [NWZ_ADC_ADIN3] = "ADIN3",
    [NWZ_ADC_ADIN4] = "ADIN4",
    [NWZ_ADC_ADIN5] = "ADIN5",
    [NWZ_ADC_ADIN6] = "ADIN6",
    [NWZ_ADC_ADIN7] = "ADIN7",
    [NWZ_ADC_ADIN8] = "ADIN8",
};

const char *nwz_adc_get_name(int ch)
{
    return nwz_adc_name[ch];
}

int nwz_adc_get_val(int fd, int ch)
{
    unsigned char val;
    if(ioctl(fd, NWZ_ADC_GET_VAL(ch), &val) < 0)
        return -1;
    else
        return val;
}

int nwz_ts_open(void)
{
    return nwz_input_open(NWZ_TS_NAME);
}

void nwz_ts_close(int fd)
{
    close(fd);
}

int nwz_ts_state_init(int fd, struct nwz_ts_state_t *state)
{
    memset(state, 0, sizeof(struct nwz_ts_state_t));
    struct input_absinfo info;
    if(ioctl(fd, EVIOCGABS(ABS_X), &info) < 0)
        return -1;
    state->max_x = info.maximum;
    if(ioctl(fd, EVIOCGABS(ABS_Y), &info) < 0)
        return -1;
    state->max_y = info.maximum;
    if(ioctl(fd, EVIOCGABS(ABS_PRESSURE), &info) < 0)
        return -1;
    state->max_pressure = info.maximum;
    if(ioctl(fd, EVIOCGABS(ABS_TOOL_WIDTH), &info) < 0)
        return -1;
    state->max_tool_width = info.maximum;
    return 1;
}

int nwz_ts_state_update(struct nwz_ts_state_t *state, struct input_event *evt)
{
    switch(evt->type)
    {
        case EV_SYN:
            return 1;
        case EV_REL:
            if(evt->code == REL_RX)
                state->flick_x = evt->value;
            else if(evt->code == REL_RY)
                state->flick_y = evt->value;
            else
                return -1;
            state->flick = true;
            break;
        case EV_ABS:
            if(evt->code == ABS_X)
                state->x = evt->value;
            else if(evt->code == ABS_Y)
                state->y = evt->value;
            else if(evt->code == ABS_PRESSURE)
                state->pressure = evt->value;
            else if(evt->code == ABS_TOOL_WIDTH)
                state->tool_width = evt->value;
            else
                return -1;
            break;
        case EV_KEY:
            if(evt->code == BTN_TOUCH)
                state->touch = evt->value;
            else
                return -1;
            break;
        default:
            return -1;
    }
    return 0;
}

int nwz_ts_state_post_syn(struct nwz_ts_state_t *state)
{
    state->flick = false;
    return 1;
}

int nwz_ts_read_events(int fd, struct input_event *evts, int nr_evts)
{
    int ret = read(fd, evts, nr_evts * sizeof(struct input_event));
    if(ret < 0)
        return -1;
    return ret / sizeof(struct input_event);
}

long nwz_wait_fds(int *fds, int nr_fds, long tmo_us)
{
    fd_set rfds;
    struct timeval tv;
    struct timeval *tv_ptr = NULL;
    /* watch the input device */
    FD_ZERO(&rfds);
    int max_fd = 0;
    for(int i = 0; i < nr_fds; i++)
    {
        FD_SET(fds[i], &rfds);
        if(fds[i] > max_fd)
            max_fd = fds[i];
    }
    /* setup timeout */
    if(tmo_us >= 0)
    {
        tv.tv_sec = 0;
        tv.tv_usec = tmo_us;
        tv_ptr = &tv;
    }
    int ret = select(max_fd + 1, &rfds, NULL, NULL, tv_ptr);
    if(ret <= 0)
        return ret;
    long bitmap = 0;
    for(int i = 0; i < nr_fds; i++)
        if(FD_ISSET(fds[i], &rfds))
            bitmap |= 1 << i;
    return bitmap;
}

int nwz_power_open(void)
{
    return open(NWZ_POWER_DEV, O_RDWR);
}

void nwz_power_close(int fd)
{
    close(fd);
}

int nwz_power_get_status(int fd)
{
    int status;
    if(ioctl(fd, NWZ_POWER_GET_STATUS, &status) < 0)
        return -1;
    return status;
}

static int nwz_power_adval_to_mv(int adval, int ad_base)
{
    if(adval == -1)
        return -1;
    /* the AD base corresponds to the millivolt value if adval was 255 */
    return (adval * ad_base) / 255;
}

int nwz_power_get_vbus_adval(int fd)
{
    int status;
    if(ioctl(fd, NWZ_POWER_GET_VBUS_ADVAL, &status) < 0)
        return -1;
    return status;
}

int nwz_power_get_vbus_voltage(int fd)
{
    return nwz_power_adval_to_mv(nwz_power_get_vbus_adval(fd), NWZ_POWER_AD_BASE_VBUS);
}

int nwz_power_get_vbus_limit(int fd)
{
    int status;
    if(ioctl(fd, NWZ_POWER_GET_VBUS_LIMIT, &status) < 0)
        return -1;
    return status;
}

int nwz_power_get_charge_switch(int fd)
{
    int status;
    if(ioctl(fd, NWZ_POWER_GET_CHARGE_SWITCH, &status) < 0)
        return -1;
    return status;
}

int nwz_power_get_charge_current(int fd)
{
    int status;
    if(ioctl(fd, NWZ_POWER_GET_CHARGE_CURRENT, &status) < 0)
        return -1;
    return status;
}

int nwz_power_get_battery_gauge(int fd)
{
    int status;
    if(ioctl(fd, NWZ_POWER_GET_BAT_GAUGE, &status) < 0)
        return -1;
    return status;
}

int nwz_power_get_battery_adval(int fd)
{
    int status;
    if(ioctl(fd, NWZ_POWER_GET_BAT_ADVAL, &status) < 0)
        return -1;
    return status;
}

int nwz_power_get_battery_voltage(int fd)
{
    return nwz_power_adval_to_mv(nwz_power_get_battery_adval(fd), NWZ_POWER_AD_BASE_VBAT);
}

int nwz_power_get_vbat_adval(int fd)
{
    int status;
    if(ioctl(fd, NWZ_POWER_GET_VBAT_ADVAL, &status) < 0)
        return -1;
    return status;
}

int nwz_power_get_vbat_voltage(int fd)
{
    return nwz_power_adval_to_mv(nwz_power_get_vbat_adval(fd), NWZ_POWER_AD_BASE_VBAT);
}

int nwz_power_get_sample_count(int fd)
{
    int status;
    if(ioctl(fd, NWZ_POWER_GET_SAMPLE_COUNT, &status) < 0)
        return -1;
    return status;
}

int nwz_power_get_vsys_adval(int fd)
{
    int status;
    if(ioctl(fd, NWZ_POWER_GET_VSYS_ADVAL, &status) < 0)
        return -1;
    return status;
}

int nwz_power_get_vsys_voltage(int fd)
{
    return nwz_power_adval_to_mv(nwz_power_get_vsys_adval(fd), NWZ_POWER_AD_BASE_VSYS);
}

int nwz_power_get_acc_charge_mode(int fd)
{
    int status;
    if(ioctl(fd, NWZ_POWER_GET_ACCESSARY_CHARGE_MODE, &status) < 0)
        return -1;
    return status;
}

int nwz_power_is_fully_charged(int fd)
{
    int status;
    if(ioctl(fd, NWZ_POWER_IS_FULLY_CHARGED, &status) < 0)
        return -1;
    return status;
}

int nwz_pminfo_open(void)
{
    return open(NWZ_PMINFO_DEV, O_RDONLY);
}

void nwz_pminfo_close(int fd)
{
    close(fd);
}

unsigned int nwz_pminfo_get_factor(int fd)
{
    unsigned int val;
    if(ioctl(fd, NWZ_PMINFO_GET_FACTOR, &val) < 0)
        return 0;
    else
        return val;
}

static unsigned long find_model_id(void)
{
    /* try with the environment variable */
    const char *mid = getenv("ICX_MODEL_ID");
    if(mid == NULL)
        return 0;
    char *end;
    unsigned long v = strtoul(mid, &end, 0);
    if(*end)
        return 0;
    else
        return v;
}

unsigned long nwz_get_model_id(void)
{
    static unsigned long model_id = 0xffffffff;
    if(model_id == 0xffffffff)
        model_id = find_model_id();
    return model_id;
}

const char *nwz_get_model_name()
{
    for(int i = 0; i < NWZ_MODEL_COUNT; i++)
        if(nwz_model[i].mid == nwz_get_model_id())
            return nwz_model[i].name;
    return NULL;
}

static int find_series(void)
{
    for(int i = 0; i < NWZ_SERIES_COUNT; i++)
        for(int j = 0; j < nwz_series[i].mid_count; j++)
            if(nwz_series[i].mid[j] == nwz_get_model_id())
                return i;
    return -1;
}

int nwz_get_series(void)
{
    static int series = -2;
    if(series == -2)
        series = find_series();
    return series;
}

static nwz_nvp_index_t *get_nvp_index(void)
{
    static nwz_nvp_index_t *index = 0;
    if(index == 0)
    {
        int series = nwz_get_series();
        index = series < 0 ? 0 : nwz_series[series].nvp_index;
    }
    return index;
}

int nwz_nvp_read(enum nwz_nvp_node_t node, void *data)
{
    int size = nwz_nvp[node].size;
    if(data == 0)
        return size;
    nwz_nvp_index_t *index = get_nvp_index();
    if(index == 0 || (*index)[node] == NWZ_NVP_INVALID)
        return -1;
    char nvp_path[32];
    snprintf(nvp_path, sizeof(nvp_path), "/dev/icx_nvp/%03d", (*index)[node]);
    int fd = open(nvp_path, O_RDONLY);
    if(fd < 0)
        return -1;
    int cnt = read(fd, data, size);
    close(fd);
    return cnt == size ? size : -1;
}

int nwz_nvp_write(enum nwz_nvp_node_t node, void *data)
{
    int size = nwz_nvp[node].size;
    nwz_nvp_index_t *index = get_nvp_index();
    if(index == 0 || (*index)[node] == NWZ_NVP_INVALID)
        return -1;
    char nvp_path[32];
    snprintf(nvp_path, sizeof(nvp_path), "/dev/icx_nvp/%03d", (*index)[node]);
    int fd = open(nvp_path, O_WRONLY);
    if(fd < 0)
        return -1;
    int cnt = write(fd, data, size);
    close(fd);
    return cnt == size ? 0 : -1;
}
