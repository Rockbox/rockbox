/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009, 2010 Wincent Balin
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

#ifndef PDBOX_H
#define PDBOX_H

/* Use TLSF. */
#include <tlsf.h>

/* Pure Data */
#include "PDa/src/m_pd.h"

/* Minimal memory size. */
#define MIN_MEM_SIZE (4 * 1024 * 1024)

/* Memory prototypes. */

/* Direct memory allocator functions to TLSF. */
#define malloc(size) tlsf_malloc(size)
#define free(ptr) tlsf_free(ptr)
#define realloc(ptr, size) tlsf_realloc(ptr, size)
#define calloc(elements, elem_size) tlsf_calloc(elements, elem_size)

/* Audio declarations. */
#ifdef SIMULATOR
  #define PD_SAMPLERATE 44100
#elif (HW_SAMPR_CAPS & SAMPR_CAP_22)
  #define PD_SAMPLERATE 22050
#elif (HW_SAMPR_CAPS & SAMPR_CAP_32)
  #define PD_SAMPLERATE 32000
#elif (HW_SAMPR_CAPS & SAMPR_CAP_44)
  #define PD_SAMPLERATE 44100
#else
  #error No sufficient sample rate available!
#endif
#define PD_SAMPLES_PER_HZ ((PD_SAMPLERATE / HZ) + \
                           (PD_SAMPLERATE % HZ > 0 ? 1 : 0))
#define PD_OUT_CHANNELS 2

/* Audio buffer part. Contains data for one HZ period. */
#ifdef SIMULATOR
#define AUDIOBUFSIZE (PD_SAMPLES_PER_HZ * PD_OUT_CHANNELS * 16)
#else
#define AUDIOBUFSIZE (PD_SAMPLES_PER_HZ * PD_OUT_CHANNELS)
#endif
struct audio_buffer
{
    int16_t data[AUDIOBUFSIZE];
    unsigned int fill;
};


/* Additional functions. */
char *rb_strncat(char *s, const char *t, size_t n);
double rb_strtod(const char*, char**);
double rb_atof(const char*);
void rb_ftoan(float, char*, int);
float rb_floor(float);
long rb_atol(const char* s);
float rb_sin(float rad);
float rb_cos(float rad);
int rb_fscanf_f(int fd, float* f);
int rb_fprintf_f(int fd, float f);
float rb_log10(float);
float rb_log(float);
float rb_exp(float);
float rb_pow(float, float);
float rb_sqrt(float);
float rb_fabs(float);
float rb_atan(float);
float rb_atan2(float, float);
float rb_sinh(float);
float rb_tan(float);
//#ifndef SIMULATOR
/*FIXME: is it a correct replacement??? */
#if !(CONFIG_PLATFORM & PLATFORM_HOSTED)
typedef struct
{
    int quot;
    int rem;
}
div_t;
div_t div(int x, int y);
#endif
union f2i
{
    float f;
    int32_t i;
};
void sys_findlibdir(const char* filename);
int sys_startgui(const char *guidir);


/* Network declarations. */

/* Maximal size of the datagram. */
#define MAX_DATAGRAM_SIZE 255

/* This structure replaces a UDP datagram. */
struct datagram
{
    bool used;
    uint8_t size;
    char data[MAX_DATAGRAM_SIZE];
};

/* Prototypes of network functions. */
void net_init(void);
void net_destroy(void);
bool send_datagram(struct event_queue* route, int port,
                   char* data, size_t size);
bool receive_datagram(struct event_queue* route, int port,
                      struct datagram* buffer);

/* Network message queues. */
extern struct event_queue gui_to_core;
extern struct event_queue core_to_gui;

/* UDP ports of the original software. */
#define PD_CORE_PORT 3333
#define PD_GUI_PORT 3334

/* Convinience macros. */
#define SEND_TO_CORE(data) \
    send_datagram(&gui_to_core, PD_CORE_PORT, data, strlen(data))
#define RECEIVE_TO_CORE(buffer) \
    receive_datagram(&gui_to_core, PD_CORE_PORT, buffer)
#define SEND_FROM_CORE(data) \
    send_datagram(&core_to_gui, PD_GUI_PORT, data, strlen(data))
#define RECEIVE_FROM_CORE(buffer) \
    receive_datagram(&core_to_gui, PD_GUI_PORT, buffer)

/* PD core message callback. */
void rockbox_receive_callback(struct datagram* dg);


/* Pure Data declarations. */

/* Pure Data function prototypes. */
void pd_init(void);


/* Redefinitons of ANSI C functions. */
#include "lib/wrappers.h"

#define strncmp rb->strncmp
#define atoi rb->atoi
#define write rb->write

#undef strncat
#define strncat rb_strncat

//#ifndef SIMULATOR
/*FIXME: is it a correct replacement??? */
/* #if !(CONFIG_PLATFORM & PLATFORM_HOSTED) */
#define floor rb_floor
#define atof rb_atof
#define atol rb_atol
#define sin rb_sin
#define cos rb_cos
#define log10 rb_log10
#define log rb_log
#define exp rb_exp
#define pow rb_pow
#define sqrt rb_sqrt
#define fabs rb_fabs
#define atan rb_atan
#define atan2 rb_atan2
#define sinh rb_sinh
#define tan rb_tan
/*
#else
#include <math.h>
#endif
*/

#define ftoan rb_ftoan
#undef strtok_r
#define strtok_r rb->strtok_r
#define strstr rb->strcasestr


/* PdPod GUI declarations. */

