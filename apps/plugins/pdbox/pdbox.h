/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Wincent Balin
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


#if 1
/* Use TLSF. */
#include "codecs/lib/tlsf/src/tlsf.h"
#endif

/* Pure Data */
#include "PDa/src/m_pd.h"

/* Minimal memory size. */
#define MIN_MEM_SIZE (4 * 1024 * 1024)

/* Memory prototypes. */

#if 1
/* Direct memory allocator functions to TLSF. */
#define malloc(size) tlsf_malloc(size)
#define free(ptr) tlsf_free(ptr)
#define realloc(ptr, size) tlsf_realloc(ptr, size)
#define calloc(elements, elem_size) tlsf_calloc(elements, elem_size)
#endif

#if 0
extern void set_memory_pool(void* memory_pool, size_t memory_size);
extern void clear_memory_pool(void);
extern void* mcalloc(size_t nmemb, size_t size);
extern void* mmalloc(size_t size);
extern void mfree(void* ptr);
extern void* mrealloc(void* ptr, size_t size);
extern void print_memory_pool(void);

#define malloc mmalloc
#define free mfree
#define realloc mrealloc
#define calloc mcalloc
#endif

#if 0
#include <stdlib.h>
#define malloc malloc
#define free free
#define realloc realloc
#define calloc calloc
#endif

/* Audio declarations. */
#define PD_SAMPLERATE 22050
#define PD_SAMPLES_PER_HZ ((PD_SAMPLERATE / HZ) + \
                           (PD_SAMPLERATE % HZ > 0 ? 1 : 0))
#define PD_OUT_CHANNELS 2

/* Audio buffer part. Contains data for one HZ period. */
#ifdef SIMULATOR
#define AUDIOBUFSIZE (PD_SAMPLES_PER_HZ * PD_OUT_CHANNELS * 32)
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
typedef struct
{
    int quot;
    int rem;
}
div_t;
div_t div(int x, int y);
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

#define strncat rb_strncat
#define floor rb_floor
#define atof rb_atof
#define atol rb_atol
#define ftoan rb_ftoan
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
#if defined(IRIVER_H300_SERIES)
    /* Added by wincent */
    #define PDPOD_QUIT (BUTTON_OFF)
    #define PDPOD_PLAY (BUTTON_ON)
    #define PDPOD_PREVIOUS (BUTTON_LEFT)
    #define PDPOD_NEXT (BUTTON_RIGHT)
    #define PDPOD_MENU (BUTTON_SELECT)
    #define PDPOD_WHEELLEFT (BUTTON_DOWN)
    #define PDPOD_WHEELRIGHT (BUTTON_UP)
    #define PDPOD_ACTION (BUTTON_MODE)
#elif defined(IRIVER_H100_SERIES)
    /* Added by wincent */
    #define PDPOD_QUIT (BUTTON_OFF)
    #define PDPOD_PLAY (BUTTON_REC)
    #define PDPOD_PREVIOUS (BUTTON_LEFT)
    #define PDPOD_NEXT (BUTTON_RIGHT)
    #define PDPOD_MENU (BUTTON_SELECT)
    #define PDPOD_WHEELLEFT (BUTTON_DOWN)
    #define PDPOD_WHEELRIGHT (BUTTON_UP)
    #define PDPOD_ACTION (BUTTON_ON)
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
