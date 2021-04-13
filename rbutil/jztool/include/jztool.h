/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef JZTOOL_H
#define JZTOOL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Types, enumerations, etc
 */

typedef struct jz_context       jz_context;
typedef struct jz_usbdev        jz_usbdev;
typedef struct jz_device_info   jz_device_info;
typedef struct jz_buffer        jz_buffer;
typedef struct jz_paramlist     jz_paramlist;

typedef enum jz_error           jz_error;
typedef enum jz_identify_error  jz_identify_error;
typedef enum jz_log_level       jz_log_level;
typedef enum jz_device_type     jz_device_type;
typedef enum jz_cpu_type        jz_cpu_type;

typedef void(*jz_log_cb)(jz_log_level, const char*);
typedef int(*jz_device_action_fn)(jz_context*, jz_paramlist*);

enum jz_error {
    JZ_SUCCESS = 0,
    JZ_ERR_OUT_OF_MEMORY = -1,
    JZ_ERR_OPEN_FILE = -2,
    JZ_ERR_FILE_IO = -3,
    JZ_ERR_USB = -4,
    JZ_ERR_NO_DEVICE = -5,
    JZ_ERR_BAD_FILE_FORMAT = -6,
    JZ_ERR_FLASH_ERROR = -7,
    JZ_ERR_OTHER = -99,
};

enum jz_identify_error {
    JZ_IDERR_OTHER = -1,
    JZ_IDERR_WRONG_SIZE = -2,
    JZ_IDERR_BAD_HEADER = -3,
    JZ_IDERR_BAD_CHECKSUM = -4,
    JZ_IDERR_UNRECOGNIZED_MODEL = -5,
};

enum jz_log_level {
    JZ_LOG_IGNORE  = -1,
    JZ_LOG_ERROR   = 0,
    JZ_LOG_WARNING = 1,
    JZ_LOG_NOTICE  = 2,
    JZ_LOG_DETAIL  = 3,
    JZ_LOG_DEBUG   = 4,
};

enum jz_device_type {
    JZ_DEVICE_FIIOM3K,
};

enum jz_cpu_type {
    JZ_CPU_X1000,
};

struct jz_device_info {
    const char* name;
    const char* description;
    jz_device_type device_type;
    jz_cpu_type cpu_type;
    uint16_t vendor_id;
    uint16_t product_id;
    int num_actions;
    const char* const* action_names;
    const jz_device_action_fn* action_funcs;
    const char* const* const* action_params;
};

struct jz_buffer {
    size_t size;
    uint8_t* data;
};

/******************************************************************************
 * Library context and general functions
 */

jz_context* jz_context_create(void);
void jz_context_destroy(jz_context* jz);

void jz_context_set_user_data(jz_context* jz, void* ptr);
void* jz_context_get_user_data(jz_context* jz);

void jz_context_set_log_cb(jz_context* jz, jz_log_cb cb);
void jz_context_set_log_level(jz_context* jz, jz_log_level lev);

void jz_log(jz_context* jz, jz_log_level lev, const char* fmt, ...);
void jz_log_cb_stderr(jz_log_level lev, const char* msg);

void jz_sleepms(int ms);

/******************************************************************************
 * Device and file info
 */

int jz_get_num_device_info(void);
const jz_device_info* jz_get_device_info(jz_device_type type);
const jz_device_info* jz_get_device_info_named(const char* name);
const jz_device_info* jz_get_device_info_indexed(int index);

int jz_identify_x1000_spl(const void* data, size_t len);
int jz_identify_scramble_image(const void* data, size_t len);
int jz_identify_fiiom3k_bootimage(const void* data, size_t len);

/******************************************************************************
 * USB boot ROM protocol
 */

int jz_usb_open(jz_context* jz, jz_usbdev** devptr, uint16_t vend_id, uint16_t prod_id);
void jz_usb_close(jz_usbdev* dev);

int jz_usb_send(jz_usbdev* dev, uint32_t addr, size_t len, const void* data);
int jz_usb_recv(jz_usbdev* dev, uint32_t addr, size_t len, void* data);
int jz_usb_start1(jz_usbdev* dev, uint32_t addr);
int jz_usb_start2(jz_usbdev* dev, uint32_t addr);
int jz_usb_flush_caches(jz_usbdev* dev);

/******************************************************************************
 * X1000 flash protocol
 */

int jz_x1000_setup(jz_usbdev* dev, size_t spl_len, const void* spl_data);
int jz_x1000_read_flash(jz_usbdev* dev, uint32_t addr, size_t len, void* data);
int jz_x1000_write_flash(jz_usbdev* dev, uint32_t addr, size_t len, const void* data);
int jz_x1000_boot_rockbox(jz_usbdev* dev);

/******************************************************************************
 * FiiO M3K bootloader backup/installation
 */

int jz_fiiom3k_readboot(jz_usbdev* dev, jz_buffer** bufptr);
int jz_fiiom3k_writeboot(jz_usbdev* dev, size_t image_size, const void* image_buf);
int jz_fiiom3k_patchboot(jz_context* jz, void* image_buf, size_t image_size,
                         const void* spl_buf, size_t spl_size,
                         const void* boot_buf, size_t boot_size);

int jz_fiiom3k_install(jz_context* jz, jz_paramlist* pl);
int jz_fiiom3k_backup(jz_context* jz, jz_paramlist* pl);
int jz_fiiom3k_restore(jz_context* jz, jz_paramlist* pl);

/******************************************************************************
 * Simple buffer API
 */

jz_buffer* jz_buffer_alloc(size_t size, const void* data);
void jz_buffer_free(jz_buffer* buf);

int jz_buffer_load(jz_buffer** buf, const char* filename);
int jz_buffer_save(jz_buffer* buf, const char* filename);

/******************************************************************************
 * Parameter list
 */

jz_paramlist* jz_paramlist_new(void);
void jz_paramlist_free(jz_paramlist* pl);
int jz_paramlist_set(jz_paramlist* pl, const char* param, const char* value);
const char* jz_paramlist_get(jz_paramlist* pl, const char* param);

/******************************************************************************
 * END
 */

#ifdef __cplusplus
}
#endif

#endif /* JZTOOL_H */