enum pd_widget_id
{
    PD_BANG,
    PD_VSLIDER,
    PD_HSLIDER,
    PD_VRADIO,
    PD_HRADIO,
    PD_NUMBER,
    PD_SYMBOL,
    PD_TEXT
};

struct pd_widget
{
    enum pd_widget_id id;
    char name[128];
    int x;
    int y;
    int w;
    int h;
    int min;
    int max;
    float value;
    int timeout;
};

enum pd_key_id
{
    KEY_PLAY,
    KEY_REWIND,
    KEY_FORWARD,
    KEY_MENU,
    KEY_ACTION,
    KEY_WHEELLEFT,
    KEY_WHEELRIGHT,
    PD_KEYS
};

/* Map real keys to virtual ones.
   Feel free to add your preferred keymap here. */
#if (CONFIG_KEYPAD == IRIVER_H300_PAD)
    /* Added by wincent */
    #define PDPOD_QUIT (BUTTON_OFF)
    #define PDPOD_PLAY (BUTTON_ON)
    #define PDPOD_PREVIOUS (BUTTON_LEFT)
    #define PDPOD_NEXT (BUTTON_RIGHT)
    #define PDPOD_MENU (BUTTON_SELECT)
    #define PDPOD_WHEELLEFT (BUTTON_DOWN)
    #define PDPOD_WHEELRIGHT (BUTTON_UP)
    #define PDPOD_ACTION (BUTTON_MODE)
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD)
    /* Added by wincent */
    #define PDPOD_QUIT (BUTTON_OFF)
    #define PDPOD_PLAY (BUTTON_REC)
    #define PDPOD_PREVIOUS (BUTTON_LEFT)
    #define PDPOD_NEXT (BUTTON_RIGHT)
    #define PDPOD_MENU (BUTTON_SELECT)
    #define PDPOD_WHEELLEFT (BUTTON_DOWN)
    #define PDPOD_WHEELRIGHT (BUTTON_UP)
    #define PDPOD_ACTION (BUTTON_ON)
#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
    /* Added by funman */
    #define PDPOD_QUIT (BUTTON_HOME|BUTTON_REPEAT)
    #define PDPOD_PLAY BUTTON_UP
    #define PDPOD_PREVIOUS BUTTON_LEFT
    #define PDPOD_NEXT BUTTON_RIGHT
    #define PDPOD_MENU BUTTON_SELECT
    #define PDPOD_WHEELLEFT BUTTON_SCROLL_BACK
    #define PDPOD_WHEELRIGHT BUTTON_SCROLL_FWD
    #define PDPOD_ACTION BUTTON_DOWN
#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
    #define PDPOD_QUIT (BUTTON_POWER|BUTTON_REPEAT)
    #define PDPOD_PLAY BUTTON_UP
    #define PDPOD_PREVIOUS BUTTON_LEFT
    #define PDPOD_NEXT BUTTON_RIGHT
    #define PDPOD_MENU BUTTON_DOWN
    #define PDPOD_WHEELLEFT BUTTON_SCROLL_BACK
    #define PDPOD_WHEELRIGHT BUTTON_SCROLL_FWD
    #define PDPOD_ACTION BUTTON_SELECT
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
    /* Added by wincent */
    #define PDPOD_QUIT (BUTTON_SELECT | BUTTON_MENU)
    #define PDPOD_PLAY (BUTTON_PLAY)
    #define PDPOD_PREVIOUS (BUTTON_LEFT)
    #define PDPOD_NEXT (BUTTON_RIGHT)
    #define PDPOD_MENU (BUTTON_MENU)
    #define PDPOD_WHEELLEFT (BUTTON_SCROLL_BACK)
    #define PDPOD_WHEELRIGHT (BUTTON_SCROLL_FWD)
    #define PDPOD_ACTION (BUTTON_SELECT)
#elif (CONFIG_KEYPAD == SAMSUNG_YPR0_PAD)
    #define PDPOD_QUIT      BUTTON_BACK
    #define PDPOD_PLAY      BUTTON_USER
    #define PDPOD_PREVIOUS  BUTTON_LEFT
    #define PDPOD_NEXT      BUTTON_RIGHT
    #define PDPOD_MENU      BUTTON_MENU
    #define PDPOD_WHEELLEFT BUTTON_UP
    #define PDPOD_WHEELRIGHT BUTTON_DOWN
    #define PDPOD_ACTION    BUTTON_SELECT
#elif (CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD)
    #define PDPOD_QUIT      BUTTON_POWER
    #define PDPOD_PLAY      BUTTON_PLAYPAUSE
    #define PDPOD_PREVIOUS  BUTTON_LEFT
    #define PDPOD_NEXT      BUTTON_RIGHT
    #define PDPOD_MENU      BUTTON_BACK
    #define PDPOD_WHEELLEFT BUTTON_UP
    #define PDPOD_WHEELRIGHT BUTTON_DOWN
    #define PDPOD_ACTION    BUTTON_SELECT
#else
    #warning "No keys defined for this architecture!"
#endif

/* Prototype of GUI functions. */
void pd_gui_init(void);
unsigned int pd_gui_load_patch(struct pd_widget* wg, unsigned int max_widgets);
void pd_gui_draw(struct pd_widget* wg, unsigned int widgets);
bool pd_gui_parse_buttons(unsigned int widgets);
void pd_gui_parse_message(struct datagram* dg,
                          struct pd_widget* wg, unsigned int widgets);
bool pd_gui_apply_timeouts(struct pd_widget* wg, unsigned int widgets);

#endif
